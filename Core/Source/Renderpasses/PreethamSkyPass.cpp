#include "Renderpasses/PreethamSkyPass.h"

bool Engine::PreethamSkyPass::CreateRenderTarget() {
    colorAttachment = RenderTarget::CreateColorTexture(m_Device,
        RenderTarget::TargetType::CUBEMAP,
        m_Extent.width,
        m_Extent.height,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    return colorAttachment->HasImageView();
}

bool Engine::PreethamSkyPass::CreateFramebuffer() {

    for (uint32_t i = 0; i < 6; ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = colorAttachment->GetTextureImage();
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = i;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(m_Device.vk_GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cubemap face view!");
        }
        m_CubemapFaceViews[i] = imageView;
    }

    for (uint32_t face = 0; face < 6; face++) {
        VkFramebufferCreateInfo faceFramebufferInfo{};
        faceFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        faceFramebufferInfo.renderPass = m_RenderPass;
        faceFramebufferInfo.width = m_Extent.width;
        faceFramebufferInfo.height = m_Extent.height;
        faceFramebufferInfo.layers = 1;
        faceFramebufferInfo.attachmentCount = 1; // 1 attachment per framebuffer
        faceFramebufferInfo.pAttachments = &m_CubemapFaceViews[face];

        if (vkCreateFramebuffer(m_Device.vk_GetDevice(), &faceFramebufferInfo, nullptr,
                              &m_Framebuffers[face]) != VK_SUCCESS) {
            return false;
        }
    }
    // write framebuffer result to descriptor set
    WriteToDescriptorSet();

    return true;
}

void Engine::PreethamSkyPass::CreateClearValues() {
    m_ClearValues[0].color = {0.2f, 0.2f, 0.2f, 1.0f};
    m_ClearValues[1].depthStencil = {1.0f, 0};
}

bool Engine::PreethamSkyPass::CreateRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    std::vector<VkSubpassDependency> dependencies;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(m_Device.vk_GetDevice(), &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
        return false;
    }

    return true;
}

