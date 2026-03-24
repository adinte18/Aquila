#ifndef AQUILA_RENDERER_H
#define AQUILA_RENDERER_H

#include "Aquila/Graphics/Core/CommandBuffer.h"
#include "Aquila/Graphics/Core/CommandBufferPool.h"
#include "Aquila/Graphics/Core/Swapchain.h"
#include "Aquila/Graphics/Core/SynchronizationManager.h"
#include "Aquila/Graphics/Pipeline/Descriptor.h"
#include "Aquila/Graphics/Pipeline/DescriptorAllocator.h"
#include "Aquila/Graphics/Pipeline/Rendertarget.h"
#include "Aquila/Rendering/Camera.h"
#include "Aquila/Rendering/Systems/CascadedShadowMapsRenderingSystem.h"
#include "Aquila/Rendering/Systems/CompositeRenderingSystem.h"
#include "Aquila/Rendering/Systems/DeferredRenderingSystem.h"
#include "Aquila/Rendering/Systems/GizmoRenderingSystem.h"
#include "Aquila/Rendering/Systems/LightingRenderingSystem.h"
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Graphics/Core/DeletionManager.h"

#include "Aquila/Graphics/Pipeline/SceneRenderingContext.h"
#include "Aquila/Rendering/Environment/SkyboxRenderingSystem.h"
#include "Aquila/Scene/Components/TransformComponent.h"

namespace Aquila::Graphics {

using namespace Aquila::Rendering;

/**
 * @brief Complete rendering context for a single scene.
 * Each scene has its own frame cycling, command buffers, and descriptor sets.
 */
struct SceneRenderContext {
	struct CameraGPUData {
		alignas(16) mat4 view;				// 64
		alignas(16) mat4 projection;		// 64
		alignas(16) mat4 viewProjection;	// 64
		alignas(16) mat4 inverseView;		// 64
		alignas(16) mat4 inverseProjection; // 64
		alignas(16) vec3 cameraPosition;	// 12
		f32 nearPlane;						// 4
		f32 farPlane;						// 4
	}; // 340 total bytes

	struct RenderTargets {
		Ref<Aquila::Graphics::RenderingPipeline::RenderTarget> gbuffer;
		Ref<Aquila::Graphics::RenderingPipeline::RenderTarget> lighting;
		Ref<Aquila::Graphics::RenderingPipeline::RenderTarget> skybox;
		Ref<Aquila::Graphics::RenderingPipeline::RenderTarget> gizmo;
		Ref<Aquila::Graphics::RenderingPipeline::RenderTarget> composite;
		Ref<Aquila::Graphics::RenderingPipeline::RenderTarget> shadow; // Shadow maps
	} renderTargets;

	std::vector<Ref<CommandBuffer>> commandBuffers;
	VkExtent2D extent{ 0, 0 };
	std::vector<VkFence> renderFences;

	uint32 currentFrameIndex = 0;
	SceneManagement::Scene *scene = nullptr;

	std::vector<Unique<Resources::Buffer>> cameraUBOs{};
	std::vector<VkDescriptorSet> cameraDescriptors{};

	/**
     * @brief Create all resources for this scene
     */
	void Create(Device &device, SceneManagement::Scene *scenePtr, const VkExtent2D &size, CommandBufferPool *cmdPool,
				const Ref<RenderingPipeline::DescriptorSetLayout> &cameraLayout,
				const std::vector<Systems::Systems::RenderingSystemBase *> &systems) {
		if (size.width == 0 || size.height == 0) {
			throw std::runtime_error("Invalid scene extent");
		}

		scene = scenePtr;
		extent = size;

		AQUILA_LOG_INFO("Creating scene context for '{}' ({}x{})", scenePtr->GetSceneName(), size.width, size.height);

		CreateRenderTargets(device, size.width, size.height);
		CreateCameraResources(device, *cameraLayout);

		commandBuffers.reserve(SharedConstants::MAX_FRAMES_IN_FLIGHT);
		for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
			std::string name = "Scene_" + scenePtr->GetSceneName() + "_Frame" + std::to_string(i);
			commandBuffers.push_back(cmdPool->GetFrameCommandBuffer(CommandBufferType::OFFSCREEN, i, name));
		}

		renderFences.resize(SharedConstants::MAX_FRAMES_IN_FLIGHT);
		for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			if (vkCreateFence(device.GetDevice(), &fenceInfo, nullptr, &renderFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create scene render fence");
			}
		}

		InitializeDescriptorSets(systems);
	}

