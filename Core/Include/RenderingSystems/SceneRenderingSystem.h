//
// Created by alexa on 19/10/2024.
//

#ifndef SCENERENDERINGSYSTEM_H
#define SCENERENDERINGSYSTEM_H
#include <Engine/Device.h>
#include <Engine/Pipeline.h>
#include <ECS/SceneContext.h>


namespace RenderingSystem {
    class SceneRenderingSystem {
    public:
        SceneRenderingSystem(Engine::Device &device, VkRenderPass renderPass, std::array<VkDescriptorSetLayout, 2> setLayouts);
        ~SceneRenderingSystem() { vkDestroyPipelineLayout(device.vk_GetDevice(), pipelineLayout, nullptr); };

        SceneRenderingSystem(const SceneRenderingSystem&) = delete;
        SceneRenderingSystem& operator=(const SceneRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, ECS::SceneContext& scene);
        void RecreatePipeline(VkRenderPass renderPass);


    private:
        Engine::Device& device;

        std::unique_ptr<Engine::Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(std::array<VkDescriptorSetLayout, 2> setLayouts);

    };
}



#endif //SCENERENDERINGSYSTEM_H
