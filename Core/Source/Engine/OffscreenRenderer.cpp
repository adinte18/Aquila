#include <Engine/OffscreenRenderer.h>
#include <array>

namespace Engine {
    OffscreenRenderer::OffscreenRenderer(Device& device) : m_Device(device), m_Renderpass(device, {800,600}) {
        CreateCommandBuffers();
        Initialize(800, 600);
    }
    OffscreenRenderer::~OffscreenRenderer() {
        FreeCommandBuffers();
    }

    VkDescriptorImageInfo OffscreenRenderer::GetImageInfo(RenderPassType type, VkImageLayout layout) const {
        VkDescriptorImageInfo info = {};
        info.imageView = m_Renderpass.GetRenderTarget(type);
        info.sampler = m_Renderpass.GetRenderSampler(type);
        info.imageLayout = layout;
        return info;
    }

    void OffscreenRenderer::CreateCommandBuffers() {
        m_CommandBuffers.resize(3); // 3 frames in flight - triple buffering

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_Device.vk_GetCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

        if (vkAllocateCommandBuffers(m_Device.vk_GetDevice(), &allocInfo, m_CommandBuffers.data()) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    VkCommandBuffer OffscreenRenderer::GetCurrentCommandBuffer() {
        return m_CommandBuffers[m_CurrentFrameID];
    }

    void OffscreenRenderer::FreeCommandBuffers() {
        vkFreeCommandBuffers(
            m_Device.vk_GetDevice(),
            m_Device.vk_GetCommandPool(),
            static_cast<uint32_t>(m_CommandBuffers.size()),
            m_CommandBuffers.data());
        m_CommandBuffers.clear();
    }

    void OffscreenRenderer::Initialize(uint32_t width, uint32_t height) {
        m_Extent = { width, height };

        m_Renderpass.CreateRenderTarget(m_Device, width, height);

        // Cubemap RenderPass
        m_Renderpass.CreateRenderPass(RenderPassType::ENV_TO_CUBEMAP);

        m_Renderpass.CreateRenderPass(RenderPassType::IBL);

        // Scene RenderPass
        m_Renderpass.CreateRenderPass(RenderPassType::GEOMETRY);

        // Post-processing RenderPass
        m_Renderpass.CreateRenderPass(RenderPassType::POST_PROCESSING);

        // Depth RenderPass
        m_Renderpass.CreateRenderPass(RenderPassType::SHADOW);

        // Final RenderPass
        m_Renderpass.CreateRenderPass(RenderPassType::FINAL);

        // Add here other render passes
        // ....
    }

    void OffscreenRenderer::Resize(VkExtent2D newExtent) {
        vkDeviceWaitIdle(m_Device.vk_GetDevice());

        m_Renderpass.DestroyAll();

        m_Extent = newExtent;
        m_Renderpass.SetExtent(m_Extent);

        m_Renderpass.CreateRenderTarget(m_Device, m_Extent.width, m_Extent.height);

        m_Renderpass.CreateRenderPass(RenderPassType::ENV_TO_CUBEMAP);

        m_Renderpass.CreateRenderPass(RenderPassType::IBL);

        m_Renderpass.CreateRenderPass(RenderPassType::GEOMETRY);

        m_Renderpass.CreateRenderPass(RenderPassType::POST_PROCESSING);

        // Depth RenderPass
        m_Renderpass.CreateRenderPass(RenderPassType::SHADOW);

        // Final RenderPass
        m_Renderpass.CreateRenderPass(RenderPassType::FINAL);

        // Add here other render passes (if there are any)
        // ....
    }

    void OffscreenRenderer::TransitionImages(VkCommandBuffer commandBuffer, RenderPassType src, RenderPassType dst) {
        struct TransitionInfo {
            VkImage image;
            VkImageLayout oldLayout;
            VkImageLayout newLayout;
            VkImageAspectFlags aspectMask;
            VkPipelineStageFlags srcStage;
            VkPipelineStageFlags dstStage;
            VkAccessFlags srcAccessMask;
            VkAccessFlags dstAccessMask;
            uint32_t mipLevels;
            uint32_t layerCount;
        };

        std::vector<TransitionInfo> transitions;

        if (src == RenderPassType::ENV_TO_CUBEMAP && dst == RenderPassType::IBL) {
            transitions.push_back({
                m_Renderpass.GetImage(src),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                1, // mipLevels
                6  // layerCount for cubemap
            });
        }

        if (src == RenderPassType::GEOMETRY && dst == RenderPassType::POST_PROCESSING) {
            transitions.push_back({
                m_Renderpass.GetImage(src),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                1,
                1
            });
        }

        if (src == RenderPassType::POST_PROCESSING && dst == RenderPassType::FINAL) {
            transitions.push_back({
                m_Renderpass.GetImage(src),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                1,
                1
            });
        }

        // Apply all transitions
        std::vector<VkImageMemoryBarrier> barriers;
        for (const auto& transition : transitions) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = transition.oldLayout;
            barrier.newLayout = transition.newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = transition.image;
            barrier.subresourceRange.aspectMask = transition.aspectMask;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = transition.mipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = transition.layerCount;
            barrier.srcAccessMask = transition.srcAccessMask;
            barrier.dstAccessMask = transition.dstAccessMask;

            barriers.push_back(barrier);
        }

        if (!barriers.empty()) {
            vkCmdPipelineBarrier(
                commandBuffer,
                transitions.front().srcStage,
                transitions.front().dstStage,
                0,
                0, nullptr,
                0, nullptr,
                static_cast<uint32_t>(barriers.size()), barriers.data()
            );
        }
    }

    void OffscreenRenderer::BeginRenderPass(VkCommandBuffer commandBuffer, RenderPassType type, int cubemapFace) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_Renderpass.GetRenderPass(type);

        renderPassInfo.framebuffer = (type == RenderPassType::ENV_TO_CUBEMAP) ? m_Renderpass.GetCubemapFramebuffer(cubemapFace) : m_Renderpass.GetIrradianceFramebuffer(cubemapFace);

        renderPassInfo.renderArea.offset = {0, 0};
        if (type == RenderPassType::ENV_TO_CUBEMAP) {
            renderPassInfo.renderArea.extent = {1024, 1024};
        }
        else renderPassInfo.renderArea.extent = {64, 64};

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.2f, 0.2f, 0.2f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        if (type == RenderPassType::ENV_TO_CUBEMAP) {
            VkViewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = 1024;
            viewport.height = 1024;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{{0, 0}, {1024, 1024}};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        }
        else {
            VkViewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = 64;
            viewport.height = 64;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{{0, 0}, {64, 64}};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        }
    }

