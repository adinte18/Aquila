#include "Renderpasses/LUTPass.h"

bool Engine::LUTPass::CreateRenderTarget() {
    colorAttachment = RenderTarget::CreateColorTexture(m_Device,
        RenderTarget::TargetType::TEXTURE_2D,
        m_Extent.width,
        m_Extent.height,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    return colorAttachment->HasImageView();
}

bool Engine::LUTPass::CreateFramebuffer() {
    if (!colorAttachment || !colorAttachment->HasImageView()) {
        throw std::runtime_error("Missing image views for framebuffer attachments!");
    }

    const auto attachment = colorAttachment->GetTextureImageView();

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

void Engine::LUTPass::CreateClearValues() {
    m_ClearValues[0].color = {0.2f, 0.2f, 0.2f, 1.0f};
    m_ClearValues[1].depthStencil = {1.0f, 0};
}

bool Engine::LUTPass::CreateRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Subpass dependency to ensure proper synchronization
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_Device.GetDevice(), &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
        return false;
    }

    return true;
}

