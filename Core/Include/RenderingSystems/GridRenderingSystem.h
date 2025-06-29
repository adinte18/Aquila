//
// Created by alexa on 15/02/2025.
//

#ifndef GRIDRENDERINGSYSTEM_H
#define GRIDRENDERINGSYSTEM_H

#include "Engine/Mesh.h"

#include "Engine/Device.h"
#include "Engine/Pipeline.h"
#include "ECS/SceneContext.h"


namespace Engine {
    class GridRenderingSystem {
    public:
        GridRenderingSystem(Device &device, VkRenderPass renderPass, std::array<VkDescriptorSetLayout, 2> setLayouts);
        ~GridRenderingSystem() { vkDestroyPipelineLayout(device.vk_GetDevice(), pipelineLayout, nullptr); };

        GridRenderingSystem(const GridRenderingSystem&) = delete;
        GridRenderingSystem& operator=(const GridRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, SceneContext& scene) const;
        void RecreatePipeline(VkRenderPass renderPass);

    private:
        Device& device;
        // Ref<Mesh> model = Mesh::create(device);

        Unique<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(std::array<VkDescriptorSetLayout, 2> setLayouts);

    };
}



#endif //GRIDRENDERINGSYSTEM_H
