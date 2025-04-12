#ifndef DEPTHRENDERINGSYSTEM_H
#define DEPTHRENDERINGSYSTEM_H
#include <Engine/Device.h>
#include <Engine/Pipeline.h>
#include <SceneContext.h>
#include <Engine/Model.h>
#include "Components.h"


namespace RenderingSystem {
    class DepthRenderingSystem {
    public:
        DepthRenderingSystem(Engine::Device &device, VkRenderPass renderPass, std::array<VkDescriptorSetLayout, 2> setLayouts);
        ~DepthRenderingSystem() { vkDestroyPipelineLayout(device.vk_GetDevice(), pipelineLayout, nullptr); };

        DepthRenderingSystem(const DepthRenderingSystem&) = delete;
        DepthRenderingSystem& operator=(const DepthRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, ECS::SceneContext &scene) const;
        void RecreatePipeline(VkRenderPass renderPass);

    private:
        Engine::Device& device;

        std::unique_ptr<Engine::Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(std::array<VkDescriptorSetLayout, 2> setLayouts);

    };
}



#endif //DepthRenderingSystem_H
