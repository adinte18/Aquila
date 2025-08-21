//
// Created by alexa on 10/04/2025.
//

#ifndef IBLRENDERINGSYSTEM_H
#define IBLRENDERINGSYSTEM_H

#include "Engine/Mesh.h"
#include <Engine/Renderer/Device.h>
#include <Engine/Renderer/Pipeline.h>



namespace Engine {
    class CubemapRenderingSystem {
    public:
        CubemapRenderingSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout layout);
        ~CubemapRenderingSystem() { vkDestroyPipelineLayout(device.GetDevice(), pipelineLayout, nullptr); };

        CubemapRenderingSystem(const CubemapRenderingSystem&) = delete;
        CubemapRenderingSystem& operator=(const CubemapRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, glm::mat4& viewMatrix, glm::mat4& projectionMatrix);
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



#endif //IBLRENDERINGSYSTEM_H
