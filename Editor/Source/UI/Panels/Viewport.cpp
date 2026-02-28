#include "UI/Panels/Viewport.h"

#include <math.h>
#include "Aquila/Core/Application.h"
#include "Aquila/Core/Defines.h"
#include "Aquila/Rendering/Camera.h"
#include "Aquila/Events/SceneEvent.h"
#include "Aquila/Math/Geometry/Ray.h"
#include "Aquila/Graphics/Core/Renderer.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/Components/SceneNodeComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/Entity.h"
#include "lucide.h"
#include "Aquila/Assets/AssetManager.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/EntityManager.h"

namespace Aquila::SceneManagement::Components {
struct MetadataComponent;
}

namespace Editor {

ViewportPanel::ViewportPanel(Aquila::Core::Application &app) : Layer("Viewport"), m_App(app) {
	AQUILA_LOG_INFO("ViewportPanel created");
}

void ViewportPanel::OnAttach() {
	AQUILA_LOG_INFO("ViewportPanel attached");
	m_SelectedEntity = Aquila::SceneManagement::Entity::Null();
	m_LastViewportSize = { 0, 0 };
}

void ViewportPanel::OnDetach() {
	m_IsDetached = true;
	AQUILA_LOG_INFO("ViewportPanel detached");
}

void ViewportPanel::OnUpdate(f32 deltaTime) {}

void ViewportPanel::OnRender() {
	auto *scene = m_App.GetAssetManager().GetActiveScene();
	if (scene == nullptr) {
		return;
	}

	auto &camera = m_App.GetEditorCamera();
	auto &renderer = m_App.GetRenderer();

	ImVec2 viewportSize = m_LastViewportSize;
	if (viewportSize.x > 0 && viewportSize.y > 0) {
		renderer.RenderSceneBatched(scene, camera, static_cast<uint32>(viewportSize.x),
									static_cast<uint32>(viewportSize.y));
	}
}

void ViewportPanel::OnImGuiRender() {
	ImGui::Begin(ICON_LC_CLAPPERBOARD " Scene", nullptr, ImGuiWindowFlags_NoCollapse);

	ImVec2 viewportSize = ImGui::GetContentRegionAvail();

	if (static_cast<uint32>(viewportSize.x) != static_cast<uint32>(m_LastViewportSize.x) ||
		static_cast<uint32>(viewportSize.y) != static_cast<uint32>(m_LastViewportSize.y)) {
		OnViewportResize(viewportSize);
		m_LastViewportSize = viewportSize;
	}

	auto *scene = m_App.GetAssetManager().GetActiveScene();
	if (!scene) {
		ImGui::End();
		return;
	}

	auto *attachment = m_App.GetRenderer().GetSceneOutputTexture(scene).get();

	if (attachment && attachment->GetDescriptorSet() != VK_NULL_HANDLE) {
		auto textureId = reinterpret_cast<ImTextureID>(attachment->GetDescriptorSet());

		// Render the image FIRST
		ImGui::Image(textureId, viewportSize, { 0, 1 }, { 1, 0 });

		// THEN get its bounds
		ImVec2 imageMin = ImGui::GetItemRectMin();
		ImVec2 imageMax = ImGui::GetItemRectMax();

		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(imageMin.x, imageMin.y, imageMax.x - imageMin.x, imageMax.y - imageMin.y);

		HandleObjectPicking(viewportSize);
		HandleCameraControls(viewportSize);

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SCENE_PATH")) {
				if (const auto droppedPath = static_cast<const char *>(payload->Data); droppedPath != nullptr) {
					Aquila::Events::SceneLoadedEvent event(droppedPath);
					m_App.OnEvent(event);
				}
			}

			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("MESH_ASSET")) {
				const char *meshPath = static_cast<const char *>(payload->Data);
				if (meshPath != nullptr) {
					AQUILA_LOG_INFO("Mesh dropped in viewport: {}", meshPath);

					auto *scene = m_App.GetAssetManager().GetActiveScene();
					if (scene == nullptr) {
						ImGui::EndDragDropTarget();
						ImGui::End();
						return;
					}

					auto *entityManager = scene->GetEntityManager();
					auto entity = entityManager->CreateEntity("Loaded entity");

					if (!entity.IsValid()) {
						ImGui::EndDragDropTarget();
						ImGui::End();
						return;
					}

					entity.AddComponent<Aquila::SceneManagement::Components::MaterialComponent>();
					auto &meshComponent = entity.AddComponent<Aquila::SceneManagement::Components::MeshComponent>();

					auto mesh = m_App.GetAssetManager().LoadMesh(meshPath);

					if (!mesh) {
						AQUILA_LOG_CRITICAL("Mesh could not be loaded");
						ImGui::EndDragDropTarget();
						ImGui::End();
						return;
					}

					meshComponent.data = mesh;
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (m_ShowGizmo) {
			DrawGizmos();
		}
		// DrawGizmoToolbarOverlay({image});
	}

	ImGui::End();
}

void ViewportPanel::OnViewportResize(const ImVec2 &newSize) {
	AQUILA_LOG_DEBUG("Viewport resized to: {}x{}", newSize.x, newSize.y);
}

void ViewportPanel::HandleCameraControls(const ImVec2 &viewportSize) {
	bool isHovered = ImGui::IsItemHovered();
	bool isGizmoUsing = ImGuizmo::IsUsing() || ImGuizmo::IsOver();

	if (!isHovered || isGizmoUsing) {
		m_IsFirstMouse = true;
		return;
	}

	ImVec2 mousePos = ImGui::GetMousePos();
	ImVec2 viewportPos = ImGui::GetItemRectMin();

	f32 relativeMouseX = mousePos.x - viewportPos.x;
	f32 relativeMouseY = mousePos.y - viewportPos.y;

	f32 normalizedMouseX = ((relativeMouseX / viewportSize.x) * 2.0f) - 1.0f;
	f32 normalizedMouseY = 1.0f - ((relativeMouseY / viewportSize.y) * 2.0f);

	HandleMouseInput(normalizedMouseX, normalizedMouseY, viewportSize);
	HandleKeyboardInput();
	HandleMouseWheel(viewportSize);
}

void ViewportPanel::HandleMouseInput(f32 mouseX, f32 mouseY, const ImVec2 &viewportSize) {
	auto &camera = m_App.GetEditorCamera();
	ImGuiIO &io = ImGui::GetIO();

	static f32 lastMouseX = 0.0f;
	static f32 lastMouseY = 0.0f;

	if (m_IsFirstMouse) {
		lastMouseX = mouseX;
		lastMouseY = mouseY;
		m_IsFirstMouse = false;
	}

	f32 deltaX = mouseX - lastMouseX;
	f32 deltaY = mouseY - lastMouseY;

	constexpr f32 sensitivity = 2.0f;
	deltaX *= sensitivity;
	deltaY *= sensitivity;

	if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_None);

