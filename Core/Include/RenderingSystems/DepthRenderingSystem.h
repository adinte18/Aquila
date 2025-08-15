#ifndef DEPTHRENDERINGSYSTEM_H
#define DEPTHRENDERINGSYSTEM_H
#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Pipeline.h"
#include "Engine/Mesh.h"




namespace Engine {
    class DepthRenderingSystem {
    public:
        DepthRenderingSystem(Device &device, VkRenderPass renderPass, std::array<VkDescriptorSetLayout, 2> setLayouts);
        ~DepthRenderingSystem() { vkDestroyPipelineLayout(device.GetDevice(), pipelineLayout, nullptr); };

        DepthRenderingSystem(const DepthRenderingSystem&) = delete;
        DepthRenderingSystem& operator=(const DepthRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer) const;
        void RecreatePipeline(VkRenderPass renderPass);

    private:
        Device& device;

        Unique<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(std::array<VkDescriptorSetLayout, 2> setLayouts);

    };
}



#endif //DepthRenderingSystem_H
