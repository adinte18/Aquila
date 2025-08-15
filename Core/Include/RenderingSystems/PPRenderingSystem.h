#ifndef PPRenderingSystem_H
#define PPRenderingSystem_H
#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Pipeline.h"


namespace Engine {
    class PPRenderingSystem {
    public:
        PPRenderingSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout layout);
        ~PPRenderingSystem() { vkDestroyPipelineLayout(device.GetDevice(), pipelineLayout, nullptr); };

        PPRenderingSystem(const PPRenderingSystem&) = delete;
        PPRenderingSystem& operator=(const PPRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, VkDescriptorSet &descriptorSet);
        void RecreatePipeline(VkRenderPass renderPass);

    private:
        Device& device;

        Unique<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(VkDescriptorSetLayout layout);

    };
}



#endif //PPRenderingSystem_H
