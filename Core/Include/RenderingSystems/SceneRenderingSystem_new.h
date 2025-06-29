#ifndef SCENE_RENDER_SYSTEM_H
#define SCENE_RENDER_SYSTEM_H

#include "RenderingSystems/RenderingSystemBase.h"
#include "Scene/Scene.h"

namespace Engine{
    class SceneRenderingSystem_new : public RenderingSystemBase {
        public:
            struct SceneUniformData{
                alignas(16) glm::mat4 projection{1.f};
                alignas(16) glm::mat4 view{1.f};
                alignas(16) glm::mat4 inverseView{1.f};
            };

            SceneRenderingSystem_new(Device& device, VkRenderPass renderPass);
            ~SceneRenderingSystem_new() override = default;

            SceneRenderingSystem_new(const SceneRenderingSystem_new&) = delete;
            SceneRenderingSystem_new& operator=(const SceneRenderingSystem_new&) = delete;

            void Render(const RenderContext& context) override;
            void UpdateBuffer(const RenderContext& context);

        private:
            void CreateDescriptorSetLayout() override;
            void CreatePipeline(VkRenderPass renderPass) override;
            void CreatePipelineLayout() override;
            
            Unique<Buffer> m_Buffer;
    };
};

#endif