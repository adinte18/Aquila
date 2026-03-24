#include "Aquila/Graphics/Core/Renderer.h"

#include "Aquila/Core/Defines.h"
#include "Aquila/Events/Event.h"
#include "Aquila/Events/SceneEvent.h"
#include "Aquila/Events/WindowEvent.h"
#include "Aquila/Graphics/Core/Swapchain.h"
#include "Aquila/Graphics/Pipeline/Descriptor.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"
#include "Aquila/Foundation/Profiler.h"

namespace Aquila::Graphics {

using namespace Aquila::Graphics::Helpers;

Renderer::Renderer(Device &device, Core::Window &window, Graphics::Material::MaterialSystem &materialSystem,
				   Camera &cam)
	: m_Device(device), m_Window(window), m_EditorCamera(cam), m_MaterialSystem(materialSystem) {
	Initialize(m_Window.GetWidth(), m_Window.GetHeight());
}

Renderer::~Renderer() {
	AQUILA_LOG_DEBUG("Destroying renderer!");
	m_Device.Wait();

	auto systems = GetRenderingSystems();

	for (auto &context : m_SceneContexts | std::views::values) {
		context.QueueForDeletion(m_Device, systems);
	}
	m_SceneContexts.clear();

	m_PresentCommandBuffers.clear();

	m_CompositeRendering.reset();
	m_GizmoRendering.reset();
	m_LightingRendering.reset();
	m_DeferredRendering.reset();
	m_ShadowRendering.reset();
	m_SkyboxRendering.reset();
	m_Swapchain.reset();
	m_CommandPool.reset();
	m_SynchronizationManager.reset();

	AQUILA_LOG_DEBUG("Renderer destroyed successfully");
}

bool Renderer::ValidateRenderSetup() const {
	bool valid = true;

	if (!m_ShadowRendering) {
		AQUILA_LOG_ERROR("Shadow rendering system not initialized");
		valid = false;
	}

	if (!m_DeferredRendering) {
		AQUILA_LOG_ERROR("Deferred rendering system not initialized");
		valid = false;
	}

	if (!m_LightingRendering) {
		AQUILA_LOG_ERROR("Lighting rendering system not initialized");
		valid = false;
	}

	if (!m_GizmoRendering) {
		AQUILA_LOG_ERROR("Gizmo rendering system not initialized");
		valid = false;
	}

	if (!m_CompositeRendering) {
		AQUILA_LOG_ERROR("Composite rendering system not initialized");
		valid = false;
	}

	return valid;
}

void Renderer::Initialize(const uint32 width, const uint32 height) {
	m_Extent = { .width = width, .height = height };

	m_CommandPool = CreateUnique<CommandBufferPool>(m_Device, SharedConstants::MAX_FRAMES_IN_FLIGHT);
	if (m_CommandPool == nullptr) {
		AQUILA_LOG_CRITICAL("Could not initialize command pool. Exiting");
		abort();
	}

	m_SynchronizationManager = CreateUnique<SynchronizationManager>(m_Device, SharedConstants::MAX_FRAMES_IN_FLIGHT);
	if (m_SynchronizationManager == nullptr) {
		AQUILA_LOG_CRITICAL("Could not initialize synchronization manager. Exiting");
		abort();
	}

	m_Swapchain = CreateUnique<Swapchain>(m_Device, m_Extent);
	if (m_Swapchain == nullptr) {
		AQUILA_LOG_CRITICAL("Could not initialize swapchain. Exiting");

		abort();
	}

	SetupCommandBuffers();
	SetupSynchronization();

	CreateRenderingSystems();

	AQUILA_LOG_INFO("Renderer initialized with dynamic rendering");
}

void Renderer::CreateRenderingSystems() {
	AQUILA_ASSERT(m_Device.GetDevice() != nullptr, "Device should exist");

	m_GlobalCameraLayout = RenderingPipeline::DescriptorSetLayout::Builder(m_Device)
							   .AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
							   .Build();

	std::vector<Ref<RenderingPipeline::DescriptorSetLayout>> GlobalLayouts = { m_GlobalCameraLayout };

	m_MaterialSystem.Initialize(m_GlobalCameraLayout.get());

	const Ref<RenderingPipeline::DescriptorSetLayout> CascadesLayout =
		RenderingPipeline::DescriptorSetLayout::Builder(m_Device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.Build();

	std::vector<Ref<RenderingPipeline::DescriptorSetLayout>> CascadesLayouts = { CascadesLayout };

	m_ShadowRendering = CreateRef<Rendering::Systems::ShadowRenderSystem>(m_Device, CascadesLayouts);
	if (!m_ShadowRendering) {
		throw std::runtime_error("Failed to create Shadow rendering system");
	}
	AQUILA_LOG_INFO("Shadow rendering system initialized (dynamic rendering)");

	m_DeferredRendering =
		CreateRef<Rendering::Systems::DeferredRenderSystem>(m_Device, GlobalLayouts, m_MaterialSystem);
	if (!m_DeferredRendering) {
		throw std::runtime_error("Failed to create Deferred rendering system");
	}
	AQUILA_LOG_INFO("Deferred rendering system initialized (dynamic rendering)");

	const Ref<RenderingPipeline::DescriptorSetLayout> LightsEnvLayout =
		RenderingPipeline::DescriptorSetLayout::Builder(m_Device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // LightsData
			.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // EnvironmentData
			.Build();

	const Ref<RenderingPipeline::DescriptorSetLayout> GBufferLayout =
		RenderingPipeline::DescriptorSetLayout::Builder(m_Device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // position
			.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // normal
			.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // albedo
			.Build();

	// 4 samplers should be a texture array lol
	const Ref<RenderingPipeline::DescriptorSetLayout> ShadowLayout =
		RenderingPipeline::DescriptorSetLayout::Builder(m_Device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build();

	// Correct order: set 0, 1, 2, 3
	std::vector<Ref<RenderingPipeline::DescriptorSetLayout>> LightLayouts = {
		m_GlobalCameraLayout, // set 0
		LightsEnvLayout,	  // set 1
		GBufferLayout,		  // set 2
		ShadowLayout		  // set 3
	};

	m_LightingRendering = CreateRef<Rendering::Systems::LightingRenderSystem>(m_Device, LightLayouts);
	if (!m_LightingRendering) {
		throw std::runtime_error("Failed to create Lighting rendering system");
	}
	AQUILA_LOG_INFO("Lighting rendering system initialized (dynamic rendering)");

	const Ref<RenderingPipeline::DescriptorSetLayout> SkyboxTextureLayout =
		RenderingPipeline::DescriptorSetLayout::Builder(m_Device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build();

	std::vector<Ref<RenderingPipeline::DescriptorSetLayout>> SkyboxLayouts = { m_GlobalCameraLayout,
																			   SkyboxTextureLayout };

	m_SkyboxRendering = CreateRef<Rendering::Systems::SkyboxRenderSystem>(m_Device, SkyboxLayouts);

	if (!m_SkyboxRendering) {
		throw std::runtime_error("Failed to create Skybox rendering system");
	}
	AQUILA_LOG_INFO("Skybox rendering system initialized (dynamic rendering)");

	m_GizmoRendering = CreateRef<Rendering::Systems::GizmoRenderSystem>(m_Device, GlobalLayouts);
	if (!m_GizmoRendering) {
		throw std::runtime_error("Failed to create Gizmo rendering system");
	}
	AQUILA_LOG_INFO("Gizmo rendering system initialized (dynamic rendering)");

	const Ref<RenderingPipeline::DescriptorSetLayout> CompositeLayout =
		RenderingPipeline::DescriptorSetLayout::Builder(m_Device)
			.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build();

	std::vector<Ref<RenderingPipeline::DescriptorSetLayout>> CompositeLayouts = { CompositeLayout };

	m_CompositeRendering = CreateRef<Rendering::Systems::CompositeRenderingSystem>(m_Device, CompositeLayouts);
	if (!m_CompositeRendering) {
		throw std::runtime_error("Failed to create Composite rendering system");
	}
	AQUILA_LOG_INFO("Composite rendering system initialized (dynamic rendering)");
}

void Renderer::SetupCommandBuffers() {
	m_PresentCommandBuffers.reserve(SharedConstants::MAX_FRAMES_IN_FLIGHT);
	m_ComputeCommandBuffers.reserve(SharedConstants::MAX_FRAMES_IN_FLIGHT);

	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		m_PresentCommandBuffers.push_back(
			m_CommandPool->GetFrameCommandBuffer(CommandBufferType::PRESENT, i, "PresentCommandBuffer"));
	}

	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		m_ComputeCommandBuffers.push_back(
			m_CommandPool->GetFrameCommandBuffer(CommandBufferType::COMPUTE, i, "ComputeCommandBuffer"));
	}
}

void Renderer::SetupSynchronization() const {
	m_SynchronizationManager->CreateSemaphore("ImageAvailable");
	m_SynchronizationManager->CreateSemaphore("RenderFinished");
	m_SynchronizationManager->CreateFence("InFlight", true);

	m_SynchronizationManager->CreateSemaphore("ComputeFinished");
	m_SynchronizationManager->CreateFence("ComputeInFlight", true);
}

void Renderer::BeginSceneRendering() {
	AQUILA_ASSERT(!m_SceneBatch.batchActive, "Scene batch already active!");
	m_SceneBatch.Reset();
	m_SceneBatch.batchActive = true;
}

void Renderer::DispatchCompute() {
	auto &computeCmd = m_ComputeCommandBuffers[m_CurrentFrameID];
	computeCmd->Reset();
	computeCmd->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	//! IMPORTANT: Compute work goes here

	computeCmd->End();
}

void Renderer::RenderSceneBatched(SceneManagement::Scene *scene, Camera &camera, uint32 width, uint32 height) {
	AQUILA_ASSERT(m_SceneBatch.batchActive, "Must call BeginSceneRendering() first!");

	if ((scene == nullptr) || width == 0 || height == 0) {
		AQUILA_LOG_WARNING("Invalid parameters for RenderSceneBatched");
		return;
	}

	auto &context = GetOrCreateSceneContext(scene, { width, height });
	if (!context.IsValid()) {
		AQUILA_LOG_ERROR("Invalid scene context for '{}'", scene->GetSceneName());
		return;
	}

	// Build FrameSpec
	Rendering::Systems::FrameSpec frameSpec{};
	frameSpec.scene = scene;
	frameSpec.standaloneCamera = &camera;
	frameSpec.frameIndex = context.currentFrameIndex;

	{
		PROFILE_SCOPE("	UploadCameraUBO");

		SceneRenderContext::CameraGPUData camData{};
		camData.view = frameSpec.GetViewMatrix();
		camData.projection = frameSpec.GetProjectionMatrix();
		camData.viewProjection = camData.projection * camData.view;
		camData.inverseView = frameSpec.GetInverseViewMatrix();
		camData.inverseProjection = glm::inverse(camData.projection);
		camData.cameraPosition = glm::vec3(camData.inverseView[3]);
		camData.nearPlane = frameSpec.GetNearPlane();
		camData.farPlane = frameSpec.GetFarPlane();

		context.UploadCameraData(camData, context.currentFrameIndex);

		frameSpec.cameraDescriptorSet = context.cameraDescriptors[context.currentFrameIndex];
	}

	{
		PROFILE_SCOPE("	CPU Preparation");

		scene->UpdateTransformHierarchy();

		if (m_ShadowRendering) {
			m_ShadowRendering->OnUpdate(frameSpec);
		}
		if (m_DeferredRendering) {
			m_DeferredRendering->OnUpdate(frameSpec);
		}

		if (m_SkyboxRendering) {
			bool shouldRenderSkybox = false;
			frameSpec.scene->GetEntityManager()->ForEach<SceneManagement::Components::SkyLightComponent>(
				[&](SceneManagement::Components::SkyLightComponent &skyLight) {
					if (skyLight.ShouldRenderSkybox() && skyLight.GetHDRTexture()) {
						m_SkyboxRendering->SetSkyboxTexture(skyLight.GetHDRTexture());
						shouldRenderSkybox = true;
					}
				});
			frameSpec.renderSkybox = shouldRenderSkybox;
			if (shouldRenderSkybox) {
				m_SkyboxRendering->OnUpdate(frameSpec);
			}
		}

		if (m_LightingRendering) {
			// shadow UBO changes every frame (cascade splits recalculated)
			// so this one stays here — it's not static
			auto shadowUniformInfo = m_ShadowRendering->GetShadowUniformBufferInfo(frameSpec.frameIndex);
			m_LightingRendering->BindUniformBuffer(scene, 3, 4, &shadowUniformInfo);
			m_LightingRendering->OnUpdate(frameSpec);
		}

		if (m_GizmoRendering) {
			m_GizmoRendering->OnUpdate(frameSpec);
		}
	}

	CommandBuffer *cmd = context.GetCurrentCommandBufferSafe(m_Device);
	cmd->Reset();
	cmd->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	frameSpec.commandBuffer = cmd->GetHandle();

	// !! IMPORTANT: This space is reserved for rendering
	ExecuteRenderingForScene(cmd->GetHandle(), frameSpec, context);

	cmd->End();

	m_SceneBatch.commandBuffers.push_back(cmd->GetHandle());
	m_SceneBatch.fences.push_back(context.GetCurrentFence());
	m_SceneBatch.scenes.push_back(scene);
	m_SceneBatch.frameIndices.push_back(context.currentFrameIndex);

	context.AdvanceFrame();
}

void Renderer::BindSceneStaticDescriptors(SceneRenderContext &context, SceneManagement::Scene *scene) {
	if (m_LightingRendering) {
		const auto &gbufferAttachments = context.renderTargets.gbuffer->GetColorAttachments();
		for (size_t i = 0; i < gbufferAttachments.size(); i++) {
			auto info = gbufferAttachments[i]->GetDescriptorImageInfo();
			m_LightingRendering->BindTexture(scene, 2, static_cast<uint32>(i), &info);
		}

		for (size_t i = 0; i < Rendering::Systems::ShadowRenderSystem::CASCADE_COUNT; i++) {
			const auto &shadowDepth = m_ShadowRendering->GetCascadeDepthAttachment(i);
			auto info = shadowDepth->GetDescriptorImageInfo();
			m_LightingRendering->BindTexture(scene, 3, static_cast<uint32>(i), &info);
		}
	}

	if (m_CompositeRendering) {
		auto litDescriptor = context.renderTargets.lighting->GetColorAttachment()->GetDescriptorImageInfo();
		m_CompositeRendering->BindTexture(scene, 0, 0, &litDescriptor);

		auto skyboxDescriptor = context.renderTargets.skybox->GetColorAttachment()->GetDescriptorImageInfo();
		m_CompositeRendering->BindTexture(scene, 0, 1, &skyboxDescriptor);

		auto gizmoDescriptor = context.renderTargets.gizmo->GetColorAttachment()->GetDescriptorImageInfo();
		m_CompositeRendering->BindTexture(scene, 0, 2, &gizmoDescriptor);
	}

	for (uint32 frame = 0; frame < SharedConstants::MAX_FRAMES_IN_FLIGHT; frame++) {
		Rendering::Systems::FrameSpec flushSpec{};
		flushSpec.scene = scene;
		flushSpec.frameIndex = frame;

		if (m_CompositeRendering) {
			m_CompositeRendering->UpdateDescriptorsForScene(flushSpec);
		}
		if (m_LightingRendering) {
			m_LightingRendering->UpdateDescriptorsForScene(flushSpec);
		}
	}
}

void Renderer::EndSceneRendering() {
	PROFILE_SCOPE("EndSceneRendering");
	AQUILA_ASSERT(m_SceneBatch.batchActive, "Scene batch not active!");

	if (m_SceneBatch.commandBuffers.empty()) {
		m_SceneBatch.batchActive = false;
		return;
	}

	m_SynchronizationManager->WaitForFence("ComputeInFlight", m_CurrentFrameID);
	m_SynchronizationManager->ResetFence("ComputeInFlight", m_CurrentFrameID);

	auto *computeCmd = m_ComputeCommandBuffers[m_CurrentFrameID]->GetHandle();
	VkSemaphore computeFinished = m_SynchronizationManager->GetSemaphore("ComputeFinished", m_CurrentFrameID);
	VkFence computeFence = m_SynchronizationManager->GetFence("ComputeInFlight", m_CurrentFrameID);

	VkSubmitInfo computeSubmit{};
	computeSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	computeSubmit.commandBufferCount = 1;
	computeSubmit.pCommandBuffers = &computeCmd;
	computeSubmit.signalSemaphoreCount = 1;
	computeSubmit.pSignalSemaphores = &computeFinished;

	m_Device.SubmitToComputeQueue(&computeSubmit, computeFence);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

	for (size_t i = 0; i < m_SceneBatch.commandBuffers.size(); i++) {
		VkSubmitInfo graphicsSubmit{};
		graphicsSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		graphicsSubmit.waitSemaphoreCount = 1;
		graphicsSubmit.pWaitSemaphores = &computeFinished;
		graphicsSubmit.pWaitDstStageMask = &waitStage;
		graphicsSubmit.commandBufferCount = 1;
		graphicsSubmit.pCommandBuffers = &m_SceneBatch.commandBuffers[i];
		graphicsSubmit.signalSemaphoreCount = 0;

		m_Device.SubmitToGraphicsQueue(&graphicsSubmit, m_SceneBatch.fences[i]);
	}

	m_SceneBatch.Reset();
}

VkCommandBuffer Renderer::BeginFrame() {
	AQUILA_ASSERT(!m_FrameStarted, "Frame already started");

	m_SynchronizationManager->WaitForFence("InFlight", m_CurrentFrameID);
	m_SynchronizationManager->ResetFence("InFlight", m_CurrentFrameID);

	VkSemaphore imageAvailable = m_SynchronizationManager->GetSemaphore("ImageAvailable", m_CurrentFrameID);

	const auto result = m_Swapchain->AcquireNextImage(&m_CurrentImageID, imageAvailable);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		InvalidateSwapchain();
		return VK_NULL_HANDLE;
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swapchain image");
	}

	m_FrameStarted = true;

	auto &presentCmd = m_PresentCommandBuffers[m_CurrentFrameID];
	presentCmd->Reset();
	presentCmd->Begin();

	VkImage currentImage = m_Swapchain->GetCurrentImage(m_CurrentImageID);
	bool firstFrame = !m_Swapchain->IsImageInitialized(m_CurrentImageID);

	DynamicRendering::TransitionImages(m_Device, presentCmd->GetHandle(), { currentImage },
									   m_Swapchain->GetImageFormat(), VK_NULL_HANDLE, VK_FORMAT_UNDEFINED,
									   firstFrame ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
									   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	if (firstFrame) {
		m_Swapchain->MarkImageInitialized(m_CurrentImageID);
	}

	return presentCmd->GetHandle();
}

void Renderer::EndFrame() {
	AQUILA_ASSERT(m_FrameStarted, "Frame not started");

	const auto &presentCmd = m_PresentCommandBuffers[m_CurrentFrameID];
	presentCmd->End();

	VkFence inFlightFence = m_SynchronizationManager->GetFence("InFlight", m_CurrentFrameID);
	VkFence computeFence = m_SynchronizationManager->GetFence("ComputeInFlight", m_CurrentFrameID);
	VkSemaphore imageAvailable = m_SynchronizationManager->GetSemaphore("ImageAvailable", m_CurrentFrameID);
	VkSemaphore renderFinished = m_SynchronizationManager->GetSemaphore("RenderFinished", m_CurrentFrameID);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailable;
	submitInfo.pWaitDstStageMask = &waitStage;

	VkCommandBuffer presentHandle = presentCmd->GetHandle();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &presentHandle;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinished;

	m_Device.SubmitToGraphicsQueue(&submitInfo, inFlightFence);

	const auto result = m_Swapchain->PresentImage(&m_CurrentImageID, renderFinished);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window.IsWindowResized()) {
		m_Window.ResetResizedFlag();
		InvalidateSwapchain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swapchain image");
	}

	m_FrameStarted = false;
	m_CurrentFrameID = (m_CurrentFrameID + 1) % SharedConstants::MAX_FRAMES_IN_FLIGHT;
}

bool Renderer::IsPreviousFrameComplete() const {
	const uint32 previousFrame =
		(m_CurrentFrameID + SharedConstants::MAX_FRAMES_IN_FLIGHT - 1) % SharedConstants::MAX_FRAMES_IN_FLIGHT;
	return m_SynchronizationManager->IsFenceSignaled("InFlight", previousFrame);
}

SceneRenderContext &Renderer::GetOrCreateSceneContext(SceneManagement::Scene *scene, const VkExtent2D &extent) {
	if (extent.width == 0 || extent.height == 0) {
		AQUILA_LOG_CRITICAL("Invalid extent for scene context");
		throw std::runtime_error("Invalid extent for scene context");
	}

	auto iterator = m_SceneContexts.find(scene);
	if (iterator != m_SceneContexts.end()) {
		if (iterator->second.extent.width != extent.width || iterator->second.extent.height != extent.height) {
			Resize(extent);
		}
		return iterator->second;
	}

	AQUILA_LOG_INFO("Creating context for scene '{}' ({}x{})", scene->GetSceneName(), extent.width, extent.height);

	SceneRenderContext context;
	auto systems = GetRenderingSystems();

	context.Create(m_Device, scene, extent, m_CommandPool.get(), m_GlobalCameraLayout, systems);

	BindSceneStaticDescriptors(context, scene);

	auto [iter, inserted] = m_SceneContexts.emplace(scene, std::move(context));
	return iter->second;
}
void Renderer::CleanupSceneContext(SceneManagement::Scene *scene) {
	auto iterator = m_SceneContexts.find(scene);
	if (iterator == m_SceneContexts.end()) {
		return;
	}

	AQUILA_LOG_INFO("Cleaning up context for scene '{}'", scene->GetSceneName());

	iterator->second.QueueForDeletion(m_Device, GetRenderingSystems());
	m_MaterialSystem.CleanupSceneMaterialData(scene);
	m_SceneContexts.erase(iterator);
}

Ref<Resources::Texture2D> Renderer::GetSceneOutputTexture(SceneManagement::Scene *scene) const {
	auto iterator = m_SceneContexts.find(scene);
	if (iterator != m_SceneContexts.end()) {
		return iterator->second.renderTargets.composite->GetColorAttachment();
	}
	return nullptr;
}

void Renderer::ExecuteRenderingForScene(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec,
										const SceneRenderContext &context) {
	{
		PROFILE_SCOPE("	RenderShadowPass");
		RenderShadowPass(cmd, frameSpec);
	}

	{
		PROFILE_SCOPE("	RenderGBufferPass");
		RenderGBufferPass(cmd, frameSpec, context);
	}

	{
		PROFILE_SCOPE("	RenderLightingPass");
		RenderLightingPass(cmd, frameSpec, context);
	}

	{
		PROFILE_SCOPE("	RenderSkyboxPass");
		RenderSkyboxPass(cmd, frameSpec, context);
	}

	{
		PROFILE_SCOPE("	RenderGizmoPass");
		RenderGizmoPass(cmd, frameSpec, context);
	}

	{
		PROFILE_SCOPE("	RenderComposite");
		RenderCompositePass(cmd, frameSpec, context);
	}
}

void Renderer::RenderShadowPass(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec) {
	if (!m_ShadowRendering) {
		return;
	}

	for (uint32 i = 0; i < Rendering::Systems::ShadowRenderSystem::CASCADE_COUNT; ++i) {
		auto cascade = m_ShadowRendering->GetCascadeRenderTarget(i);
		VkImageLayout oldLayout =
			cascade->IsFirstUse() ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DynamicRendering::TransitionImages(m_Device, cmd, {}, VK_FORMAT_UNDEFINED, cascade->GetDepthImage(),
										   cascade->GetDepthFormat(), oldLayout,
										   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		cascade->MarkUsed();
	}

	m_ShadowRendering->OnRender(frameSpec);

	for (uint32 i = 0; i < Rendering::Systems::ShadowRenderSystem::CASCADE_COUNT; ++i) {
		auto cascade = m_ShadowRendering->GetCascadeRenderTarget(i);
		DynamicRendering::TransitionImages(m_Device, cmd, {}, VK_FORMAT_UNDEFINED, cascade->GetDepthImage(),
										   cascade->GetDepthFormat(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
										   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

void Renderer::RenderGBufferPass(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec,
								 const SceneRenderContext &context) {
	if (!m_DeferredRendering) {
		return;
	}

	const auto &gbuffer = context.renderTargets.gbuffer;
	auto colorImages = gbuffer->GetColorImages();
	VkImageLayout oldLayout =
		gbuffer->IsFirstUse() ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	DynamicRendering::TransitionImages(m_Device, cmd, colorImages, gbuffer->GetColorFormat(), VK_NULL_HANDLE,
									   VK_FORMAT_UNDEFINED, oldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	DynamicRendering::TransitionImages(m_Device, cmd, {}, VK_FORMAT_UNDEFINED, gbuffer->GetDepthImage(),
									   gbuffer->GetDepthFormat(), oldLayout,
									   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	gbuffer->MarkUsed();

	DynamicRendering::BeginGBuffer(cmd, gbuffer, true);
	m_DeferredRendering->OnRender(frameSpec);
	DynamicRendering::End(cmd);

	DynamicRendering::TransitionImages(m_Device, cmd, colorImages, gbuffer->GetColorFormat(), VK_NULL_HANDLE,
									   VK_FORMAT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	DynamicRendering::TransitionImages(m_Device, cmd, {}, VK_FORMAT_UNDEFINED, gbuffer->GetDepthImage(),
									   gbuffer->GetDepthFormat(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
									   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Renderer::RenderLightingPass(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec,
								  const SceneRenderContext &context) {
	if (!m_LightingRendering) {
		return;
	}

	const auto &lighting = context.renderTargets.lighting;
	auto colorImages = lighting->GetColorImages();
	VkImageLayout oldLayout =
		lighting->IsFirstUse() ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	DynamicRendering::TransitionImages(m_Device, cmd, colorImages, lighting->GetColorFormat(), VK_NULL_HANDLE,
									   VK_FORMAT_UNDEFINED, oldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	lighting->MarkUsed();

	DynamicRendering::Begin(cmd, lighting, true);
	m_LightingRendering->OnRender(frameSpec);
	DynamicRendering::End(cmd);

	DynamicRendering::TransitionImages(m_Device, cmd, colorImages, lighting->GetColorFormat(), VK_NULL_HANDLE,
									   VK_FORMAT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Renderer::RenderSkyboxPass(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec,
								const SceneRenderContext &context) {
	const auto &skybox = context.renderTargets.skybox;
	auto colorImages = skybox->GetColorImages();
	VkImageLayout oldLayout =
		skybox->IsFirstUse() ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	DynamicRendering::TransitionImages(m_Device, cmd, colorImages, skybox->GetColorFormat(), VK_NULL_HANDLE,
									   VK_FORMAT_UNDEFINED, oldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	skybox->MarkUsed();

	if (m_SkyboxRendering && frameSpec.renderSkybox) {
		auto *depthView = context.renderTargets.gbuffer->GetDepthAttachment()->GetImageView();
		DynamicRendering::BeginWithExternalDepth(cmd, skybox, depthView, true, false);
		m_SkyboxRendering->OnRender(frameSpec);
	} else {
		DynamicRendering::Begin(cmd, skybox, true);
	}

	DynamicRendering::End(cmd);
	DynamicRendering::TransitionImages(m_Device, cmd, colorImages, skybox->GetColorFormat(), VK_NULL_HANDLE,
									   VK_FORMAT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Renderer::RenderGizmoPass(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec,
							   const SceneRenderContext &context) {
	if (!m_GizmoRendering) {
		return;
	}

	const auto &gizmo = context.renderTargets.gizmo;
	auto colorImages = gizmo->GetColorImages();
	VkImageLayout oldLayout =
		gizmo->IsFirstUse() ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	DynamicRendering::TransitionImages(m_Device, cmd, colorImages, gizmo->GetColorFormat(), VK_NULL_HANDLE,
									   VK_FORMAT_UNDEFINED, oldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	gizmo->MarkUsed();

	DynamicRendering::Begin(cmd, gizmo, true, { { 0.0F, 0.0F, 0.0F, 0.0F } });
	m_GizmoRendering->OnRender(frameSpec);
	DynamicRendering::End(cmd);

	DynamicRendering::TransitionImages(m_Device, cmd, colorImages, gizmo->GetColorFormat(), VK_NULL_HANDLE,
									   VK_FORMAT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Renderer::RenderCompositePass(VkCommandBuffer cmd, const Rendering::Systems::FrameSpec &frameSpec,
								   const SceneRenderContext &context) {
	if (!m_CompositeRendering) {
		return;
	}

	const auto &composite = context.renderTargets.composite;
	auto colorImages = composite->GetColorImages();
	VkImageLayout oldLayout =
		composite->IsFirstUse() ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	DynamicRendering::TransitionImages(m_Device, cmd, colorImages, composite->GetColorFormat(), VK_NULL_HANDLE,
									   VK_FORMAT_UNDEFINED, oldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	composite->MarkUsed();

	DynamicRendering::Begin(cmd, composite, true);
	m_CompositeRendering->OnRender(frameSpec);
	DynamicRendering::End(cmd);

	DynamicRendering::TransitionImages(m_Device, cmd, colorImages, composite->GetColorFormat(), VK_NULL_HANDLE,
									   VK_FORMAT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Renderer::OnEvent(Events::Event &event) {
	// Treat renderer events
	Events::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Events::WindowResizeEvent>([this](const Events::WindowResizeEvent &event) {
		if (event.GetWidth() == 0 || event.GetHeight() == 0) {
			return false;
		}
		Resize({ event.GetWidth(), event.GetHeight() });
		return true;
	});

	dispatcher.Dispatch<Events::ViewportResizeEvent>([this](const Events::ViewportResizeEvent &event) {
		if (event.GetWidth() == 0 || event.GetHeight() == 0) {
			return false;
		}
		Resize({ event.GetWidth(), event.GetHeight() });
		return true; // consumed
	});

	// Check if there are events that concern systems
	if (!event.handled) {
		auto systems = GetRenderingSystems();

		for (auto &system : systems) {
			system->OnEvent(event);
		}
	}
}

void Renderer::Resize(const VkExtent2D newExtent) {
	if (newExtent.width == 0 || newExtent.height == 0) {
		AQUILA_LOG_DEBUG("Ignoring resize to zero extent: {}x{}", newExtent.width, newExtent.height);
		return;
	}

	m_Extent = newExtent;
	m_Resized = true;
}

void Renderer::InvalidateSwapchain() {
	auto extent = m_Window.GetExtent();
	while (m_Window.GetWidth() == 0 || m_Window.GetHeight() == 0) {
		extent = m_Window.GetExtent();
		glfwWaitEvents();
	}

	m_Device.Wait();

	if (m_Swapchain == nullptr) {
		m_Swapchain = CreateUnique<Swapchain>(m_Device, extent);
	} else {
		Ref<Swapchain> oldSwapchain = std::move(m_Swapchain);
		m_Swapchain = CreateUnique<Swapchain>(m_Device, extent, oldSwapchain);
	}

	m_Extent = extent;
	m_FrameStarted = false;
}

bool Renderer::InvalidatePasses() {
	if (!m_Resized) {
		return false;
	}

	if (m_Extent.width == 0 || m_Extent.height == 0) {
		m_Resized = false;
		return false;
	}

	AQUILA_LOG_INFO("Invalidating render passes to {}x{}", m_Extent.width, m_Extent.height);
	m_Device.Wait();

	m_EditorCamera.OnResize(static_cast<f32>(m_Extent.width), static_cast<f32>(m_Extent.height));

	for (auto &[scene, context] : m_SceneContexts) {
		context.Resize(m_Extent.width, m_Extent.height);

		BindSceneStaticDescriptors(context, scene);

		auto systems = GetRenderingSystems();
		for (auto *system : systems) {
			if (system != nullptr) {
				system->MarkSceneDescriptorsDirty(scene);
			}
		}
	}

	m_Resized = false;
	return true;
}
} // namespace Aquila::Graphics