	/**
     * @brief Advance to next frame index for this scene
     */
	void AdvanceFrame() { currentFrameIndex = (currentFrameIndex + 1) % SharedConstants::MAX_FRAMES_IN_FLIGHT; }

	/**
     * @brief Get current frame's command buffer
     */
	CommandBuffer *GetCurrentCommandBuffer() { return commandBuffers[currentFrameIndex].get(); }

	/**
	 * @brief Initialize descriptor sets for all rendering systems
	 */
	void InitializeDescriptorSets(const std::vector<Systems::RenderingSystemBase *> &systems) const {
		for (auto *system : systems) {
			if (system != nullptr) {
				system->AllocateDescriptorSetsForScene(scene);
			}
		}
		AQUILA_LOG_DEBUG("Initialized descriptor sets for scene '{}'", scene->GetSceneName());
	}

	/**
	 * @brief Clean up descriptor sets for this scene
	 */
	void CleanupDescriptorSets(const std::vector<Systems::RenderingSystemBase *> &systems) const {
		if (scene == nullptr) {
			AQUILA_LOG_ERROR("Cannot cleanup descriptor sets - scene is null");
			return;
		}

		for (auto *system : systems) {
			if (system != nullptr) {
				system->CleanupSceneDescriptors(scene);
			}
		}
		AQUILA_LOG_DEBUG("Cleaned up descriptor sets for scene '{}'", scene->GetSceneName());
	}

