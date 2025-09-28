#ifndef GIZMO_RENDER_SYSTEM_H
#define GIZMO_RENDER_SYSTEM_H

#include "AquilaCore.h"
#include "Engine/EditorCamera.h"
#include "Engine/Renderer/Buffer.h"
#include "Engine/Renderer/Vertex.h"
#include "RenderingSystems/RenderingSystemBase.h"

namespace Engine {
class GizmoRenderSystem : public RenderingSystemBase {
public:
  struct GizmoUniformData {
    mat4 view;
    mat4 projection;
  };

  GizmoRenderSystem(Device &device, VkRenderPass renderPass);
  ~GizmoRenderSystem() override = default;

  AQUILA_NONCOPYABLE(GizmoRenderSystem);

  void Update(EditorCamera &camera);
  void Render(const FrameSpec &frameSpec) override;

  void Clear();
  void AddLine(const vec3 &start, const vec3 &end,
               const vec3 &color = vec3(1.0f));
  void AddWireframeBox(const vec3 &center, const vec3 &size,
                       const glm::quat &rotation, const vec3 &color);
  void AddCoordinateAxes(const vec3 &origin, float length = 1.0f);
  void AddCameraFrustum(const mat4 &viewMatrix, const mat4 &projMatrix,
                        const vec3 &color, const vec3 &cameraPos);

private:
  Unique<Buffer> m_Buffer;
  std::vector<Unique<Buffer>> m_VertexBuffers;
  std::vector<Unique<Buffer>> m_IndexBuffers;

  std::vector<Vertex> m_Vertices;
  std::vector<uint32> m_Indices;

  uint32 m_CurrentVertexCount = 0;
  uint32 m_CurrentIndexCount = 0;
  bool m_BuffersNeedUpdate = false;

  void UpdateBuffers(uint32 frameIndex);

  void CreateDescriptorSetLayout() override;
  void CreatePipeline(VkRenderPass renderPass) override;
  void CreatePipelineLayout() override;
};

} // namespace Engine

#endif