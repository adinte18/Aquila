//
// Created by alexa on 08/04/2025.
//

#ifndef CUBEMAPRENDERINGSYSTEM_H
#define CUBEMAPRENDERINGSYSTEM_H

#include <Engine/Device.h>
#include <Engine/Pipeline.h>
#include <ECS/SceneContext.h>

namespace RenderingSystem {
    class EnvToCubemapRenderingSystem {
    public:
        EnvToCubemapRenderingSystem(Engine::Device &device, VkRenderPass renderPass, VkDescriptorSetLayout layout);
        ~EnvToCubemapRenderingSystem() { vkDestroyPipelineLayout(device.vk_GetDevice(), pipelineLayout, nullptr); };

        EnvToCubemapRenderingSystem(const EnvToCubemapRenderingSystem&) = delete;
        EnvToCubemapRenderingSystem& operator=(const EnvToCubemapRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, glm::mat4& viewMatrix);
        void RecreatePipeline(VkRenderPass renderPass);


    private:
        Engine::Device& device;

        std::unique_ptr<Engine::Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
        std::shared_ptr<Engine::Model3D> model;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(VkDescriptorSetLayout layout);

    };
}


#endif //CUBEMAPRENDERINGSYSTEM_H
