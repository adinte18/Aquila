#include "Aquila/Rendering/Systems/GizmoRenderingSystem.h"
#include "Aquila/Events/Event.h"
#include "Aquila/Rendering/Camera.h"
#include "Aquila/Graphics/Core/Swapchain.h"
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/EntityManager.h"

namespace Aquila::Rendering::Systems {

GizmoRenderSystem::GizmoRenderSystem(Device &device, const std::vector<Ref<DescriptorSetLayout>> &layouts)
	: RenderingSystemBase(device) {
	m_Layouts = layouts;

	CreatePipelineLayout();

	auto gizmoFormat = PipelineRenderingFormats::SingleColor(VK_FORMAT_R8G8B8A8_UNORM);
	CreatePipeline(gizmoFormat);

	m_VertexBuffers.resize(SharedConstants::MAX_FRAMES_IN_FLIGHT);
	m_IndexBuffers.resize(SharedConstants::MAX_FRAMES_IN_FLIGHT);

	m_Vertices.reserve(10000);
	m_Indices.reserve(20000);
}

void GizmoRenderSystem::CreatePipeline(const PipelineRenderingFormats &renderingFormats) {
	PipelineConfigInfo pipelineConfig{};
	Pipeline::DefaultPipelineConfig(pipelineConfig);

	pipelineConfig.pipelineLayout = m_PipelineLayout;
	pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	pipelineConfig.rasterizationInfo.lineWidth = 2.F;
	pipelineConfig.colorBlendAttachments[0].blendEnable = VK_FALSE;

	// render gizmos on top
	pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
	pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;

	pipelineConfig.bindingDescriptions.clear();
	pipelineConfig.attributeDescriptions.clear();

	VkVertexInputBindingDescription binding{};
	binding.binding = 0;
	binding.stride = sizeof(Vertex);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	pipelineConfig.bindingDescriptions.push_back(binding);

	pipelineConfig.attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
	pipelineConfig.attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3) });

	pipelineConfig.colorFormats = renderingFormats.colorFormats;
	pipelineConfig.depthFormat = renderingFormats.depthFormat;
	pipelineConfig.stencilFormat = VK_FORMAT_UNDEFINED;

	VkPipelineRenderingCreateInfo renderingCreateInfo{};
	renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingCreateInfo.colorAttachmentCount = static_cast<uint32>(pipelineConfig.colorFormats.size());
	renderingCreateInfo.pColorAttachmentFormats =
		pipelineConfig.colorFormats.empty() ? nullptr : pipelineConfig.colorFormats.data();
	renderingCreateInfo.depthAttachmentFormat = pipelineConfig.depthFormat;
	renderingCreateInfo.stencilAttachmentFormat = pipelineConfig.stencilFormat;

	pipelineConfig.pNext = &renderingCreateInfo;

	m_Pipeline = CreateUnique<Pipeline>(device, std::string(IMMUTABLE_SHADERS_PATH) + "Gizmo.slang", pipelineConfig);
}

void GizmoRenderSystem::CreatePipelineLayout() {
	auto *setLayout = m_Layouts[0]->GetDescriptorSetLayout();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &setLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create gizmo pipeline layout!");
	}
}

bool GizmoRenderSystem::IsInFrustum(const vec3 &position, f32 radius, Camera &camera) const {
	mat4 vp = camera.GetProjection() * camera.GetView();

	vec4 leftPlane = vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);
	vec4 rightPlane = vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);
	vec4 bottomPlane = vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);
	vec4 topPlane = vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);
	vec4 nearPlane = vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]);
	vec4 farPlane = vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

	vec4 planes[] = { leftPlane, rightPlane, bottomPlane, topPlane, nearPlane, farPlane };

	for (const auto &plane : planes) {
		vec3 normal = vec3(plane);
		float dist = glm::dot(normal, position) + plane.w;

		if (dist < -radius) {
			return false;
		}
	}

	return true;
}

void GizmoRenderSystem::OnEvent(Events::Event &event) {}

