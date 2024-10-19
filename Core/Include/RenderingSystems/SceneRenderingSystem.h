//
// Created by alexa on 19/10/2024.
//

#ifndef SCENERENDERINGSYSTEM_H
#define SCENERENDERINGSYSTEM_H
#include <Engine/Device.h>
#include <Engine/Pipeline.h>


namespace RenderingSystem {
    class SceneRenderingSystem {
    public:
        SceneRenderingSystem(Engine::Device &device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> setLayouts);
        ~SceneRenderingSystem();

        SceneRenderingSystem(const SceneRenderingSystem&) = delete;
        SceneRenderingSystem& operator=(const SceneRenderingSystem&) = delete;

        void Render();

    private:
        Engine::Device& device;

        std::unique_ptr<Engine::Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(std::vector<VkDescriptorSetLayout> setLayouts);

    };
}



#endif //SCENERENDERINGSYSTEM_H
