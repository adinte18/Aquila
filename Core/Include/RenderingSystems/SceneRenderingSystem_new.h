#ifndef SCENE_RENDER_SYSTEM_H
#define SCENE_RENDER_SYSTEM_H

#include "RenderingSystems/RenderingSystemBase.h"
#include "Engine/Renderer/Buffer.h"
#include "Engine/EditorCamera.h"
namespace Engine{
    class SceneRenderSystem : public RenderingSystemBase {
        public:
            struct alignas(16) LightData {
                glm::vec3 color;
                float intensity;
                glm::vec3 direction;
                float range;
                glm::vec3 position;
                int type;
                float innerCone;
                float outerCone;
                int isActive;
            };
            
            struct SceneUniformData{
                alignas(16) glm::mat4 projection{1.f};
                alignas(16) glm::mat4 view{1.f};
                alignas(16) glm::mat4 inverseView{1.f};
            };

            SceneRenderSystem(Device& device, VkRenderPass renderPass);
            ~SceneRenderSystem() override = default;

            SceneRenderSystem(const SceneRenderSystem&) = delete;
            SceneRenderSystem& operator=(const SceneRenderSystem&) = delete;

            void Render(const FrameSpec& context) override;
            void UpdateBuffer(EditorCamera& camera);
            void UpdateLights();

        private:
            void CreateDescriptorSetLayout() override;
            void CreatePipeline(VkRenderPass renderPass) override;
            void CreatePipelineLayout() override;
            
            Unique<Buffer> m_Buffer;
            Unique<Buffer> m_LightBuffer;
    };
};

#endif