		if (camera.GetType() == Aquila::Rendering::Camera::CameraType::Free) {
			camera.Rotate(deltaX, -deltaY);
		} else if (camera.GetType() == Aquila::Rendering::Camera::CameraType::Orbit) {
			constexpr f32 orbitSensitivity = 0.5f;
			camera.OrbitRotate(deltaX * orbitSensitivity, -deltaY * orbitSensitivity);
		}
	}

	if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

		constexpr f32 panSensitivity = 0.01f;
		vec3 right = normalize(cross(camera.GetDirection(), vec3(0, 1, 0)));
		vec3 up = normalize(cross(right, camera.GetDirection()));

		vec3 panOffset = (-deltaX * right + deltaY * up) * panSensitivity;

		if (camera.GetType() == Aquila::Rendering::Camera::CameraType::Free) {
			camera.SetPosition(camera.GetPosition() + panOffset);
		} else if (camera.GetType() == Aquila::Rendering::Camera::CameraType::Orbit) {
			vec3 newTarget = camera.GetTarget() + panOffset;
			camera.SetOrbitTarget(newTarget);
		}
	}

	if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && io.KeyAlt) {
		camera.SwitchToType(Aquila::Rendering::Camera::CameraType::Orbit, vec3(0));
		constexpr f32 orbitSensitivity = 0.5f;
		camera.OrbitRotate(deltaX * orbitSensitivity, -deltaY * orbitSensitivity);
	} else if (camera.GetType() == Aquila::Rendering::Camera::CameraType::Orbit && !io.KeyAlt) {
		camera.SwitchToType(Aquila::Rendering::Camera::CameraType::Free, vec3(0));
	}

	lastMouseX = mouseX;
	lastMouseY = mouseY;
}

