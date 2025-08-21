#include "Engine/Renderer/Renderer.h"
#include "Engine/Controller.h"
#include "Engine/Renderer/Device.h"
#include "Platform/DebugLog.h"

namespace Engine {

    Renderer::Renderer(Device& device, Window& window) : m_Device(device), m_Window(window) {
        Initialize(m_Window.GetExtent().width, m_Window.GetExtent().height);
    }

    Renderer::~Renderer() {
        m_Device.Wait();
    }

    void Renderer::Initialize(uint32_t width, uint32_t height) {
        m_Extent = {width, height};
        
        m_CommandPool = CreateUnique<CommandBufferPool>(m_Device, Swapchain::MAX_FRAMES_IN_FLIGHT);
        m_SemaphoreManager = CreateUnique<SemaphoreManager>(m_Device, Swapchain::MAX_FRAMES_IN_FLIGHT);
        
        SetupCommandBuffers();
        SetupSynchronization();

        m_Swapchain = CreateUnique<Swapchain>(m_Device, m_Extent);
        if (!m_Swapchain) {
            throw std::runtime_error("Failed to create swapchain");
        }

        m_SharedDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(m_Device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        if ((m_GeometryPass = GeometryPass::Initialize(m_Device, m_Extent, m_SharedDescriptorSetLayout))) {
            std::cout << "Geometry pass initialized" << std::endl;
        }

        if ((m_SceneRendering = CreateRef<SceneRenderSystem>(m_Device, m_GeometryPass->GetRenderPass()))) {
            std::cout << "Scene rendering system initialized" << std::endl;
        }
    }

    void Renderer::SetupCommandBuffers() {
        m_PresentCommandBuffers.reserve(Swapchain::MAX_FRAMES_IN_FLIGHT);
        m_OffscreenCommandBuffers.reserve(Swapchain::MAX_FRAMES_IN_FLIGHT);
        
        for (uint32_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; ++i) {
            m_PresentCommandBuffers.push_back(
                m_CommandPool->GetFrameCommandBuffer(CommandBufferType::PRESENT, i, "PresentCommandBuffer"));
            m_OffscreenCommandBuffers.push_back(
                m_CommandPool->GetFrameCommandBuffer(CommandBufferType::OFFSCREEN, i, "OffscreenCommandBuffer"));
        }
    }

    void Renderer::SetupSynchronization() {
        m_SemaphoreManager->CreateSemaphore("OffscreenFinished");
    }

    VkCommandBuffer Renderer::BeginFrame() {
        AQUILA_CORE_ASSERT(!m_FrameStarted && "Can't call BeginFrame while already in progress");

        auto result = m_Swapchain->GetNextImage(&m_CurrentImageID);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            InvalidateSwapchain();
            return VK_NULL_HANDLE;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        m_FrameStarted = true;
        m_CurrentRenderType = RenderType::PRESENT;

        auto& presentCmd = m_PresentCommandBuffers[m_CurrentFrameID];
        presentCmd->Reset();
        presentCmd->Begin();

        return presentCmd->GetHandle();
    }

    void Renderer::EndFrame() {
        AQUILA_CORE_ASSERT(m_FrameStarted && "Can't call EndFrame while frame is not in progress");

        auto& presentCmd = m_PresentCommandBuffers[m_CurrentFrameID];
        presentCmd->End();

        VkSubmitInfo offSubmit{};
        offSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        offSubmit.waitSemaphoreCount = 0;
        offSubmit.pWaitSemaphores = nullptr;
        offSubmit.commandBufferCount = 1;
        
        VkCommandBuffer offscreenHandle = m_OffscreenCommandBuffers[m_CurrentFrameID]->GetHandle();
        offSubmit.pCommandBuffers = &offscreenHandle;
        
        VkSemaphore offSignal = m_SemaphoreManager->GetSemaphore("OffscreenFinished", m_CurrentFrameID);
        offSubmit.signalSemaphoreCount = 1;
        offSubmit.pSignalSemaphores = &offSignal;

        // submit offscreen 
        if (vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &offSubmit, VK_NULL_HANDLE) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit offscreen command buffer!");
        }

        VkCommandBuffer presentHandle = presentCmd->GetHandle();
        auto result = m_Swapchain->SubmitCommandBuffers(&presentHandle, &m_CurrentImageID, &offSignal, 1);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window.IsWindowResized()) {
            m_Window.ResetResizedFlag();
            InvalidateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        m_FrameStarted = false;
        m_CurrentFrameID = (m_CurrentFrameID + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::RenderScene() {
        // Note(A) : Keep in mind to update rendering systems before recording any commands to the command buffer.
        GetRenderingSystem<SceneRenderSystem>()->UpdateBuffer(Engine::Controller::Get()->GetCamera());
        GetRenderingSystem<SceneRenderSystem>()->SendDataToGPU(m_CurrentFrameID);

        auto& offscreenCmd = m_OffscreenCommandBuffers[m_CurrentFrameID];
        offscreenCmd->Reset();
        offscreenCmd->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        Engine::FrameSpec frameSpec{};
        frameSpec.frameIndex = m_CurrentFrameID;
        frameSpec.commandBuffer = offscreenCmd->GetHandle();

        // Geometry pass
        {
            RenderPassConfig config{};
            config.type = RenderType::OFFSCREEN;
            config.renderpass = GetPassObject<GeometryPass>();
            config.mipExtent = UINT32_MAX;
            config.autoSetViewportScissor = true;
            
            BeginRenderPass(offscreenCmd->GetHandle(), config);
                GetRenderingSystem<SceneRenderSystem>()->Render(frameSpec);
            EndRenderPass(offscreenCmd->GetHandle());
        }

        offscreenCmd->End();
    }

    void Renderer::Resize(VkExtent2D newExtent) {
        if (newExtent.width == 0 || newExtent.height == 0) {
            return;
        }

        m_Extent = newExtent;
        m_Resized = true;
    }

    void Renderer::InvalidateSwapchain() {
        auto extent = m_Window.GetExtent();
        while (extent.width == 0 || extent.height == 0) {
            extent = m_Window.GetExtent();
            glfwWaitEvents();
        }

        m_Device.Wait();

        if (m_Swapchain == nullptr) {
            m_Swapchain = CreateUnique<Swapchain>(m_Device, extent);
        } else {
            Ref<Swapchain> oldSwapchain = std::move(m_Swapchain);
            m_Swapchain = CreateUnique<Swapchain>(m_Device, extent, oldSwapchain);

            if (!oldSwapchain->vk_CompareSCFormats(*m_Swapchain)) {
                throw std::runtime_error("Swap chain image(or depth) format has changed");
            }
        }

        m_Extent = extent;

        m_FrameStarted = false;
    }

    void Renderer::InvalidatePasses() {
        if (m_GeometryPass && m_Resized) {
            m_Device.Wait();
            Debug::Log("Invalidating geometry pass due to resize");

            if (m_Extent.width > 0 && m_Extent.height > 0) {
                m_GeometryPass->Invalidate(m_Extent);
            } else {
                Debug::Log("Skip pass invalidation: zero extent");
            }
        }

        m_Resized = false;
    }

    void Renderer::BeginRenderPass(VkCommandBuffer cmd, const RenderPassConfig& config) {
        AQUILA_CORE_ASSERT(m_FrameStarted && "Frame not started!");
        AQUILA_CORE_ASSERT(!m_RenderPassActive && "Render pass already active!");

        m_RenderPassActive = true;
        m_CurrentRenderType = config.type;

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        
        VkExtent2D extent{};
        std::array<VkClearValue, 2> clearValues{};

        if (config.type == RenderType::PRESENT) {
            renderPassInfo.renderPass = m_Swapchain->GetRenderpass();
            renderPassInfo.framebuffer = m_Swapchain->GetFramebuffer(m_CurrentImageID);
            extent = m_Swapchain->GetExtent();
            
            clearValues = {
                VkClearValue{.color = {0.0f, 0.0f, 0.0f, 1.0f}},
                VkClearValue{.depthStencil = {1.0f, 0}}
            };
        } else {            
            auto pass = config.renderpass;
            renderPassInfo.renderPass = pass->GetRenderPass();
            renderPassInfo.framebuffer = pass->GetFramebuffers()->GetHandle();
            
            extent = pass->GetExtent();
            clearValues = pass->GetClearValues();
        }

        renderPassInfo.renderArea.offset = {0, 0};
        if (config.mipExtent != UINT32_MAX) {
            renderPassInfo.renderArea.extent = {config.mipExtent, config.mipExtent};
            extent = {config.mipExtent, config.mipExtent};
        } else {
            renderPassInfo.renderArea.extent = extent;
        }

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        if (config.autoSetViewportScissor) {
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(extent.width);
            viewport.height = static_cast<float>(extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = extent;

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);
        }
    }

    void Renderer::EndRenderPass(VkCommandBuffer cmd) {
        AQUILA_CORE_ASSERT(m_FrameStarted && "Frame not started!");
        AQUILA_CORE_ASSERT(m_RenderPassActive && "No active render pass to end!");

        vkCmdEndRenderPass(cmd);
        m_RenderPassActive = false;
    }
}