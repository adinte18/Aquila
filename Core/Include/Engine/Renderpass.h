#ifndef RENDERPASS_H
#define RENDERPASS_H

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

#include "CommandBuffer.h"

namespace Engine {
    class RenderPass {
    public:
        RenderPass(VkDevice device);
        ~RenderPass();

        void BeginRenderPass(CommandBuffer *commandBuffer);

        void AddColorAttachment(VkFormat format, VkSampleCountFlagBits samples);
        void SetDepthAttachment(VkFormat format, VkSampleCountFlagBits samples);
        void CreateRenderPass();
        VkRenderPass GetRenderPass() const { return renderPass; }

    private:
        VkDevice device;
        VkRenderPass renderPass;

        struct Attachment {
            VkAttachmentDescription description;
            VkAttachmentReference reference;
        };

        std::vector<Attachment> colorAttachments;
        Attachment depthAttachment;
        bool hasDepthAttachment = false;

        void createRenderPassInfo(VkRenderPassCreateInfo& renderPassInfo);
    };

}


#endif // RENDERPASS_H
