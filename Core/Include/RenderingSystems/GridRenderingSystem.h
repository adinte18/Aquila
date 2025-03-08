//
// Created by alexa on 15/02/2025.
//

#ifndef GRIDRENDERINGSYSTEM_H
#define GRIDRENDERINGSYSTEM_H

#include <Engine/Model.h>

#include "Engine/Device.h"
#include "Engine/Pipeline.h"
#include "Scene.h"


namespace RenderingSystem {
    class GridRenderingSystem {
    public:
        GridRenderingSystem(Engine::Device &device, VkRenderPass renderPass, std::array<VkDescriptorSetLayout, 2> setLayouts);
        ~GridRenderingSystem() { vkDestroyPipelineLayout(device.vk_GetDevice(), pipelineLayout, nullptr); };

        GridRenderingSystem(const GridRenderingSystem&) = delete;
        GridRenderingSystem& operator=(const GridRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, ECS::Scene& scene);

    private:
        Engine::Device& device;
        std::shared_ptr<Engine::Model3D> model = Engine::Model3D::create(device);

        std::unique_ptr<Engine::Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(std::array<VkDescriptorSetLayout, 2> setLayouts);

    };
}



#endif //GRIDRENDERINGSYSTEM_H