void GizmoRenderSystem::OnUpdate(const FrameSpec &frameSpec) {
	if (frameSpec.scene == nullptr) {
		return;
	}

	UpdateDescriptorsForScene(frameSpec);

	Clear();

	frameSpec.scene->GetEntityManager()
		->ForEach<SceneManagement::Components::MetadataComponent, SceneManagement::Components::TransformComponent>(
			[&](SceneManagement::Entity entity, const SceneManagement::Components::MetadataComponent &metadata,
				const SceneManagement::Components::TransformComponent &transform) {
				if (!metadata.IsVisible()) {
					return;
				}

				vec3 pos = transform.GetWorldPosition();

				// if (!IsInFrustum(pos, 50.0f, camera)) return;

				vec3 scale = transform.GetWorldScale();
				quaternion rotation = transform.GetWorldRotation();

				// Mesh gizmos (selection wireframes)
				if (entity.HasAnyComponent<SceneManagement::Components::MeshComponent>()) {
					if (metadata.IsSelected()) {
						AddWireframeBox(pos, scale * 2.0f, rotation, vec3(0.259f, 0.961f, 0.471f));
					}
				}

				// Light gizmos
				if (entity.HasAnyComponent<SceneManagement::Components::LightComponent>()) {
					auto &light = entity.GetComponent<SceneManagement::Components::LightComponent>();

					if (!light.IsActive()) {
						return;
					}

					vec3 gizmoColor = light.GetColor() * light.GetIntensity() * 0.5f;
					gizmoColor = glm::clamp(gizmoColor, vec3(0.3f), vec3(1.0f));

					switch (light.GetType()) {
					case SceneManagement::Components::LightComponent::Type::Point:
						AddPointLightGizmo(pos, light.GetRange(), gizmoColor);
						break;

					case SceneManagement::Components::LightComponent::Type::Directional:
						AddDirectionalLightGizmo(pos, light.GetDirection(), gizmoColor);
						break;

					case SceneManagement::Components::LightComponent::Type::Spot:
						AddSpotLightGizmo(pos, light.GetDirection(), light.GetRange(), light.GetInnerConeAngle(),
										  light.GetOuterConeAngle(), gizmoColor);
						break;
					}
				}

				// Camera gizmos (frustum visualization)
				if (entity.HasAnyComponent<SceneManagement::Components::CameraComponent>()) {
					auto &camera = entity.GetComponent<SceneManagement::Components::CameraComponent>();
					mat4 viewMatrix = camera.GetViewMatrix(transform.GetLocalPosition(), transform.GetLocalRotation());
					mat4 projMatrix = camera.GetProjectionMatrix();
					AddCameraFrustum(viewMatrix, projMatrix, vec3(1.0f, 1.0f, 0.0f), transform.GetLocalPosition());
				}
			});

	m_BuffersNeedUpdate = true;
}

