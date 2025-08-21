#ifndef AQUILA_FRAMEBUFFER_H
#define AQUILA_FRAMEBUFFER_H

#include "Defines.h"
#include "Engine/Renderer/Device.h"

namespace Engine {
    class Framebuffer {
    public:
        enum class Target {
            Offscreen,
            Swapchain
        };

        struct FramebufferDetails {
            uint32_t width;
            uint32_t height;
            Target target{Target::Offscreen};
            uint32_t layers{1};
            std::vector<VkImageView> attachments;
            VkRenderPass renderPass;
        };

        Framebuffer(Device& device, FramebufferDetails details);
        ~Framebuffer();

        AQUILA_NONCOPYABLE(Framebuffer);
        AQUILA_NONMOVEABLE(Framebuffer);

        static Ref<Framebuffer> Construct(Device& device, FramebufferDetails details) {
            return CreateRef<Framebuffer>(device, details);
        }

        void Resize(const VkExtent2D& newExtent);

        void Destroy() {
            if (m_Framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(m_Device->GetDevice(), m_Framebuffer, nullptr);
                m_Framebuffer = VK_NULL_HANDLE;
            }
        }

        [[nodiscard]] VkFramebuffer GetHandle() const { return m_Framebuffer; }
        [[nodiscard]] const VkExtent2D& GetExtent() const { return m_Extent; }
        [[nodiscard]] const Target GetTarget() const { return m_Target; }

    private:
        Device* m_Device;

        VkFramebuffer m_Framebuffer;
        VkExtent2D m_Extent;

        VkRenderPass m_RenderPass;
        std::vector<VkImageView> m_Attachments;

        Target m_Target;
    };
}

#endif // AQUILA_FRAMEBUFFER_H