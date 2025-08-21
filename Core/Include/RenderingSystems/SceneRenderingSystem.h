//
// Created by alexa on 19/10/2024.
//

#ifndef SCENERENDERINGSYSTEM_H
#define SCENERENDERINGSYSTEM_H
#include <Engine/Renderer/Device.h>
#include <Engine/Renderer/Pipeline.h>



namespace Engine {
    class SceneRenderingSystem {
    public:
        SceneRenderingSystem(Device &device, VkRenderPass renderPass, std::array<VkDescriptorSetLayout, 2> setLayouts);
        ~SceneRenderingSystem() { vkDestroyPipelineLayout(device.GetDevice(), pipelineLayout, nullptr); };

        SceneRenderingSystem(const SceneRenderingSystem&) = delete;
        SceneRenderingSystem& operator=(const SceneRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer);
        void RecreatePipeline(VkRenderPass renderPass);


    private:
        Device& device;

        Unique<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(std::array<VkDescriptorSetLayout, 2> setLayouts);

    };
}



#endif //SCENERENDERINGSYSTEM_H