void GizmoRenderSystem::OnRender(const FrameSpec &frameSpec) {
	if (frameSpec.scene == nullptr) {
		return;
	}

	if (m_Vertices.empty()) {
		return;
	}

	if (m_BuffersNeedUpdate) {
		UpdateBuffers(frameSpec.frameIndex);
		m_BuffersNeedUpdate = false;
	}

	m_Pipeline->Bind(frameSpec.commandBuffer);

	vkCmdSetLineWidth(frameSpec.commandBuffer, 2.0f);

	vkCmdBindDescriptorSets(frameSpec.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1,
							&frameSpec.cameraDescriptorSet, 0, nullptr);

	const VkBuffer vertexBuffers[] = { m_VertexBuffers[frameSpec.frameIndex]->GetBuffer() };
	constexpr VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(frameSpec.commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(frameSpec.commandBuffer, m_IndexBuffers[frameSpec.frameIndex]->GetBuffer(), 0,
						 VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(frameSpec.commandBuffer, m_CurrentIndexCount, 1, 0, 0, 0);
}

void GizmoRenderSystem::Clear() {
	m_Vertices.clear();
	m_Indices.clear();
	m_CurrentVertexCount = 0;
	m_CurrentIndexCount = 0;
	m_BuffersNeedUpdate = true;
}

void GizmoRenderSystem::AddLine(const vec3 &start, const vec3 &end, const vec3 &color) {
	uint32_t startIndex = m_CurrentVertexCount;

	Vertex startVertex{};
	startVertex.pos = start;
	startVertex.color = color;

	Vertex endVertex{};
	endVertex.pos = end;
	endVertex.color = color;

	m_Vertices.push_back(startVertex);
	m_Vertices.push_back(endVertex);

	m_Indices.push_back(startIndex);
	m_Indices.push_back(startIndex + 1);

	m_CurrentVertexCount += 2;
	m_CurrentIndexCount += 2;
	m_BuffersNeedUpdate = true;
}

void GizmoRenderSystem::AddWireframeBox(const vec3 &center, const vec3 &size, const quaternion &rotation,
										const vec3 &color) {
	const vec3 halfSize = size * 0.5f;
	vec3 corners[8] = { vec3(-halfSize.x, -halfSize.y, -halfSize.z), vec3(halfSize.x, -halfSize.y, -halfSize.z),
						vec3(halfSize.x, -halfSize.y, halfSize.z),	 vec3(-halfSize.x, -halfSize.y, halfSize.z),
						vec3(-halfSize.x, halfSize.y, -halfSize.z),	 vec3(halfSize.x, halfSize.y, -halfSize.z),
						vec3(halfSize.x, halfSize.y, halfSize.z),	 vec3(-halfSize.x, halfSize.y, halfSize.z) };

	for (auto &corner : corners) {
		corner = center + (rotation * corner);
	}

	// Bottom face
	AddLine(corners[0], corners[1], color);
	AddLine(corners[1], corners[2], color);
	AddLine(corners[2], corners[3], color);
	AddLine(corners[3], corners[0], color);

	// Top face
	AddLine(corners[4], corners[5], color);
	AddLine(corners[5], corners[6], color);
	AddLine(corners[6], corners[7], color);
	AddLine(corners[7], corners[4], color);

	// Vertical edges
	AddLine(corners[0], corners[4], color);
	AddLine(corners[1], corners[5], color);
	AddLine(corners[2], corners[6], color);
	AddLine(corners[3], corners[7], color);
}

void GizmoRenderSystem::AddCoordinateAxes(const vec3 &origin, f32 length) {
	AddLine(origin, origin + vec3(length, 0, 0), vec3(1, 0, 0));
	AddLine(origin, origin + vec3(0, length, 0), vec3(0, 1, 0));
	AddLine(origin, origin + vec3(0, 0, length), vec3(0, 0, 1));
}

void GizmoRenderSystem::AddPointLightGizmo(const vec3 &position, f32 /*range*/, const vec3 &color) {
	constexpr float axisLength = 0.5f;
	AddLine(position - vec3(axisLength, 0, 0), position + vec3(axisLength, 0, 0), color);
	AddLine(position - vec3(0, axisLength, 0), position + vec3(0, axisLength, 0), color);
	AddLine(position - vec3(0, 0, axisLength), position + vec3(0, 0, axisLength), color);
}

void GizmoRenderSystem::AddDirectionalLightGizmo(const vec3 &position, const vec3 &direction, const vec3 &color) {
	vec3 normalizedDir = glm::normalize(direction);
	f32 length = 2.0f;

	AddCircle(position, normalizedDir, 0.5f, color, 256);

	vec3 arrowEnd = position + normalizedDir * length;
	AddLine(position, arrowEnd, color);

	vec3 perpendicular1;
	vec3 perpendicular2;
	if (abs(normalizedDir.y) < 0.9f) {
		perpendicular1 = glm::normalize(glm::cross(normalizedDir, vec3(0, 1, 0)));
	} else {
		perpendicular1 = glm::normalize(glm::cross(normalizedDir, vec3(1, 0, 0)));
	}
	perpendicular2 = glm::normalize(glm::cross(normalizedDir, perpendicular1));

	f32 arrowSize = 0.3f;
	vec3 arrowBase = arrowEnd - normalizedDir * arrowSize;

	vec3 arrowPoint1 = arrowBase + perpendicular1 * arrowSize * 0.5f;
	vec3 arrowPoint2 = arrowBase - perpendicular1 * arrowSize * 0.5f;
	vec3 arrowPoint3 = arrowBase + perpendicular2 * arrowSize * 0.5f;
	vec3 arrowPoint4 = arrowBase - perpendicular2 * arrowSize * 0.5f;

	AddLine(arrowEnd, arrowPoint1, color);
	AddLine(arrowEnd, arrowPoint2, color);
	AddLine(arrowEnd, arrowPoint3, color);
	AddLine(arrowEnd, arrowPoint4, color);

	for (int i = 0; i < 4; i++) {
		f32 parallelOffset = 0.3f;
		vec3 offset = vec3(0);
		if (i == 0) {
			offset = perpendicular1 * parallelOffset + perpendicular2 * parallelOffset;
		} else if (i == 1) {
			offset = -perpendicular1 * parallelOffset + perpendicular2 * parallelOffset;
		} else if (i == 2) {
			offset = perpendicular1 * parallelOffset - perpendicular2 * parallelOffset;
		} else {
			offset = -perpendicular1 * parallelOffset - perpendicular2 * parallelOffset;
		}

		vec3 lineStart = position + offset - normalizedDir * 0.5f;
		vec3 lineEnd = lineStart + normalizedDir * (length - 0.5f);
		AddLine(lineStart, lineEnd, color * 0.7f);
	}
}

void GizmoRenderSystem::AddSpotLightGizmo(const vec3 &position, const vec3 &direction, f32 range, f32 innerAngle,
										  const f32 outerAngle, const vec3 &color) {
	const vec3 normalizedDir = glm::normalize(direction);

	AddCone(position, normalizedDir, range, glm::radians(outerAngle), color, 16);

	if (innerAngle > 0.0f) {
		AddCone(position, normalizedDir, range * 0.95f, glm::radians(innerAngle), color * 1.5f, 12);
	}

	AddSphere(position, 0.1f, color, 8);

	AddLine(position, position + normalizedDir * range, color * 0.8f);

	const vec3 endPoint = position + normalizedDir * range;
	const f32 endRadius = range * glm::tan(glm::radians(outerAngle));
	AddCircle(endPoint, normalizedDir, endRadius, color * 0.6f, 256);
}

void GizmoRenderSystem::AddCircle(const vec3 &center, const vec3 &normal, f32 radius, const vec3 &color, int segments) {
	const vec3 normalizedNormal = glm::normalize(normal);

	vec3 perpendicular1;
	if (abs(normalizedNormal.y) < 0.9f) {
		perpendicular1 = glm::normalize(glm::cross(normalizedNormal, vec3(0, 1, 0)));
	} else {
		perpendicular1 = glm::normalize(glm::cross(normalizedNormal, vec3(1, 0, 0)));
	}
	const vec3 perpendicular2 = glm::normalize(glm::cross(normalizedNormal, perpendicular1));

	for (int i = 0; i < segments; i++) {
		const f32 angle1 = static_cast<f32>(i) / static_cast<f32>(segments) * 2.0f * Math::PI;
		const f32 angle2 = static_cast<f32>(i + 1) / static_cast<f32>(segments) * 2.0f * Math::PI;

		vec3 point1 = center + (perpendicular1 * glm::cos(angle1) + perpendicular2 * glm::sin(angle1)) * radius;
		vec3 point2 = center + (perpendicular1 * glm::cos(angle2) + perpendicular2 * glm::sin(angle2)) * radius;

		AddLine(point1, point2, color);
	}
}

void GizmoRenderSystem::AddCone(const vec3 &apex, const vec3 &direction, f32 length, f32 angle, const vec3 &color,
								int segments) {
	const vec3 normalizedDir = glm::normalize(direction);
	const f32 radius = length * glm::tan(angle);

	const vec3 baseCenter = apex + normalizedDir * length;

	vec3 perpendicular1;
	if (abs(normalizedDir.y) < 0.9f) {
		perpendicular1 = glm::normalize(glm::cross(normalizedDir, vec3(0, 1, 0)));
	} else {
		perpendicular1 = glm::normalize(glm::cross(normalizedDir, vec3(1, 0, 0)));
	}
	const vec3 perpendicular2 = glm::normalize(glm::cross(normalizedDir, perpendicular1));

	std::vector<vec3> basePoints;
	for (int i = 0; i <= segments; i++) {
		f32 angle = static_cast<f32>(i) / static_cast<f32>(segments) * 2.0f * Math::PI;
		vec3 point = baseCenter + (perpendicular1 * glm::cos(angle) + perpendicular2 * glm::sin(angle)) * radius;
		basePoints.push_back(point);

		if (i % 4 == 0) {
			AddLine(apex, point, color);
		}

		if (i > 0) {
			AddLine(basePoints[i - 1], point, color);
		}
	}
}

void GizmoRenderSystem::AddSphere(const vec3 &center, const f32 radius, const vec3 &color, const int segments) {
	AddCircle(center, vec3(1, 0, 0), radius, color, segments);
	AddCircle(center, vec3(0, 1, 0), radius, color, segments);
	AddCircle(center, vec3(0, 0, 1), radius, color, segments);

	for (int i = 1; i < segments / 4; i++) {
		const f32 angle = static_cast<f32>(i) / static_cast<f32>(segments / 4) * Math::PI * 0.5f;
		const f32 y = glm::sin(angle) * radius;
		const f32 circleRadius = glm::cos(angle) * radius;

		AddCircle(center + vec3(0, y, 0), vec3(0, 1, 0), circleRadius, color * 0.7f, segments);
		AddCircle(center - vec3(0, y, 0), vec3(0, 1, 0), circleRadius, color * 0.7f, segments);
	}
}

void GizmoRenderSystem::AddCameraFrustum(const mat4 &viewMatrix, const mat4 &projMatrix, const vec3 &color,
										 const vec3 &cameraPos) {
	const mat4 invVP = inverse(projMatrix * viewMatrix);

	vec3 worldCorners[8];
	for (int i = 0; i < 8; ++i) {
		const vec4 frustumCorners[8] = { { -1, -1, 0, 1 }, { 1, -1, 0, 1 }, { 1, 1, 0, 1 }, { -1, 1, 0, 1 },
										 { -1, -1, 1, 1 }, { 1, -1, 1, 1 }, { 1, 1, 1, 1 }, { -1, 1, 1, 1 } };
		vec4 worldPos = invVP * frustumCorners[i];
		worldCorners[i] = vec3(worldPos) / worldPos.w;
	}

	constexpr auto nearColor = vec3(1, 0, 0);
	AddLine(worldCorners[0], worldCorners[1], nearColor);
	AddLine(worldCorners[1], worldCorners[2], nearColor);
	AddLine(worldCorners[2], worldCorners[3], nearColor);
	AddLine(worldCorners[3], worldCorners[0], nearColor);

	constexpr auto farColor = vec3(0, 1, 0);
	AddLine(worldCorners[4], worldCorners[5], farColor);
	AddLine(worldCorners[5], worldCorners[6], farColor);
	AddLine(worldCorners[6], worldCorners[7], farColor);
	AddLine(worldCorners[7], worldCorners[4], farColor);

	constexpr auto connectColor = vec3(0, 0, 1);
	AddLine(worldCorners[0], worldCorners[4], connectColor);
	AddLine(worldCorners[1], worldCorners[5], connectColor);
	AddLine(worldCorners[2], worldCorners[6], connectColor);
	AddLine(worldCorners[3], worldCorners[7], connectColor);
}

int GizmoRenderSystem::GetLODSegments(const f32 distance) const {
	if (distance < 10.0f) {
		return HIGH_DETAIL_SEGMENTS;
	} else if (distance < 50.0f) {
		return MEDIUM_DETAIL_SEGMENTS;
	} else {
		return LOW_DETAIL_SEGMENTS;
	}
}

void GizmoRenderSystem::UpdateBuffers(uint32 frameIndex) {
	if (m_Vertices.empty()) {
		return;
	}

	const VkDeviceSize vertexBufferSize = sizeof(Vertex) * m_Vertices.size();
	const VkDeviceSize indexBufferSize = sizeof(uint32_t) * m_Indices.size();

	auto &currentVertexBuffer = m_VertexBuffers[frameIndex];
	auto &currentIndexBuffer = m_IndexBuffers[frameIndex];

	if (!currentVertexBuffer || currentVertexBuffer->GetBufferSize() < vertexBufferSize) {
		VkDeviceSize newVertexSize = vertexBufferSize * 1.5f;

		currentVertexBuffer =
			CreateUnique<Buffer>(device, "Gizmo_VertexBuffer", newVertexSize, 1, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
								 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		currentVertexBuffer->Map();
	}

	if (!currentIndexBuffer || currentIndexBuffer->GetBufferSize() < indexBufferSize) {
		VkDeviceSize newIndexSize = indexBufferSize * 1.5f;

		currentIndexBuffer =
			CreateUnique<Buffer>(device, "Gizmo_IndexBuffer", newIndexSize, 1, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
								 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		currentIndexBuffer->Map();
	}

	currentVertexBuffer->Write(m_Vertices.data(), vertexBufferSize);
	currentIndexBuffer->Write(m_Indices.data(), indexBufferSize);
}

} // namespace Aquila::Rendering::Systems