void ViewportPanel::HandleKeyboardInput() const {
	auto &camera = m_App.GetEditorCamera();
	ImGuiIO &io = ImGui::GetIO();

	if (camera.GetType() != Aquila::Rendering::Camera::CameraType::Free) {
		return;
	}

	if (io.KeyShift) {
		camera.SpeedUp();
	} else {
		camera.ResetSpeed();
	}

	f32 deltaTime = io.DeltaTime;

	if (ImGui::IsKeyDown(ImGuiKey_W)) {
		camera.MoveForward(deltaTime);
	}
	if (ImGui::IsKeyDown(ImGuiKey_S)) {
		camera.MoveBackward(deltaTime);
	}
	if (ImGui::IsKeyDown(ImGuiKey_A)) {
		camera.MoveLeft(deltaTime);
	}
	if (ImGui::IsKeyDown(ImGuiKey_D)) {
		camera.MoveRight(deltaTime);
	}

	if (ImGui::IsKeyDown(ImGuiKey_Q) || ImGui::IsKeyDown(ImGuiKey_E)) {
		vec3 up = vec3(0, 1, 0);
		f32 verticalSpeed = camera.GetMovementSpeed();

		if (ImGui::IsKeyDown(ImGuiKey_Q)) {
			camera.SetPosition(camera.GetPosition() - up * verticalSpeed * deltaTime);
		}
		if (ImGui::IsKeyDown(ImGuiKey_E)) {
			camera.SetPosition(camera.GetPosition() + up * verticalSpeed * deltaTime);
		}
	}
}

void ViewportPanel::HandleMouseWheel(const ImVec2 &viewportSize) const {
	auto &camera = m_App.GetEditorCamera();

	if (ImGuiIO &io = ImGui::GetIO(); io.MouseWheel != 0.0f) {
		constexpr f32 zoomSensitivity = 0.1f;
		f32 zoomDelta = io.MouseWheel * zoomSensitivity;

		if (camera.GetType() == Aquila::Rendering::Camera::CameraType::Free) {
			if (io.MouseWheel > 0) {
				camera.MoveForward(zoomDelta);
			} else if (io.MouseWheel < 0) {
				camera.MoveBackward(-zoomDelta);
			}
		} else if (camera.GetType() == Aquila::Rendering::Camera::CameraType::Orbit) {
			camera.OrbitZoom(-zoomDelta);
		}
	}
}

