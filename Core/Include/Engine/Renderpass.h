#ifndef RENDERPASS_H
#define RENDERPASS_H

#include <unordered_map>
#include "Engine/Device.h"
#include "Engine/Rendertarget.h"
#include "Engine/Descriptor.h"

namespace Engine {

    enum class RenderPassType {
        G_BUFFER,
        GEOMETRY,
        SHADOW,          
        SSAO,            
        POST_PROCESSING,
        GRID,
        FINAL
    };
    
    class Renderpass {
        public:
            Renderpass(Device &device, VkExtent2D extent): device(device), extent(extent) {};
            ~Renderpass() {
                DestroyAll();
            }

            void CreateRenderPass(RenderPassType type);

            void CreateFramebuffer(RenderPassType type);
            void CreateFramebuffer(RenderPassType type, int cascades);

            void CreateRenderTarget(Device& device, uint32_t width, uint32_t height);
            void WriteImageToDescriptor(RenderPassType type);
            [[nodiscard]] VkRenderPass GetRenderPass(RenderPassType type) const;
            [[nodiscard]] VkFramebuffer GetFramebuffer(RenderPassType type) const;
            [[nodiscard]] VkImage GetImage(RenderPassType type) const;
            [[nodiscard]] VkImageView GetRenderTarget(RenderPassType type) const;
            [[nodiscard]] VkSampler GetRenderSampler(RenderPassType type) const;
            [[nodiscard]] VkDescriptorSet GetDescriptorSet(RenderPassType) const;

            void WriteDepthImageToDescriptor(RenderPassType type, DescriptorSetLayout &sceneSetLayout, DescriptorPool &descriptorPool);

            void UpdateExtent(const VkExtent2D extent) {
                this->extent = extent;
            }

            void DestroyAll();

            void SetExtent(VkExtent2D extent);

        private:
            Device& device;
            VkExtent2D extent;
        
            std::unordered_map<RenderPassType, VkRenderPass> renderPasses;
            std::unordered_map<RenderPassType, VkFramebuffer> framebuffers;
            std::unordered_map<RenderPassType, std::shared_ptr<RenderTarget>> renderTargets;
            std::unordered_map<RenderPassType, VkDescriptorSet> imageToViewport;
            std::unordered_map<RenderPassType, std::unique_ptr<DescriptorPool>> descriptorPools;
            std::unordered_map<RenderPassType, std::unique_ptr<DescriptorSetLayout>> descriptorSetLayouts;

            std::unique_ptr<DescriptorPool> descriptorPool;
            std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;

            void CreateGBufferPass();
            void CreateShadowPass();
            void CreateSSAOPass();
            void CreatePostProcessingPass();

            void CreateGridPass();

            void CreateGeometryPass();

            void CreateFinalPass();
    };
        
};

#endif //RENDERPASS_H