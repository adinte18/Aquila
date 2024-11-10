#include "Engine/OffscreenRenderer.h"

#include <array>


void Engine::OffscreenRenderer::BeginRenderPass(VkCommandBuffer commandBuffer) {
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffer.GetFramebuffer();
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = extent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {0.2f, 0.2f, 0.2f, 1.0f};
	clearValues[1].depthStencil = {1.0f, 0};
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = static_cast<float>(extent.height);
	viewport.width = static_cast<float>(extent.width);
	viewport.height = -static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{{0,0}, extent};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Engine::OffscreenRenderer::EndRenderPass(VkCommandBuffer commandBuffer) {
	vkCmdEndRenderPass(commandBuffer);
}

Engine::OffscreenRenderer::OffscreenRenderer(Device& device, VkExtent2D extent)
	: device(device), framebuffer(device, renderPass), extent(extent) {
	VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB; // or VK_FORMAT_B8G8R8A8_SRGB for sRGB
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear before rendering
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the result
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil buffer
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Undefined layout at the start
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Layout for presentation

    // Define the depth attachment
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT; // or VK_FORMAT_D24_UNORM_S8_UINT for depth and stencil
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear before rendering
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Do not store depth
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Undefined layout at the start
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Layout for depth attachment

    // Specify the attachment references
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0; // Index of the color attachment in the attachments array
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout during rendering

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1; // Index of the depth attachment in the attachments array
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Layout during rendering

    // Create a subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Use graphics pipeline
    subpass.colorAttachmentCount = 1; // One color attachment
    subpass.pColorAttachments = &colorAttachmentRef; // Reference to the color attachment
    subpass.pDepthStencilAttachment = &depthAttachmentRef; // Reference to the depth attachment

    // Specify dependencies for the render pass
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // External subpass (before the first subpass)
    dependency.dstSubpass = 0; // First subpass
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Stage for the source
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Stage for the destination
    dependency.srcAccessMask = 0; // No access mask for the source
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Write access to color attachment
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT; // Dependency is local

    // Create the render pass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2; // Number of attachments
    VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
    renderPassInfo.pAttachments = attachments; // Pointer to the attachments
    renderPassInfo.subpassCount = 1; // Number of subpasses
    renderPassInfo.pSubpasses = &subpass; // Pointer to the subpasses
    renderPassInfo.dependencyCount = 1; // Number of dependencies
    renderPassInfo.pDependencies = &dependency; // Pointer to the dependencies

    // Create the render pass
    if (vkCreateRenderPass(device.vk_GetDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

	framebuffer.CreateFramebuffer(extent, renderPass);
}

Engine::OffscreenRenderer::~OffscreenRenderer() {
	vkDestroyRenderPass(device.vk_GetDevice(), renderPass, nullptr);
}

void Engine::OffscreenRenderer::Recreate(VkExtent2D newExtent) {
    vkDeviceWaitIdle(device.vk_GetDevice());

    framebuffer.DestroyFramebuffer();

    extent = newExtent;

    framebuffer.CreateFramebuffer(extent, renderPass);

    std::cout << "Recreated framebuffer with extent: " << extent.width << " " << extent.height << std::endl;
}
