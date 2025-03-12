#ifndef PPRenderingSystem_H
#define PPRenderingSystem_H
#include "Engine/Device.h"
#include "Engine/Pipeline.h"
#include "Scene.h"


namespace RenderingSystem {
    class PPRenderingSystem {
    public:
        PPRenderingSystem(Engine::Device &device, VkRenderPass renderPass, VkDescriptorSetLayout layout);
        ~PPRenderingSystem() { vkDestroyPipelineLayout(device.vk_GetDevice(), pipelineLayout, nullptr); };

        PPRenderingSystem(const PPRenderingSystem&) = delete;
        PPRenderingSystem& operator=(const PPRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, VkDescriptorSet &descriptorSet);

    private:
        Engine::Device& device;

        std::unique_ptr<Engine::Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(VkDescriptorSetLayout layout);

    };
}



#endif //PPRenderingSystem_H
