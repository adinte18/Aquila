#include "Engine/OnscreenRenderer.h"

#include <array>

Engine::OnscreenRenderer::OnscreenRenderer(Window& window, Device& device)
    :window(window), device(device) {
    vk_RecreateSwapChain();
    vk_CreateCommandBuffers();
}


Engine::OnscreenRenderer::~OnscreenRenderer() {
    vk_FreeCommandBuffers();
}

VkImageView Engine::OnscreenRenderer::vk_GetCurrentImageView() const {
    return swapChain->GetImageView(currentImageID);
}

VkCommandBuffer Engine::OnscreenRenderer::vk_BeginFrame()
{
    AQUILA_CORE_ASSERT(!frameStarted && "Can't call vk_BeginFrame while already in progress");
    auto result = swapChain->GetNextImage(&currentImageID);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        vk_RecreateSwapChain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    frameStarted = true;

    auto commandBuffer = vk_GetCurrentCommandBuffer();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    return commandBuffer;
}

void Engine::OnscreenRenderer::vk_EndFrame()
{
    AQUILA_CORE_ASSERT(frameStarted && "Can't call vk_EndFrame while frame is not in progress");
    auto commandBuffer = vk_GetCurrentCommandBuffer();

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    auto result = swapChain->SubmitCommandBuffers(&commandBuffer, &currentImageID);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.IsWindowResized()) {
        window.ResetResizedFlag();
        vk_RecreateSwapChain();
    }

    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    frameStarted = false;

    currentFrameID = (currentFrameID + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
}

VkExtent2D Engine::OnscreenRenderer::vk_GetSwapChainExtent() const
{
    return swapChain->GetExtent();
}

void Engine::OnscreenRenderer::vk_BeginSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
    AQUILA_CORE_ASSERT(frameStarted
        && "Can't call vk_BeginSwapChainRenderPass if frame is not in progress");
    AQUILA_CORE_ASSERT(commandBuffer == vk_GetCurrentCommandBuffer()
        && "Can't begin render pass on command buffer from a different frame");

    VkRenderPass pass =  swapChain->GetRenderpass();
    VkFramebuffer fb = swapChain->GetFramebuffer(currentImageID);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pass;
    renderPassInfo.framebuffer = fb;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain->GetExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.f, 0.f, 0.f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain->GetExtent().width);
    viewport.height = static_cast<float>(swapChain->GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0,0}, swapChain->GetExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Engine::OnscreenRenderer::vk_EndSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
    AQUILA_CORE_ASSERT(frameStarted
    && "Can't call vk_EndSwapChainRenderPass if frame is not in progress");
    AQUILA_CORE_ASSERT(commandBuffer == vk_GetCurrentCommandBuffer()
        && "Can't end render pass on command buffer from a different frame");
    vkCmdEndRenderPass(commandBuffer);
}

void Engine::OnscreenRenderer::vk_CreateCommandBuffers(){
    commandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT * 2);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.GetCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device.vk_GetDevice(), &allocInfo, commandBuffers.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void Engine::OnscreenRenderer::vk_FreeCommandBuffers()
{
    vkFreeCommandBuffers(
        device.vk_GetDevice(),
        device.GetCommandPool(),
        static_cast<uint32_t>(commandBuffers.size()),
        commandBuffers.data());
    commandBuffers.clear();
}

void Engine::OnscreenRenderer::vk_RecreateSwapChain() {
    std::cout << "Trying to recreate swap chain" << std::endl;
    auto extent = window.GetExtent();
    while (extent.width == 0 || extent.height == 0) {
        extent = window.GetExtent();
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device.vk_GetDevice());

    if (swapChain == nullptr) {
        swapChain = std::make_unique<Swapchain>(device, extent);
    }
    else {
        Ref<Swapchain> oldSC = std::move(swapChain);
        swapChain = std::make_unique<Swapchain>(device, extent, oldSC);

        if(!oldSC->vk_CompareSCFormats(*swapChain)) {
            throw std::runtime_error("Swap chain image(or depth) format has changed");
        }
    }
    std::cout << "Finished" << std::endl;
}

