#pragma once

#include "Aquila/Core/Defines.h"
#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Graphics/Pipeline/Descriptor.h"
#include "Aquila/Graphics/Pipeline/DescriptorAllocator.h"
#include "Aquila/Graphics/Pipeline/Pipeline.h"
#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"
#include "Aquila/Graphics/Resources/Texture2D.h"
#include "Aquila/Rendering/Camera.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Graphics/Pipeline/RenderSettings.h"
#include "Aquila/Scene/Entity.h"

namespace Aquila::Rendering::Systems {

using namespace Aquila::Graphics;
using namespace Aquila::Graphics::RenderingPipeline;
using namespace Aquila::Graphics::Resources;
using namespace Aquila::Graphics::Helpers;

struct FrameSpec {
	SceneManagement::Scene *scene = nullptr;

	const SceneManagement::Components::CameraComponent *cameraComponent = nullptr;
	const SceneManagement::Components::TransformComponent *cameraTransform = nullptr;

	Camera *standaloneCamera = nullptr;

	uint32 frameIndex = 0;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkDescriptorSet cameraDescriptorSet = VK_NULL_HANDLE;

	bool renderSkybox = false;

	// Helper to get projection matrix
	[[nodiscard]] mat4 GetProjectionMatrix() const {
		if (standaloneCamera != nullptr) {
			return standaloneCamera->GetProjection();
		}
		if (cameraComponent != nullptr) {
			return cameraComponent->GetProjectionMatrix();
		}
		return { 1.0F };
	}

	[[nodiscard]] f32 GetNearPlane() const {
		if (standaloneCamera != nullptr) {
			return standaloneCamera->GetNearPlane();
		}
		if (cameraComponent != nullptr) {
			return cameraComponent->GetNearPlane();
		}
		return 1.0F;
	}

	[[nodiscard]] f32 GetFarPlane() const {
		if (standaloneCamera != nullptr) {
			return standaloneCamera->GetFarPlane();
		}
		if (cameraComponent != nullptr) {
			return cameraComponent->GetFarPlane();
		}
		return 1.0F;
	}

	// Helper to get view matrix
	[[nodiscard]] mat4 GetViewMatrix() const {
		if (standaloneCamera != nullptr) {
			return standaloneCamera->GetView();
		}
		if ((cameraComponent != nullptr) && (cameraTransform != nullptr)) {
			return cameraComponent->GetViewMatrix(cameraTransform->GetWorldPosition(),
												  cameraTransform->GetWorldRotation());
		}
		return { 1.0F };
	}

	// Helper to get inverse view matrix
	[[nodiscard]] mat4 GetInverseViewMatrix() const {
		if (standaloneCamera != nullptr) {
			return standaloneCamera->GetInverseView();
		}
		if ((cameraComponent != nullptr) && (cameraTransform != nullptr)) {
			return cameraComponent->GetInverseViewMatrix(cameraTransform->GetWorldPosition(),
														 cameraTransform->GetWorldRotation());
		}
		return { 1.0F };
	}

	[[nodiscard]] bool HasValidCamera() const {
		return standaloneCamera != nullptr || (cameraComponent != nullptr && cameraTransform != nullptr);
	}
};

// Per-entity render state tracking
struct EntityRenderState {
	uint32 lastMeshVersion = 0;
	Ref<Graphics::Resources::Mesh> lastMesh = nullptr;
	std::vector<Ref<Graphics::Material::Material>> lastMaterials;

	uint32 lastFrameRendered = 0;
	bool isCastingShadows = true;
	bool needsRebind = false;
};

struct SceneCache {
	// Entity lists
	std::vector<SceneManagement::Entity> renderableEntities;
	std::vector<SceneManagement::Entity> lightEntities;
	std::vector<SceneManagement::Entity> shadowCasters;
	// Material groupings
	struct MaterialBatch {
		Ref<Graphics::Material::Material> material;
		std::vector<SceneManagement::Entity> entities;
	};
	std::vector<MaterialBatch> materialGroups;

	// Dirty flags
	bool entitiesDirty = true;
	bool materialGroupsDirty = true;