	CommandBuffer *GetCurrentCommandBufferSafe(Device &device) {
		VkFence fence = renderFences[currentFrameIndex];
		vkWaitForFences(device.GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
		vkResetFences(device.GetDevice(), 1, &fence);

		return commandBuffers[currentFrameIndex].get();
	}

	/**
	 * @brief Get the fence for the current frame
	 */
	[[nodiscard]] VkFence GetCurrentFence() const { return renderFences[currentFrameIndex]; }

	/**
     * @brief Resize all resources
     */
	void Resize(uint32 width, uint32 height) {
		if (width == 0 || height == 0) {
			AQUILA_LOG_ERROR("Cannot resize to zero dimensions");
			return;
		}

		if (extent.width == width && extent.height == height) {
			return;
		}

		AQUILA_LOG_INFO("Resizing scene '{}' from {}x{} to {}x{}", scene->GetSceneName(), extent.width, extent.height,
						width, height);

		extent = { .width = width, .height = height };

		renderTargets.gbuffer->Resize(width, height);
		renderTargets.lighting->Resize(width, height);
		renderTargets.skybox->Resize(width, height);
		renderTargets.gizmo->Resize(width, height);
		renderTargets.composite->Resize(width, height);
	}

	/**
     * @brief Queue all resources for deletion
     */
	void QueueForDeletion(Device &device, const std::vector<Systems::RenderingSystemBase *> &systems) {
		CleanupDescriptorSets(systems);

		if (renderTargets.gbuffer) {
			renderTargets.gbuffer->QueueForDeletion();
		}
		if (renderTargets.lighting) {
			renderTargets.lighting->QueueForDeletion();
		}
		if (renderTargets.skybox) {
			renderTargets.skybox->QueueForDeletion();
		}
		if (renderTargets.gizmo) {
			renderTargets.gizmo->QueueForDeletion();
		}
		if (renderTargets.composite) {
			renderTargets.composite->QueueForDeletion();
		}
		if (renderTargets.shadow) {
			renderTargets.shadow->QueueForDeletion();
		}

		for (auto *fence : renderFences) {
			if (fence != VK_NULL_HANDLE) {
				vkDestroyFence(device.GetDevice(), fence, nullptr);
			}
		}
		renderFences.clear();

		commandBuffers.clear();
	}

	[[nodiscard]] bool IsValid() const {
		return renderTargets.gbuffer && renderFences.size() == SharedConstants::MAX_FRAMES_IN_FLIGHT &&
			   renderTargets.lighting && renderTargets.gizmo && renderTargets.composite && renderTargets.shadow &&
			   !commandBuffers.empty();
	}

	void UploadCameraData(const CameraGPUData &data, uint32 frameIndex) {
		cameraUBOs[frameIndex]->Write(&data, sizeof(CameraGPUData));
	}

  private:
	void CreateCameraResources(Device &device, RenderingPipeline::DescriptorSetLayout &layout) {
		cameraUBOs.resize(SharedConstants::MAX_FRAMES_IN_FLIGHT);
		cameraDescriptors.resize(SharedConstants::MAX_FRAMES_IN_FLIGHT);

		for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; i++) {
			cameraUBOs[i] = CreateUnique<Resources::Buffer>(
				device, "GlobalCameraUBO_" + std::to_string(i), sizeof(CameraGPUData), 1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); // ask for coherent

			cameraUBOs[i]->Map();

			if (RenderingPipeline::DescriptorAllocator::Allocate(layout.GetDescriptorSetLayout(),
																 cameraDescriptors[i])) {
				RenderingPipeline::DescriptorWriter writer{ layout,
															*RenderingPipeline::DescriptorAllocator::GetSharedPool() };
				auto bufferInfo = cameraUBOs[i]->DescriptorInfo();
				writer.WriteBuffer(0, &bufferInfo);
				writer.Overwrite(cameraDescriptors[i]);
			}
		}
	}
	void CreateRenderTargets(Device &device, uint32 width, uint32 height) {
		renderTargets.gbuffer =
			Aquila::Graphics::RenderingPipeline::RenderTarget::Builder(device, "GBuffer_" + scene->GetSceneName())
				.WithSize(width, height)
				.AddColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT,
									VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, "Position")
				.AddColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT,
									VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, "Normal")
				.AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM,
									VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, "Albedo")
				.WithDepthFormat(VK_FORMAT_D32_SFLOAT)
				.Build();

		renderTargets.lighting =
			Aquila::Graphics::RenderingPipeline::RenderTarget::Builder(device, "Lighting_" + scene->GetSceneName())
				.WithSize(width, height)
				.WithColorFormat(VK_FORMAT_R8G8B8A8_UNORM)
				.WithType(Aquila::Graphics::RenderingPipeline::RenderTarget::Type::COLOR_ONLY)
				.Build();

		renderTargets.skybox =
			Aquila::Graphics::RenderingPipeline::RenderTarget::Builder(device, "Skybox_" + scene->GetSceneName())
				.WithSize(width, height)
				.WithColorFormat(VK_FORMAT_R32G32B32A32_SFLOAT)
				.WithType(Aquila::Graphics::RenderingPipeline::RenderTarget::Type::COLOR_ONLY)
				.Build();

		renderTargets.gizmo =
			Aquila::Graphics::RenderingPipeline::RenderTarget::Builder(device, "Gizmo_" + scene->GetSceneName())
				.WithSize(width, height)
				.WithColorFormat(VK_FORMAT_R8G8B8A8_UNORM)
				.WithType(Aquila::Graphics::RenderingPipeline::RenderTarget::Type::COLOR_ONLY)
				.Build();

		renderTargets.composite =
			Aquila::Graphics::RenderingPipeline::RenderTarget::Builder(device, "Composite_" + scene->GetSceneName())
				.WithSize(width, height)
				.WithColorFormat(VK_FORMAT_R8G8B8A8_UNORM)
				.WithType(Aquila::Graphics::RenderingPipeline::RenderTarget::Type::COLOR_ONLY)
				.Build();

		const uint32 shadowSize =
			2048; // TODO: stop using magic numbers, centralize everything in a RenderSettings struct or whatever!!!
		renderTargets.shadow =
			Aquila::Graphics::RenderingPipeline::RenderTarget::Builder(device, "Shadow_" + scene->GetSceneName())
				.WithSize(shadowSize, shadowSize)
				.WithDepthFormat(VK_FORMAT_D32_SFLOAT)
				.WithType(Aquila::Graphics::RenderingPipeline::RenderTarget::Type::DEPTH_ONLY)
				.Build();
	}
};

/**
 * @brief High-level renderer using dynamic rendering
 * Much simpler than before - no render passes or framebuffers!
 */
class Renderer {
  public:
	Renderer(Device &device, Core::Window &window, Graphics::Material::MaterialSystem &materialSystem, Camera &cam);
	~Renderer();

	/**
     * @brief Render a scene with a specific extent
     */
	void RenderWithExtent(SceneManagement::Scene *scene, uint32 width, uint32 height);

	/**
     * @brief Get the final output texture for a scene
     */
	Ref<Resources::Texture2D> GetSceneOutputTexture(SceneManagement::Scene *scene) const;

	/**
     * @brief Clean up all resources for a scene
     */
	void CleanupSceneContext(SceneManagement::Scene *scene);

	VkCommandBuffer BeginFrame();
	void EndFrame();
	bool IsPreviousFrameComplete() const;

	void Resize(VkExtent2D newExtent);
	bool InvalidatePasses();
	bool IsResizePending() const { return m_Resized; }
	bool ValidateRenderSetup() const;

