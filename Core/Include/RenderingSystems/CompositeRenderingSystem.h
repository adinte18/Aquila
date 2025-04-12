#ifndef CompositeRenderingSystem_H
#define CompositeRenderingSystem_H
#include "Engine/Device.h"
#include "Engine/Pipeline.h"
#include "Scene.h"


namespace RenderingSystem {
    class CompositeRenderingSystem {
    public:
        CompositeRenderingSystem(Engine::Device &device, VkRenderPass renderPass, VkDescriptorSetLayout layout);
        ~CompositeRenderingSystem() { vkDestroyPipelineLayout(device.vk_GetDevice(), pipelineLayout, nullptr); };

        CompositeRenderingSystem(const CompositeRenderingSystem&) = delete;
        CompositeRenderingSystem& operator=(const CompositeRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, VkDescriptorSet &descriptorSet);
        void RecreatePipeline(VkRenderPass renderPass);
    private:
        Engine::Device& device;

        std::unique_ptr<Engine::Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(VkDescriptorSetLayout layout);

    };
}



#endif //CompositeRenderingSystem_H
