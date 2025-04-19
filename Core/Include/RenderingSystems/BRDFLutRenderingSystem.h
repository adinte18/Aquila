//
// Created by alexa on 18/04/2025.
//

#ifndef BRDFLUTRENDERINGSYSTEM_H
#define BRDFLUTRENDERINGSYSTEM_H



#include "Engine/Device.h"
#include "Engine/Pipeline.h"
#include "Scene.h"


namespace RenderingSystem {
    class BRDFLutRenderingSystem {
    public:
        BRDFLutRenderingSystem(Engine::Device &device, VkRenderPass renderPass, VkDescriptorSetLayout layout);
        ~BRDFLutRenderingSystem() { vkDestroyPipelineLayout(device.vk_GetDevice(), pipelineLayout, nullptr); };

        BRDFLutRenderingSystem(const BRDFLutRenderingSystem&) = delete;
        BRDFLutRenderingSystem& operator=(const BRDFLutRenderingSystem&) = delete;

        void Render(VkCommandBuffer commandBuffer, VkDescriptorSet &descriptorSet);
        void RecreatePipeline(VkRenderPass renderPass);
    private:
        Engine::Device& device;

        std::unique_ptr<Engine::Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;


        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineLayout(VkDescriptorSetLayout layout);

    };
}




#endif //BRDFLUTRENDERINGSYSTEM_H