void ViewportPanel::HandleObjectPicking(const ImVec2 &viewportSize) {
	bool isHovered = ImGui::IsItemHovered();

	if (ImGuiIO &io = ImGui::GetIO(); !isHovered || io.KeyAlt || ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
									  ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
		return;
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 viewportPos = ImGui::GetItemRectMin();

		f32 relativeMouseX = mousePos.x - viewportPos.x;
		f32 relativeMouseY = mousePos.y - viewportPos.y;

		ObjectPickingResult result =
			PerformObjectPicking(vec2(relativeMouseX, relativeMouseY), vec2(viewportSize.x, viewportSize.y));

		auto *scene = m_App.GetAssetManager().GetActiveScene();
		if (scene == nullptr) {
			return;
		}
		auto *entityManager = scene->GetEntityManager();

		entityManager->ForEach<Aquila::SceneManagement::Components::MetadataComponent>(
			[](Aquila::SceneManagement::Components::MetadataComponent &metaData) { metaData.SetSelected(false); });

		if (result.hit) {
			Aquila::SceneManagement::Entity selected(result.entity);
			if (selected.HasComponent<Aquila::SceneManagement::Components::MetadataComponent>()) {
				selected.GetComponent<Aquila::SceneManagement::Components::MetadataComponent>().SetSelected(true);
			}

			Aquila::Events::EntitySelectedEvent event(selected);
			m_App.OnEvent(event);

			AQUILA_LOG_DEBUG(
				"Entity '{}' was selected!",
				selected.HasComponent<Aquila::SceneManagement::Components::MetadataComponent>()
					? selected.GetComponent<Aquila::SceneManagement::Components::MetadataComponent>().GetName()
					: "<unknown>");
		} else {
			entityManager->ForEach<Aquila::SceneManagement::Components::MetadataComponent>(
				[](Aquila::SceneManagement::Components::MetadataComponent &metaData) { metaData.SetSelected(false); });

			m_SelectedEntity = Aquila::SceneManagement::Entity::Null();

			Aquila::Events::EntitySelectedEvent clearEvent(m_SelectedEntity);
			m_App.OnEvent(clearEvent);

			AQUILA_LOG_DEBUG("Selection cleared (no entity hit)");
		}
	}
}

ViewportPanel::ObjectPickingResult ViewportPanel::PerformObjectPicking(const vec2 &mousePos,
																	   const vec2 &viewportSize) const {
	ObjectPickingResult result;

	auto &camera = m_App.GetEditorCamera();
	const Aquila::Math::Geometry::Ray ray =
		Aquila::Math::Geometry::ScreenToWorldRay(mousePos, viewportSize, camera.GetView(), camera.GetProjection());

	auto *scene = m_App.GetAssetManager().GetActiveScene();
	if (scene == nullptr) {
		return result;
	}
	auto *entityManager = scene->GetEntityManager();

	entityManager->ForEach<Aquila::SceneManagement::Components::MetadataComponent,
						   Aquila::SceneManagement::Components::TransformComponent>(
		[&](Aquila::SceneManagement::Entity entity, Aquila::SceneManagement::Components::MetadataComponent &metadata,
			Aquila::SceneManagement::Components::TransformComponent &transform) {
			if (!metadata.IsVisible()) {
				return;
			}

			f32 distance = 0.0f;
			bool hit = false;

			if (entity.HasComponent<Aquila::SceneManagement::Components::MeshComponent>()) {
				auto &meshComp = entity.GetComponent<Aquila::SceneManagement::Components::MeshComponent>();

				mat4 worldMatrix = transform.GetWorldMatrixLazy();
				vec3 worldPos = vec3(worldMatrix[3]);
				vec3 worldScale = transform.GetWorldScale();

				vec3 aabbMin = worldPos - worldScale;
				vec3 aabbMax = worldPos + worldScale;
				f32 aabbDist = NAN;
				if (!ray.IntersectAABB(aabbMin, aabbMax, aabbDist)) {
					return;
				}

				const auto &vertices = meshComp.data->GetVertices();
				const auto &indices = meshComp.data->GetIndices();
				f32 closestDist = std::numeric_limits<f32>::max();

				for (size_t i = 0; i < indices.size(); i += 3) {
					vec3 v0 = vec3(worldMatrix * vec4(vertices[indices[i]].pos, 1.0f));
					vec3 v1 = vec3(worldMatrix * vec4(vertices[indices[i + 1]].pos, 1.0f));
					vec3 v2 = vec3(worldMatrix * vec4(vertices[indices[i + 2]].pos, 1.0f));

					f32 triDist = NAN;
					if (ray.IntersectTriangle(v0, v1, v2, triDist)) {
						if (triDist < closestDist) {
							closestDist = triDist;
							hit = true;
						}
					}
				}

				if (hit) {
					distance = closestDist;
				}

			} else {
				vec3 worldPos = vec3(transform.GetWorldMatrixLazy()[3]);
				f32 radius = 0.5f;
				hit = ray.IntersectSphere(worldPos, radius, distance);
			}

			if (hit && distance < result.distance) {
				result.hit = true;
				result.distance = distance;
				result.entity = entity;
				result.hitPoint = ray.GetPoint(distance);
			}
		});

	return result;
}

