#ifndef GIZMO_RENDER_SYSTEM_H
#define GIZMO_RENDER_SYSTEM_H

#include "Aquila/Core/AquilaCore.h"
#include "Aquila/Rendering/Camera.h"
#include "Aquila/Graphics/Resources/Buffer.h"
#include "Aquila/Graphics/Resources/Vertex.h"
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/Scene/Scene.h"

namespace Aquila::Rendering::Systems {

using namespace Aquila::Graphics;
using namespace Aquila::Graphics::RenderingPipeline;
using namespace Aquila::Graphics::Resources;

class GizmoRenderSystem final : public RenderingSystemBase {
  public:
	struct GizmoUniformData {
		mat4 view;
		mat4 projection;
	};

	GizmoRenderSystem(Device &device, const std::vector<Ref<DescriptorSetLayout>> &layouts);
	~GizmoRenderSystem() override = default;

	AQUILA_NONCOPYABLE(GizmoRenderSystem);

	void OnUpdate(const FrameSpec &frameSpec) override;
	void OnRender(const FrameSpec &frameSpec) override;
	void OnEvent(Events::Event &event) override;

	void Clear();
	void AddWireframeBox(const vec3 &center, const vec3 &size, const glm::quat &rotation, const vec3 &color);
	void AddCoordinateAxes(const vec3 &origin, f32 length = 1.0f);
	void AddCameraFrustum(const mat4 &viewMatrix, const mat4 &projMatrix, const vec3 &color, const vec3 &cameraPos);
	void AddPointLightGizmo(const vec3 &position, f32 range, const vec3 &color);
	void AddDirectionalLightGizmo(const vec3 &position, const vec3 &direction, const vec3 &color);
	void AddSpotLightGizmo(const vec3 &position, const vec3 &direction, f32 range, f32 innerAngle, f32 outerAngle,
						   const vec3 &color);
	bool HasGeometry() const { return !m_Vertices.empty(); }

  private:
	void AddCircle(const vec3 &center, const vec3 &normal, f32 radius, const vec3 &color, int segments = 256);
	void AddCone(const vec3 &apex, const vec3 &direction, f32 length, f32 angle, const vec3 &color, int segments = 256);
	void AddSphere(const vec3 &center, f32 radius, const vec3 &color, int segments = 256);
	void AddLine(const vec3 &start, const vec3 &end, const vec3 &color = vec3(1.0f));

	static constexpr uint32 MAX_VERTICES = 10000;
	static constexpr uint32 MAX_INDICES = 20000;
	static constexpr int LOW_DETAIL_SEGMENTS = 8;
	static constexpr int MEDIUM_DETAIL_SEGMENTS = 16;
	static constexpr int HIGH_DETAIL_SEGMENTS = 32;

	std::vector<Unique<Buffer>> m_UniformBuffers;
	std::vector<Unique<Buffer>> m_VertexBuffers;
	std::vector<Unique<Buffer>> m_IndexBuffers;

	std::vector<Vertex> m_Vertices;
	std::vector<uint32> m_Indices;

	uint32 m_CurrentVertexCount = 0;
	uint32 m_CurrentIndexCount = 0;
	bool m_BuffersNeedUpdate = false;
	bool IsInFrustum(const vec3 &position, f32 radius, Camera &camera) const;
	void UpdateBuffers(uint32 frameIndex);
	int GetLODSegments(f32 distance) const;
	void CreatePipeline(const PipelineRenderingFormats &renderingFormats) override;
	void CreatePipelineLayout() override;
};

} // namespace Aquila::Rendering::Systems

#endif
