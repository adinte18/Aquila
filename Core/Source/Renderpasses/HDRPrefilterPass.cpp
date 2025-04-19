#include "Renderpasses/HDRPrefilterPass.h"

bool Engine::HDRPrefilterPass::CreateRenderTarget() {
    colorAttachment = Texture2D::create(m_Device);
    colorAttachment->CreateMipMappedCubemap(m_Extent.width, m_Extent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    return true;
}

bool Engine::HDRPrefilterPass::CreateFramebuffer() {
    auto mipLevels = colorAttachment->GetMipLevels();
    m_CubemapFaceViews.resize(mipLevels * 6);
    m_Framebuffers.resize(mipLevels * 6);

    VkExtent2D originalExtent = m_Extent;

    for (uint32_t mip = 0; mip < mipLevels; ++mip) {
        m_Extent.width = std::max(1u, m_Extent.width >> mip);
        m_Extent.height = std::max(1u, m_Extent.height >> mip);

        for (uint32_t face = 0; face < 6; ++face) {
            uint32_t index = mip * 6 + face;

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = colorAttachment->GetTextureImage();
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = mip;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = face;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            if (vkCreateImageView(m_Device.vk_GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create cubemap face view!");
            }
            m_CubemapFaceViews[index] = imageView;

            VkFramebufferCreateInfo faceFramebufferInfo{};
            faceFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            faceFramebufferInfo.renderPass = m_RenderPass;
            faceFramebufferInfo.attachmentCount = 1;
            faceFramebufferInfo.pAttachments = &m_CubemapFaceViews[index];
            faceFramebufferInfo.width = m_Extent.width;
            faceFramebufferInfo.height = m_Extent.height;
            faceFramebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_Device.vk_GetDevice(), &faceFramebufferInfo, nullptr,
                                    &m_Framebuffers[index]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer for cubemap face + mip!");
            }
        }

        m_Extent = originalExtent;
    }

    WriteToDescriptorSet();
    return true;
}


void Engine::HDRPrefilterPass::CreateClearValues() {
    m_ClearValues[0].color = {0.2f, 0.2f, 0.2f, 1.0f};
    m_ClearValues[1].depthStencil = {1.0f, 0};
}

bool Engine::HDRPrefilterPass::CreateRenderPass() {
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

