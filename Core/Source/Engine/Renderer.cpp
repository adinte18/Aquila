#include "Engine/Renderer.h"
#include "Asserts.h"
#include "Engine/Device.h"
#include <memory>

namespace Engine {

    Renderer::Renderer(Device& device, Window& window) : m_Device(device), m_Window(window) {
        Initialize(m_Window.GetExtent().width, m_Window.GetExtent().height);
    }

    Renderer::~Renderer() {
        FreeCommandBuffers();
    }

    void Renderer::Initialize(uint32_t width, uint32_t height) {
        CreateCommandBuffers();

        m_Extent = {width, height};

        // Swapchain for rendering to the screen
        // Note (A) : Use the swapchain to render ONLY the UI of the editor and the "game"/runtime view. Editor viewport should render everything to an offscreen framebuffer.
        m_Swapchain = std::make_unique<Swapchain>(m_Device, m_Extent);
        if (!m_Swapchain) {
            throw std::runtime_error("Failed to create swapchain");
        }

        // Shared descriptor set layout
        m_SharedDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(m_Device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        // ====== Scene rendering system ====== 
        if ((m_GeometryPass = GeometryPass::Initialize(m_Device, m_Extent, m_SharedDescriptorSetLayout)))
            std::cout << "Geometry pass initialized" << std::endl;

        if ((m_SceneRendering = std::make_shared<SceneRenderSystem>(m_Device, m_GeometryPass->GetRenderPass()))) {
            std::cout << "Scene rendering system initialized" << std::endl;
        }
        // =====================================
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

        vkDeviceWaitIdle(m_Device.vk_GetDevice());

        if (m_Swapchain == nullptr) {
            m_Swapchain = std::make_unique<Swapchain>(m_Device, extent);
        }
        else {
            Ref<Swapchain> oldSwapchain = std::move(m_Swapchain);
            m_Swapchain = std::make_unique<Swapchain>(m_Device, extent, oldSwapchain);

            if(!oldSwapchain->vk_CompareSCFormats(*m_Swapchain)) {
                throw std::runtime_error("Swap chain image(or depth) format has changed");
            }
        }

        m_FrameStarted = false;
    }

    void Renderer::InvalidatePasses() {
        if (m_GeometryPass && m_Resized) {
            m_GeometryPass->Invalidate(m_Extent);
        }

        m_Resized = false;
    }


    void Renderer::CreateCommandBuffers() {
        m_CommandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT); // 2 - one for offscreen, one for onscreen

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_Device.GetCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

        if (vkAllocateCommandBuffers(m_Device.vk_GetDevice(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers");
        }
    }

    void Renderer::FreeCommandBuffers() {
        vkFreeCommandBuffers(m_Device.vk_GetDevice(), m_Device.GetCommandPool(),
                             static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
        m_CommandBuffers.clear();
    }

    VkCommandBuffer Renderer::BeginFrame() {
    AQUILA_CORE_ASSERT(!m_FrameStarted && "Can't call vk_BeginFrame while already in progress");
        auto result = m_Swapchain->GetNextImage(&m_CurrentImageID);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            InvalidateSwapchain();
            return VK_NULL_HANDLE;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        m_FrameStarted = true;

        auto commandBuffer = GetCurrentCommandBuffer();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        return commandBuffer;    
    }

    void Renderer::EndFrame() {
        AQUILA_CORE_ASSERT(m_FrameStarted && "Can't call vk_EndFrame while frame is not in progress");
        auto commandBuffer = GetCurrentCommandBuffer();

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        auto result = m_Swapchain->SubmitCommandBuffers(&commandBuffer, &m_CurrentImageID);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window.IsWindowResized()) {
            m_Window.ResetResizedFlag();
            InvalidateSwapchain();
        }

        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        m_FrameStarted = false;

        m_CurrentFrameID = (m_CurrentFrameID + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::BeginSwapchainRenderPass(VkCommandBuffer commandBuffer) {
        AQUILA_CORE_ASSERT(m_FrameStarted && "Frame not started!");
        AQUILA_CORE_ASSERT(commandBuffer == GetCurrentCommandBuffer() && "Command buffer mismatch!");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_Swapchain->GetRenderpass();
        renderPassInfo.framebuffer = m_Swapchain->GetFramebuffer(m_CurrentImageID);
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_Swapchain->GetExtent();

        std::array<VkClearValue, 2> clearValues = {
            VkClearValue{.color = {0.0f, 0.0f, 0.0f, 1.0f}},
            VkClearValue{.depthStencil = {1.0f, 0}} 
        };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_Swapchain->GetExtent().width);
        viewport.height = static_cast<float>(m_Swapchain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0,0}, m_Swapchain->GetExtent()};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void Renderer::EndRenderPass(VkCommandBuffer commandBuffer) {
        AQUILA_CORE_ASSERT(commandBuffer == GetCurrentCommandBuffer() && "Command buffer mismatch!");

        vkCmdEndRenderPass(commandBuffer);
    }
}