	// Change tracking
	uint32 lastEntityCount = 0;
	uint32 lastLightCount = 0;
	int lightsDirtyFramesRemaining = 0;
};

// Per-scene descriptor data
struct SceneDescriptorData {
	// Descriptor sets: [setIndex][frameIndex]
	std::vector<std::vector<VkDescriptorSet>> descriptorSets;

	std::unordered_map<uint32, std::unordered_map<uint32, VkDescriptorBufferInfo>> pendingUniformBuffers;
	std::unordered_map<uint32, std::unordered_map<uint32, VkDescriptorImageInfo>> pendingTextures;
	std::unordered_map<uint32, std::unordered_map<uint32, VkDescriptorImageInfo>> pendingTextureArrays;

	std::unordered_map<UUID, EntityRenderState> entityStates;

	std::array<bool, 2> needsUpdate{};

	SceneDescriptorData() { needsUpdate.fill(true); }
};

class RenderingSystemBase {
  public:
	explicit RenderingSystemBase(Device &device);
	virtual ~RenderingSystemBase();

	AQUILA_NONCOPYABLE(RenderingSystemBase);
	AQUILA_NONMOVEABLE(RenderingSystemBase);

	virtual void CreatePipeline(const PipelineRenderingFormats &renderingFormats) = 0;
	virtual void CreatePipelineLayout() = 0;
	virtual void OnUpdate(const FrameSpec &frameSpec) = 0;
	virtual void OnRender(const FrameSpec &frameSpec) = 0;
	virtual void OnEvent(Events::Event &event) = 0;

	// PER-SCENE DESCRIPTOR MANAGEMENT
	SceneDescriptorData &GetOrCreateSceneData(SceneManagement::Scene *scene);
	void AllocateDescriptorSetsForScene(SceneManagement::Scene *scene);

	void BindUniformBuffer(SceneManagement::Scene *scene, uint32 set, uint32 binding,
						   const VkDescriptorBufferInfo *bufferInfo);
	void BindTexture(SceneManagement::Scene *scene, uint32 set, uint32 binding, const VkDescriptorImageInfo *imageInfo);
	void BindTexture(SceneManagement::Scene *scene, uint32 set, uint32 binding, Texture2D *texture);
	void BindTextureArray(SceneManagement::Scene *scene, uint32 set, uint32 binding,
						  const VkDescriptorImageInfo *imageInfo);

	void UpdateDescriptorsForScene(const FrameSpec &frameSpec);
	VkDescriptorSet GetSceneDescriptorSet(SceneManagement::Scene *scene, uint32 setIndex, uint32 frameIndex) const;
	void MarkSceneDescriptorsDirty(SceneManagement::Scene *scene);
	void CleanupSceneDescriptors(SceneManagement::Scene *scene);

	// ENTITY STATE TRACKING - this might become obsolete soon !
	EntityRenderState &GetOrCreateEntityState(SceneManagement::Scene *scene, UUID entityID);
	bool HasMeshChanged(SceneManagement::Scene *scene, Utils::UUID entityID,
						const SceneManagement::Components::MeshComponent &meshComp);
	bool HasEntityChanged(SceneManagement::Scene *scene, UUID entityID,
						  const SceneManagement::Components::MeshComponent &meshComp, EntityRenderState &state);
	bool HasMaterialsChanged(const SceneManagement::Components::MeshComponent &mesh, const EntityRenderState &state);
	void UpdateEntityState(SceneManagement::Scene *scene, UUID entityID,
						   const SceneManagement::Components::MeshComponent &meshComp);

	// GETTERS
	VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
	const std::vector<Ref<DescriptorSetLayout>> &GetLayouts() const { return m_Layouts; }

  protected:
	Device &device;

	std::vector<Ref<DescriptorSetLayout>> m_Layouts;
	std::unordered_map<SceneManagement::Scene *, SceneDescriptorData> m_SceneDescriptors;
	std::unordered_map<SceneManagement::Scene *, SceneCache> m_SceneCaches;

	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	Unique<Pipeline> m_Pipeline = nullptr;
};

} // namespace Aquila::Rendering::Systems
