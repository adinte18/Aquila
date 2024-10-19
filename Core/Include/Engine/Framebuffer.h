#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <vector>
#include <stdexcept>

#include "Descriptor.h"
#include "Engine/Device.h"

namespace Engine {
    class Framebuffer {
    public:
        Framebuffer(Device &device, VkRenderPass renderPass);

        ~Framebuffer();

        Framebuffer& operator=(const Framebuffer&) = delete;
        Framebuffer(const Framebuffer&) = delete;

        Framebuffer&& operator=(Framebuffer&& other) = delete;
        Framebuffer(Framebuffer&& other) = delete;

        void CreateFramebuffer(VkExtent2D extent, VkRenderPass renderPass);
        void DestroyFramebuffer();
        void Resize(VkExtent2D newExtent);

        [[nodiscard]] VkExtent2D GetExtent() const { return extent; }

        [[nodiscard]] VkFramebuffer GetFramebuffer() const { return framebuffer; }

        [[nodiscard]] VkDescriptorSet GetDescriptorSet() const { return descriptorSet; }

    private:
        VkDescriptorSet descriptorSet{};

        std::shared_ptr<DescriptorPool> descriptorPool;
        std::shared_ptr<DescriptorSetLayout> descriptorSetLayout;

        Device& device;
        VkRenderPass fbRenderPass{};
        VkExtent2D extent{};
        VkFramebuffer framebuffer{};

        VkImage colorImage{};
        VkDeviceMemory colorImageMemory{};
        VkImageView colorImageView{};
        VkSampler colorSampler{};

        VkImage depthImage{};
        VkDeviceMemory depthImageMemory{};
        VkImageView depthImageView{};
        VkSampler depthSampler{};
    };
}

#endif // FRAMEBUFFER_H
