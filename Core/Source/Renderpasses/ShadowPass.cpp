//
// Created by alexa on 16/04/2025.
//

#include "Renderpasses/ShadowPass.h"

bool Engine::ShadowPass::CreateRenderTarget() {
    depthAttachment = RenderTarget::CreateDepthTexture(m_Device, 8192, 8192, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    return depthAttachment->HasImageView();
}

bool Engine::ShadowPass::CreateFramebuffer() {
    m_Extent = {8192, 8192};

    if (!depthAttachment || !depthAttachment->HasImageView()) {
        throw std::runtime_error("Missing image views for framebuffer attachments!");
    }

    const auto attachment = depthAttachment->GetTextureImageView();

    m_Framebuffers[0] = Engine::Framebuffer::Construct(m_Device, {
        m_Extent.width,
        m_Extent.height,
        Engine::Framebuffer::Target::Offscreen,
        1,
        {attachment},
        m_RenderPass
    });

    // write framebuffer result to descriptor set
    WriteToDescriptorSet();

    return true;
}

void Engine::ShadowPass::CreateClearValues() {
    m_ClearValues[0].depthStencil = {1.0f, 0};
}

bool Engine::ShadowPass::CreateRenderPass() {
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &depthAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(m_Device.GetDevice(), &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
        return false;
    }

    return true;
}
