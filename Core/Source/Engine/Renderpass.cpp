#include <array>

#include <vector>
#include <stdexcept>
#include <Engine/RenderPass.h>
#include <Engine/CommandBuffer.h>
#include <Engine/Framebuffer.h>

namespace Engine {
    RenderPass::RenderPass(VkDevice device) : device(device), renderPass(VK_NULL_HANDLE), depthAttachment() {
    }

    RenderPass::~RenderPass() {
        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, renderPass, nullptr);
        }
    }

    void RenderPass::AddColorAttachment(VkFormat format, VkSampleCountFlagBits samples) {
        Attachment attachment{};

        // Set up the attachment description
        attachment.description = {};
        attachment.description.format = format;
        attachment.description.samples = samples;
        attachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Set up the attachment reference
        attachment.reference = {};
        attachment.reference.attachment = static_cast<uint32_t>(colorAttachments.size());
        attachment.reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colorAttachments.push_back(attachment);
    }

    void RenderPass::SetDepthAttachment(VkFormat format, VkSampleCountFlagBits samples) {
        if (hasDepthAttachment) {
            throw std::runtime_error("Depth attachment already set.");
        }

        depthAttachment.description = {};
        depthAttachment.description.format = format;
        depthAttachment.description.samples = samples;
        depthAttachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Initialize the reference for the depth attachment
        depthAttachment.reference = {};
        depthAttachment.reference.attachment = static_cast<uint32_t>(colorAttachments.size()); // Index based on color attachments
        depthAttachment.reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Correct layout

        hasDepthAttachment = true;
    }

    void RenderPass::CreateRenderPass() {
        if (colorAttachments.empty()) {
            throw std::runtime_error("No color attachments added.");
        }

        std::vector<VkAttachmentDescription> attachments;
        attachments.reserve(colorAttachments.size() + (hasDepthAttachment ? 1 : 0)); // Reserve space for depth attachment if present

        for (const auto& attachment : colorAttachments) {
            attachments.push_back(attachment.description);
        }

        if (hasDepthAttachment) {
            attachments.push_back(depthAttachment.description);
        }

        // Set up the subpass
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());

        std::vector<VkAttachmentReference> colorAttachmentRefs;
        colorAttachmentRefs.reserve(colorAttachments.size());
        for (const auto&[description, reference] : colorAttachments) {
            colorAttachmentRefs.push_back(reference);
        }
        subpass.pColorAttachments = colorAttachmentRefs.data();

        if (hasDepthAttachment) {
            subpass.pDepthStencilAttachment = &depthAttachment.reference;
        } else {
            subpass.pDepthStencilAttachment = nullptr; // Ensure it's explicitly set to nullptr if no depth attachment
        }

        // Create the render pass
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass!");
        }
    }
}