	[[nodiscard]] uint32 GetCurrentFrame() const { return m_CurrentFrameID; }
	[[nodiscard]] uint32 GetCurrentImageIndex() const { return m_CurrentImageID; }
	Swapchain *GetSwapchain() const { return m_Swapchain.get(); }
	Material::MaterialSystem &GetMaterialSystem() const { return m_MaterialSystem; }

	template <typename T> Ref<T> GetRenderingSystem() const {
		if constexpr (std::is_same_v<T, Rendering::Systems::GizmoRenderSystem>) {
			return m_GizmoRendering;
		}
		if constexpr (std::is_same_v<T, Rendering::Systems::DeferredRenderSystem>) {
			return m_DeferredRendering;
		}
		if constexpr (std::is_same_v<T, Rendering::Systems::LightingRenderSystem>) {
			return m_LightingRendering;
		}
		if constexpr (std::is_same_v<T, Rendering::Systems::CompositeRenderingSystem>) {
			return m_CompositeRendering;
		}
		if constexpr (std::is_same_v<T, Rendering::Systems::ShadowRenderSystem>) {
			return m_ShadowRendering;
		}
		return nullptr;
	}

	void BeginSceneRendering();
	void DispatchCompute();
	void RenderSceneBatched(SceneManagement::Scene *scene, Camera &camera, uint32 width, uint32 height);
	void EndSceneRendering();
	void OnEvent(Events::Event &event);

  private:
	void ExecuteRenderingForScene(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec,
								  const SceneRenderContext &context);

	SceneRenderContext &GetOrCreateSceneContext(SceneManagement::Scene *scene, const VkExtent2D &extent);

	void RenderShadowPass(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec);
	void RenderGBufferPass(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec,
						   const SceneRenderContext &context);
	void RenderLightingPass(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec,
							const SceneRenderContext &context);
	void RenderSkyboxPass(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec,
						  const SceneRenderContext &context);
	void RenderGizmoPass(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec,
						 const SceneRenderContext &context);
	void RenderCompositePass(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec,
							 const SceneRenderContext &context);

	void BindSceneStaticDescriptors(SceneRenderContext &context, SceneManagement::Scene *scene);
	void Initialize(uint32 width, uint32 height);
	void InvalidateSwapchain();
	void SetupCommandBuffers();
	void SetupSynchronization() const;
	void CreateRenderingSystems();
	std::vector<Rendering::Systems::RenderingSystemBase *> GetRenderingSystems() const {
		return { m_DeferredRendering.get(),	 m_LightingRendering.get(), m_GizmoRendering.get(),
				 m_CompositeRendering.get(), m_ShadowRendering.get(),	m_SkyboxRendering.get() };
	}

	Device &m_Device;
	Core::Window &m_Window;
	Camera &m_EditorCamera;

	Material::MaterialSystem &m_MaterialSystem;

	Unique<CommandBufferPool> m_CommandPool;

	struct SceneBatchState {
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkFence> fences;
		std::vector<SceneManagement::Scene *> scenes;
		std::vector<uint32> frameIndices;
		bool batchActive = false;

		void Reset() {
			commandBuffers.clear();
			fences.clear();
			scenes.clear();
			frameIndices.clear();
			batchActive = false;
		}
	};
	SceneBatchState m_SceneBatch;
	Unique<SynchronizationManager> m_SynchronizationManager;

	std::vector<Unique<CommandBuffer>> m_PresentCommandBuffers;
	std::vector<Unique<CommandBuffer>> m_ComputeCommandBuffers;

	Unique<Swapchain> m_Swapchain;

	Ref<Rendering::Systems::DeferredRenderSystem> m_DeferredRendering;
	Ref<Rendering::Systems::LightingRenderSystem> m_LightingRendering;
	Ref<Rendering::Systems::CompositeRenderingSystem> m_CompositeRendering;
	Ref<Rendering::Systems::GizmoRenderSystem> m_GizmoRendering;
	Ref<Rendering::Systems::ShadowRenderSystem> m_ShadowRendering;
	Ref<Rendering::Systems::SkyboxRenderSystem> m_SkyboxRendering;

	std::unordered_map<SceneManagement::Scene *, SceneRenderContext> m_SceneContexts;

	Ref<RenderingPipeline::DescriptorSetLayout> m_GlobalCameraLayout = nullptr;

	VkExtent2D m_Extent{};
	uint32 m_CurrentFrameID = 0;
	uint32 m_CurrentImageID = 0;
	bool m_FrameStarted = false;
	bool m_Resized = false;
};

} // namespace Aquila::Graphics

#endif // AQUILA_RENDERER_H
