#include "Engine/Framebuffer.h"
#include "Engine/Core.h"

namespace Engine {
    Framebuffer::Framebuffer(Device& device, FramebufferDetails details)
        : m_Extent({details.width, details.height}),
          m_Device(&device),
          m_RenderPass(details.renderPass),
          m_Attachments(std::move(details.attachments)),
          m_Target(details.target) {

        if (m_Attachments.empty()) {
            throw std::runtime_error("Framebuffer must have at least one attachment");
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(m_Attachments.size());
        framebufferInfo.pAttachments = m_Attachments.data();
        framebufferInfo.width = m_Extent.width;
        framebufferInfo.height = m_Extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_Device->vk_GetDevice(), &framebufferInfo, nullptr, &m_Framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }

    Framebuffer::~Framebuffer() {
        Destroy();
    }

    void Framebuffer::Resize(const VkExtent2D& newExtent) {
        m_Extent = newExtent;

        Destroy();
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(m_Attachments.size());
        framebufferInfo.pAttachments = m_Attachments.data();
        framebufferInfo.width = m_Extent.width;
        framebufferInfo.height = m_Extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_Device->vk_GetDevice(), &framebufferInfo, nullptr, &m_Framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}
