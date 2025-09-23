#ifndef SCENE_RENDER_SYSTEM_H
#define SCENE_RENDER_SYSTEM_H

#include "Engine/EditorCamera.h"
#include "Engine/Renderer/Buffer.h"
#include "RenderingSystems/RenderingSystemBase.h"

namespace Engine {
class SceneRenderSystem : public RenderingSystemBase {
public:
  struct alignas(16) LightData {
    vec3 color;
    f32 intensity;
    vec3 direction;
    f32 range;
    vec3 position;
    int type;
    f32 innerCone;
    f32 outerCone;
    int isActive;
  };

  struct LightUniformData {
    LightData lights[32];
    int lightCount; // 4 bytes
  };

  struct SceneUniformData {
    alignas(16) glm::mat4 projection{1.f};
    alignas(16) glm::mat4 view{1.f};
    alignas(16) glm::mat4 inverseView{1.f};
  };

  SceneRenderSystem(Device &device, VkRenderPass renderPass);
  ~SceneRenderSystem() override = default;

  SceneRenderSystem(const SceneRenderSystem &) = delete;
  SceneRenderSystem &operator=(const SceneRenderSystem &) = delete;

  void Render(const FrameSpec &context) override;
  void Update(EditorCamera &camera);

private:
  void CreateDescriptorSetLayout() override;
  void CreatePipeline(VkRenderPass renderPass) override;
  void CreatePipelineLayout() override;

  Unique<Buffer> m_Buffer;
  Unique<Buffer> m_LightBuffer;
};
}; // namespace Engine

#endif