void ViewportPanel::DrawGizmos() {
	if (m_SelectedEntity.IsNull()) {
		return;
	}

	auto *scene = m_App.GetAssetManager().GetActiveScene();
	if (scene == nullptr) {
		return;
	}

	if (!m_SelectedEntity.IsValid() ||
		!m_SelectedEntity.HasComponent<Aquila::SceneManagement::Components::TransformComponent>()) {
		return;
	}

	auto &camera = m_App.GetEditorCamera();
	Aquila::SceneManagement::Entity entity = m_SelectedEntity;
	auto &transform = entity.GetComponent<Aquila::SceneManagement::Components::TransformComponent>();

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(),
					  ImGui::GetWindowHeight());

	auto cameraView = camera.GetView();
	auto cameraProjection = camera.GetProjection();
	auto transformMatrix = transform.GetWorldMatrixLazy();

	f32 *snapValues = nullptr;
	if (m_UseSnap) {
		switch (m_GizmoOperation) {
		case ImGuizmo::TRANSLATE:
			snapValues = m_SnapValues;
			break;
		case ImGuizmo::ROTATE:
			snapValues = &m_RotationSnap;
			break;
		case ImGuizmo::SCALE:
			snapValues = &m_ScaleSnap;
			break;
		default:;
		}
	}

	ImGuizmo::Manipulate(value_ptr(cameraView), value_ptr(cameraProjection), m_GizmoOperation, m_GizmoMode,
						 value_ptr(transformMatrix), nullptr, snapValues);

	if (ImGuizmo::IsUsing()) {
		vec3 newWorldPos, newWorldRotEuler, newWorldScale;
		ImGuizmo::DecomposeMatrixToComponents(value_ptr(transformMatrix), value_ptr(newWorldPos),
											  value_ptr(newWorldRotEuler), value_ptr(newWorldScale));

		glm::quat newWorldRot = glm::quat(radians(newWorldRotEuler));

		auto *node = entity.TryGetComponent<Aquila::SceneManagement::Components::SceneNodeComponent>();

		if (node && !node->Parent.IsNull()) {
			Aquila::SceneManagement::Entity parent;
			parent = node->Parent;

			if (auto *parentTransform =
					parent.TryGetComponent<Aquila::SceneManagement::Components::TransformComponent>()) {
				mat4 parentWorld = parentTransform->GetWorldMatrixLazy();
				mat4 parentInverse = inverse(parentWorld);

				vec4 localPos4 = parentInverse * vec4(newWorldPos, 1.0f);
				vec3 localPos = vec3(localPos4);

				glm::quat parentWorldRot = parentTransform->GetWorldRotation();
				glm::quat localRot = inverse(parentWorldRot) * newWorldRot;

				vec3 parentWorldScale = parentTransform->GetWorldScale();
				vec3 localScale = newWorldScale / parentWorldScale;

				transform.SetLocalPosition(localPos);
				transform.SetLocalRotation(localRot);
				transform.SetLocalScale(localScale);
			}
		} else {
			transform.SetLocalPosition(newWorldPos);
			transform.SetLocalRotation(newWorldRot);
			transform.SetLocalScale(newWorldScale);
		}
	}
}

