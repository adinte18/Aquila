//
// Created by alexa on 21/04/2025.
//

#ifndef PREETHAMSKYRENDERINGSYSTEM_H
#define PREETHAMSKYRENDERINGSYSTEM_H



#include <Engine/Device.h>
#include <Engine/Pipeline.h>
#include <ECS/SceneContext.h>

namespace Engine {
    class PreethamSkyRenderingSystem {
    public:
        PreethamSkyRenderingSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout layout);
        ~PreethamSkyRenderingSystem() { vkDestroyPipelineLayout(device.vk_GetDevice(), pipelineLayout, nullptr); };

        PreethamSkyRenderingSystem(const PreethamSkyRenderingSystem&) = delete;
        PreethamSkyRenderingSystem& operator=(const PreethamSkyRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, glm::mat4& viewMatrix);
        void RecreatePipeline(VkRenderPass renderPass);


    private:
        Device& device;

        Unique<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
        Ref<Mesh> model;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(VkDescriptorSetLayout layout);

    };
}



#endif //PREETHAMSKYRENDERINGSYSTEM_H