    void OffscreenRenderer::BeginRenderPass(VkCommandBuffer commandBuffer, RenderPassType type) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_Renderpass.GetRenderPass(type);
        renderPassInfo.framebuffer = m_Renderpass.GetFramebuffer(type);
        renderPassInfo.renderArea.offset = {0, 0};
        if (type == RenderPassType::SHADOW) {
            renderPassInfo.renderArea.extent = {8192, 8192};
        } else {
            renderPassInfo.renderArea.extent = m_Extent;
        }
        if (type == RenderPassType::SHADOW) {
            std::array<VkClearValue, 1> clearValues{};
            clearValues[0].depthStencil = {1.0f, 0};
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();
        } else {
            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {0.2f, 0.2f, 0.2f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

        }
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        if (type == RenderPassType::SHADOW) {
            viewport.width = 8192.0f;
            viewport.height = 8192.0f;
        } else {
            viewport.width = static_cast<float>(m_Extent.width);
            viewport.height = static_cast<float>(m_Extent.height);
        }
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        if (type == RenderPassType::SHADOW) {
            VkRect2D scissor{{0, 0}, {8192, 8192}};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        }
        else {
            VkRect2D scissor{{0, 0}, m_Extent};
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        }
    }

    void OffscreenRenderer::EndRenderPass(VkCommandBuffer commandBuffer) {
        vkCmdEndRenderPass(commandBuffer);
    }

    VkCommandBuffer OffscreenRenderer::BeginFrame() {
        assert(!m_FrameStarted && "Frame already started!");
        m_FrameStarted = true;

        auto commandBuffer = GetCurrentCommandBuffer();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        return commandBuffer;
    }