void ViewportPanel::DrawGizmoToolbarOverlay(ImVec2 viewportBounds[2]) {
	ImVec2 toolbarPos = ImVec2(viewportBounds[0].x + 10, viewportBounds[0].y + 10);

	ImGui::SetNextWindowPos(toolbarPos);
	ImGui::SetNextWindowBgAlpha(0.5f);
	ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
									ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
									ImGuiWindowFlags_AlwaysAutoResize;

	if (ImGui::Begin("GizmoToolbar", nullptr, overlayFlags)) {
		bool isTranslate = (m_GizmoOperation == ImGuizmo::TRANSLATE);
		if (isTranslate) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.8f, 1.0f));
		}
		if (ImGui::Button(ICON_LC_AXIS_3D)) {
			m_GizmoOperation = ImGuizmo::TRANSLATE;
		}
		if (isTranslate) {
			ImGui::PopStyleColor();
		}
		ImGui::SameLine();

		bool isRotate = (m_GizmoOperation == ImGuizmo::ROTATE);
		if (isRotate) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.8f, 1.0f));
		}
		if (ImGui::Button(ICON_LC_ROTATE_3D)) {
			m_GizmoOperation = ImGuizmo::ROTATE;
		}
		if (isRotate) {
			ImGui::PopStyleColor();
		}
		ImGui::SameLine();

		bool isScale = (m_GizmoOperation == ImGuizmo::SCALE);
		if (isScale) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.8f, 1.0f));
		}
		if (ImGui::Button(ICON_LC_SCALE_3D)) {
			m_GizmoOperation = ImGuizmo::SCALE;
		}
		if (isScale) {
			ImGui::PopStyleColor();
		}
		ImGui::SameLine();

		if (ImGui::Button(m_GizmoMode == ImGuizmo::WORLD ? "World" : "Local")) {
			m_GizmoMode = (m_GizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
		}
		ImGui::SameLine();

		ImGui::Checkbox("Show", &m_ShowGizmo);
		ImGui::SameLine();

		ImGui::Checkbox("Snap", &m_UseSnap);

		if (m_UseSnap) {
			ImGui::SameLine();
			switch (m_GizmoOperation) {
			case ImGuizmo::TRANSLATE:
				ImGui::SetNextItemWidth(80);
				ImGui::DragFloat("##TranslateSnap", &m_SnapValues[0], 0.1f, 0.1f, 10.0f, "%.1f");
				break;
			case ImGuizmo::ROTATE:
				ImGui::SetNextItemWidth(80);
				ImGui::DragFloat("##RotateSnap", &m_RotationSnap, 1.0f, 1.0f, 90.0f, "%.0f°");
				break;
			case ImGuizmo::SCALE:
				ImGui::SetNextItemWidth(80);
				ImGui::DragFloat("##ScaleSnap", &m_ScaleSnap, 0.01f, 0.01f, 1.0f, "%.2f");
				break;
			default:;
			}
		}
	}
	ImGui::End();
}

void ViewportPanel::OnEvent(Aquila::Events::Event &event) {
	if (m_IsDetached) {
		return;
	}

	Aquila::Events::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Aquila::Events::EntitySelectedEvent>([this](const Aquila::Events::EntitySelectedEvent &event) {
		if (m_IsDetached) {
			return false;
		}
		m_SelectedEntity = event.GetEntity(); // todo : maybe add guards here
		return false;
	});

	dispatcher.Dispatch<Aquila::Events::EntityDeletedEvent>([this](const Aquila::Events::EntityDeletedEvent &event) {
		if (m_IsDetached) {
			return false;
		}
		if (!m_SelectedEntity.IsNull() && m_SelectedEntity == event.GetEntity()) {
			m_SelectedEntity = Aquila::SceneManagement::Entity::Null();
		}
		return false;
	});
}

} // namespace Editor