    void OffscreenRenderer::EndFrame() {
        assert(m_FrameStarted && "Can't end a frame that wasn't started!");

        auto commandBuffer = GetCurrentCommandBuffer();

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        // Create a semaphore for synchronization (to signal GPU that it's done with the frame)
        VkSemaphore signalSemaphore;
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(m_Device.vk_GetDevice(), &semaphoreCreateInfo, nullptr, &signalSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphore!");
        }

        // Create a fence for waiting (create it unsignaled)
        VkFence fence;
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = 0;  // Don't use the SIGNALED_BIT, so the fence starts unsignaled

        if (vkCreateFence(m_Device.vk_GetDevice(), &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fence!");
        }

        // Set up the submission info
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;  // Use the created semaphore

        // Submit the command buffer for execution (offscreen rendering)
        if (vkQueueSubmit(m_Device.vk_GetGraphicsQueue(), 1, &submitInfo, fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit command buffer!");
        }

        // Wait for the fence to signal that the GPU has completed the command buffer execution
        vkWaitForFences(m_Device.vk_GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);

        // Clean up the fence and semaphore after use
        vkDestroyFence(m_Device.vk_GetDevice(), fence, nullptr);
        vkDestroySemaphore(m_Device.vk_GetDevice(), signalSemaphore, nullptr);

        m_FrameStarted = false;
        m_CurrentFrameID = (m_CurrentFrameID + 1) % 3;
    }
}









// void Engine::OffscreenRenderer::BeginRenderPass(VkCommandBuffer commandBuffer) {
// 	VkRenderPassBeginInfo renderPassInfo{};
// 	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
// 	renderPassInfo.renderPass = renderPass;
// 	renderPassInfo.framebuffer = framebuffer.GetFramebuffer();
// 	renderPassInfo.renderArea.offset = {0, 0};
// 	renderPassInfo.renderArea.extent = extent;

// 	std::array<VkClearValue, 2> clearValues{};
// 	clearValues[0].color = {0.2f, 0.2f, 0.2f, 1.0f};
// 	clearValues[1].depthStencil = {1.0f, 0};
// 	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
// 	renderPassInfo.pClearValues = clearValues.data();

// 	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

// 	VkViewport viewport{};
// 	viewport.x = 0.0f;
// 	viewport.y = static_cast<float>(extent.height);
// 	viewport.width = static_cast<float>(extent.width);
// 	viewport.height = -static_cast<float>(extent.height); // inverse coordinate system
// 	viewport.minDepth = 0.0f;
// 	viewport.maxDepth = 1.0f;
// 	VkRect2D scissor{{0,0}, extent};
// 	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
// 	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
// }

// void Engine::OffscreenRenderer::EndRenderPass(VkCommandBuffer commandBuffer) {
// 	vkCmdEndRenderPass(commandBuffer);
// }

// Engine::OffscreenRenderer::OffscreenRenderer(Device& device)
// 	: device(device) {

// 	this->extent = {1,1};

// 	// //Define the color attachment
// 	// VkAttachmentDescription colorAttachment = {};
//     // colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB; // or VK_FORMAT_B8G8R8A8_SRGB for sRGB
//     // colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
//     // colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear before rendering
//     // colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the result
//     // colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil buffer
//     // colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//     // colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Undefined layout at the start
//     // colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Layout for presentation

//     // // Define the depth attachment
//     // VkAttachmentDescription depthAttachment = {};
//     // depthAttachment.format = VK_FORMAT_D32_SFLOAT; // or VK_FORMAT_D24_UNORM_S8_UINT for depth and stencil
//     // depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
//     // depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear before rendering
//     // depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Do not store depth
//     // depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//     // depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Undefined layout at the start
//     // depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Layout for depth attachment

//     // // Specify the attachment references
//     // VkAttachmentReference colorAttachmentRef = {};
//     // colorAttachmentRef.attachment = 0; // Index of the color attachment in the attachments array
//     // colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout during rendering

//     // VkAttachmentReference depthAttachmentRef = {};
//     // depthAttachmentRef.attachment = 1; // Index of the depth attachment in the attachments array
//     // depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Layout during rendering

//     // // Create a subpass
//     // VkSubpassDescription subpass = {};
//     // subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Use graphics pipeline
//     // subpass.colorAttachmentCount = 1; // One color attachment
//     // subpass.pColorAttachments = &colorAttachmentRef; // Reference to the color attachment
//     // subpass.pDepthStencilAttachment = &depthAttachmentRef; // Reference to the depth attachment

//     // // Specify dependencies for the render pass
//     // VkSubpassDependency dependency = {};
//     // dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // External subpass (before the first subpass)
//     // dependency.dstSubpass = 0; // First subpass
//     // dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Stage for the source
//     // dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Stage for the destination
//     // dependency.srcAccessMask = 0; // No access mask for the source
//     // dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Write access to color attachment
//     // dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT; // Dependency is local

//     // // Create the render pass
//     // VkRenderPassCreateInfo renderPassInfo = {};
//     // renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//     // renderPassInfo.attachmentCount = 2; // Number of attachments
//     // VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
//     // renderPassInfo.pAttachments = attachments; // Pointer to the attachments
//     // renderPassInfo.subpassCount = 1; // Number of subpasses
//     // renderPassInfo.pSubpasses = &subpass; // Pointer to the subpasses
//     // renderPassInfo.dependencyCount = 1; // Number of dependencies
//     // renderPassInfo.pDependencies = &dependency; // Pointer to the dependencies

//     // // Create the render pass
//     // if (vkCreateRenderPass(device.vk_GetDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
//     //     throw std::runtime_error("failed to create render pass!");
//     // }

// 	// framebuffer.CreateFramebuffer(extent, renderPass);
// }

// Engine::OffscreenRenderer::~OffscreenRenderer() {
// 	vkDestroyRenderPass(device.vk_GetDevice(), renderPass, nullptr);
// }


// // void Engine::OffscreenRenderer::Recreate(const VkExtent2D newExtent) {
// //     vkDeviceWaitIdle(device.vk_GetDevice());

// // 	if (newExtent.height == extent.height && newExtent.width == extent.width) {
// // 		return;
// // 	}

// //     framebuffer.DestroyFramebuffer();

// //     extent = newExtent;

// //     framebuffer.CreateFramebuffer(extent, renderPass);

// //     std::cout << "Recreated framebuffer with extent: " << extent.width << " " << extent.height << std::endl;
// // }
