#include "UI/Panels/Viewport.h"
#include "AquilaPCH.h"
#include "Engine/Controller.h"
#include "Engine/EditorCamera.h"
#include "Engine/Events/Event.h"
#include "Engine/Geometry/Ray.h"

#include "Scene/Components/MetadataComponent.h"
#include "Scene/Components/SceneNodeComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Entity.h"

#include "UI/UI.h"
#include "lucide.h"

namespace Editor::Panels {
void Viewport::Draw() {
  ImGui::Begin(ICON_LC_CLAPPERBOARD " Scene", nullptr,
               ImGuiWindowFlags_NoCollapse);

  ImVec2 viewportSize = ImGui::GetContentRegionAvail();
  static ImVec2 lastViewportSize = {0, 0};

  if (viewportSize.x != lastViewportSize.x ||
      viewportSize.y != lastViewportSize.y) {
    auto viewportResizedEvent = CreateUnique<Engine::ViewportResizedEvent>();
    viewportResizedEvent->width = viewportSize.x;
    viewportResizedEvent->height = viewportSize.y;
    Engine::EventBus::Get()->DispatchSync(std::move(viewportResizedEvent));
    lastViewportSize = viewportSize;
  }

  ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
  ImVec2 viewportMaxRegion = ImGui::GetWindowContentRegionMax();
  ImVec2 viewportOffset = ImGui::GetWindowPos();

  ImVec2 viewportBounds[2];
  viewportBounds[0] = ImVec2(viewportMinRegion.x + viewportOffset.x,
                             viewportMinRegion.y + viewportOffset.y);
  viewportBounds[1] = ImVec2(viewportMaxRegion.x + viewportOffset.x,
                             viewportMaxRegion.y + viewportOffset.y);

  ImGuizmo::SetDrawlist();
  ImGuizmo::SetRect(viewportBounds[0].x, viewportBounds[0].y,
                    viewportBounds[1].x - viewportBounds[0].x,
                    viewportBounds[1].y - viewportBounds[0].y);

  auto image = Engine::Controller::Get()
                   ->GetRenderer()
                   .GetPassObject<Engine::GeometryPass>()
                   ->GetDescriptorSet();

  if (image != VK_NULL_HANDLE) {
    auto textureId = reinterpret_cast<ImTextureID>(image);
    ImGui::Image(textureId, {viewportSize.x, viewportSize.y}, {0, 1}, {1, 0});

    HandleObjectPicking(viewportSize);

    HandleCameraControls(viewportSize);
  }

  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *payload =
            ImGui::AcceptDragDropPayload("SCENE_PATH")) {
      const char *droppedPath = static_cast<const char *>(payload->Data);
      if (droppedPath) {
        auto openSceneEvent = CreateUnique<Engine::OpenSceneEvent>();
        openSceneEvent->scenePath = droppedPath;
        Engine::EventBus::Get()->DispatchSync(std::move(openSceneEvent));
      }
    }

    if (const ImGuiPayload *payload =
            ImGui::AcceptDragDropPayload("MESH_ASSET")) {
      const char *meshPath = (const char *)payload->Data;
      if (meshPath) {
        auto meshLoadEvent = CreateUnique<Engine::LoadMeshEvent>();
        meshLoadEvent->meshPath = meshPath;
        Engine::EventBus::Get()->DispatchSync(std::move(meshLoadEvent));
      }
    }
    ImGui::EndDragDropTarget();
  }

  if (m_ShowGizmo)
    DrawGizmos();
  DrawGizmoToolbarOverlay(viewportBounds);

  ImGui::End();
}

void Viewport::HandleCameraControls(const ImVec2 &viewportSize) {
  auto &camera = Engine::Controller::Get()->GetCamera();
  ImGuiIO &io = ImGui::GetIO();

  bool isHovered = ImGui::IsItemHovered();
  bool isGizmoUsing = ImGuizmo::IsUsing() || ImGuizmo::IsOver();

  if (!isHovered || isGizmoUsing) {
    m_IsFirstMouse = true;
    return;
  }

  ImVec2 mousePos = ImGui::GetMousePos();
  ImVec2 viewportPos = ImGui::GetItemRectMin();

  float relativeMouseX = mousePos.x - viewportPos.x;
  float relativeMouseY = mousePos.y - viewportPos.y;

  float normalizedMouseX = (relativeMouseX / viewportSize.x) * 2.0f - 1.0f;
  float normalizedMouseY = 1.0f - (relativeMouseY / viewportSize.y) * 2.0f;

  HandleMouseInput(normalizedMouseX, normalizedMouseY, viewportSize);

  HandleKeyboardInput();

  HandleMouseWheel(viewportSize);
}

void Viewport::HandleMouseInput(float mouseX, float mouseY,
                                const ImVec2 &viewportSize) {
  auto &camera = Engine::Controller::Get()->GetCamera();
  ImGuiIO &io = ImGui::GetIO();

  static float lastMouseX = 0.0f;
  static float lastMouseY = 0.0f;

  if (m_IsFirstMouse) {
    lastMouseX = mouseX;
    lastMouseY = mouseY;
    m_IsFirstMouse = false;
  }

  float deltaX = mouseX - lastMouseX;
  float deltaY = mouseY - lastMouseY;

  const float sensitivity = 2.0f;
  deltaX *= sensitivity;
  deltaY *= sensitivity;

  if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);

    if (camera.GetType() == Engine::EditorCamera::CameraType::Free) {

      camera.Rotate(deltaX, -deltaY);
    } else if (camera.GetType() == Engine::EditorCamera::CameraType::Orbit) {

      const float orbitSensitivity = 0.5f;
      camera.OrbitRotate(deltaX * orbitSensitivity, -deltaY * orbitSensitivity);
    }
  }

  if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

    const float panSensitivity = 0.01f;
    vec3 right = normalize(cross(camera.GetDirection(), vec3(0, 1, 0)));
    vec3 up = normalize(cross(right, camera.GetDirection()));

    vec3 panOffset = (-deltaX * right + deltaY * up) * panSensitivity;

    if (camera.GetType() == Engine::EditorCamera::CameraType::Free) {
      camera.SetPosition(camera.GetPosition() + panOffset);
    } else if (camera.GetType() == Engine::EditorCamera::CameraType::Orbit) {
      vec3 newTarget = camera.GetTarget() + panOffset;
      camera.SetOrbitTarget(newTarget);
    }
  }

  if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && io.KeyAlt) {
    camera.SwitchToType(Engine::EditorCamera::CameraType::Orbit, vec3(0));
    const float orbitSensitivity = 0.5f;
    camera.OrbitRotate(deltaX * orbitSensitivity, -deltaY * orbitSensitivity);
  } else if (camera.GetType() == Engine::EditorCamera::CameraType::Orbit) {
    camera.SwitchToType(Engine::EditorCamera::CameraType::Free, vec3(0));
  }
  lastMouseX = mouseX;
  lastMouseY = mouseY;
}

void Viewport::DrawGizmos() {

  if (UIManager::Get()->GetSelectedEntity() != entt::null) {
    auto *scene = Engine::Controller::Get()->GetSceneManager().GetActiveScene();
    auto &camera = Engine::Controller::Get()->GetCamera();

    Engine::Entity ent = {UIManager::Get()->GetSelectedEntity(), scene};
    auto &transform = ent.GetComponent<TransformComponent>();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y,
                      ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
    auto cameraView = camera.GetView();
    auto cameraProjection = camera.GetProjection();
    auto transformLocalMatrix = transform.GetLocalTransformMatrix();

    ImGuizmo::Manipulate(value_ptr(cameraView), value_ptr(cameraProjection),
                         m_GizmoOperation, m_GizmoMode,
                         value_ptr(transformLocalMatrix));

    if (ImGuizmo::IsUsing()) {

      vec3 translation, rotation, scale;
      ImGuizmo::DecomposeMatrixToComponents(
          value_ptr(transformLocalMatrix), value_ptr(translation),
          value_ptr(rotation), value_ptr(scale));

      transform.SetLocalPosition(translation);
      transform.SetLocalScale(scale);
      transform.SetLocalRotation(quat(radians(rotation)));

      transform.UpdateWorldMatrix();
    }
  }
}
void Viewport::HandleObjectPicking(const ImVec2 &viewportSize) {

  bool isHovered = ImGui::IsItemHovered();
  ImGuiIO &io = ImGui::GetIO();

  if (!isHovered || io.KeyAlt || ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
      ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
    return;
  }

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 viewportPos = ImGui::GetItemRectMin();

    float relativeMouseX = mousePos.x - viewportPos.x;
    float relativeMouseY = mousePos.y - viewportPos.y;

    ObjectPickingResult result =
        PerformObjectPicking(vec2(relativeMouseX, relativeMouseY),
                             vec2(viewportSize.x, viewportSize.y));

    auto *scene = Engine::Controller::Get()->GetSceneManager().GetActiveScene();
    auto &registry = scene->GetRegistry();
    auto view = registry.view<MetadataComponent>();

    for (auto entityId : view) {
      Engine::Entity entity{entityId, scene};
      auto &metadata = entity.GetComponent<MetadataComponent>();
      metadata.Selected = false;
      UIManager::Get()->SetSelectedEntity(entt::null);
    }

    if (result.hit) {
      auto &metadata = Engine::Entity{result.entityID, scene}
                           .GetComponent<MetadataComponent>();

      metadata.Selected = true;
      UIManager::Get()->SetSelectedEntity(result.entityID);

      AQUILA_LOG_DEBUG("Entity : {} was selected!", metadata.Name);
    }
  }
}

Viewport::ObjectPickingResult
Viewport::PerformObjectPicking(const vec2 &mousePos, const vec2 &viewportSize) {
  ObjectPickingResult result;

  auto &camera = Engine::Controller::Get()->GetCamera();

  Geometry::Ray ray = Geometry::ScreenToWorldRay(
      mousePos, viewportSize, camera.GetView(), camera.GetProjection());

  auto *scene = Engine::Controller::Get()->GetSceneManager().GetActiveScene();
  if (!scene) {
    return result;
  }

  auto &registry = scene->GetRegistry();

  auto view = registry.view<MetadataComponent, TransformComponent>();

  for (auto entity : view) {
    auto [metadata, transform] =
        view.get<MetadataComponent, TransformComponent>(entity);

    if (!metadata.Visible) {
      continue;
    }

    float distance;
    bool hit = false;

    if (registry.any_of<MeshComponent>(entity)) {

      vec3 pos = transform.GetLocalPosition();
      vec3 scale = transform.GetLocalScale();

      vec3 aabbMin = pos - scale;
      vec3 aabbMax = pos + scale;

      hit = ray.IntersectAABB(aabbMin, aabbMax, distance);
    } else {

      vec3 pos = transform.GetLocalPosition();
      float radius = 0.5f;

      hit = ray.IntersectSphere(pos, radius, distance);
    }

    if (hit && distance < result.distance) {
      result.hit = true;
      result.distance = distance;
      result.entityID = entity;
      result.hitPoint = ray.GetPoint(distance);
    }
  }

  return result;
}

void Viewport::HandleKeyboardInput() {
  auto &camera = Engine::Controller::Get()->GetCamera();
  ImGuiIO &io = ImGui::GetIO();

  if (camera.GetType() != Engine::EditorCamera::CameraType::Free) {
    return;
  }

  if (io.KeyShift) {
    camera.SpeedUp();
  } else {
    camera.ResetSpeed();
  }

  float deltaTime = io.DeltaTime;

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
    float verticalSpeed = camera.GetMovementSpeed();

    if (ImGui::IsKeyDown(ImGuiKey_Q)) {
      camera.SetPosition(camera.GetPosition() - up * verticalSpeed * deltaTime);
    }
    if (ImGui::IsKeyDown(ImGuiKey_E)) {
      camera.SetPosition(camera.GetPosition() + up * verticalSpeed * deltaTime);
    }
  }
}

void Viewport::HandleMouseWheel(const ImVec2 &viewportSize) {
  auto &camera = Engine::Controller::Get()->GetCamera();
  ImGuiIO &io = ImGui::GetIO();

  if (io.MouseWheel != 0.0f) {
    const float zoomSensitivity = 0.1f;
    float zoomDelta = io.MouseWheel * zoomSensitivity;

    if (camera.GetType() == Engine::EditorCamera::CameraType::Free) {
      if (io.MouseWheel > 0) {
        camera.MoveForward(zoomDelta);
      } else if (io.MouseWheel < 0)
        camera.MoveBackward(-zoomDelta);
    } else if (camera.GetType() == Engine::EditorCamera::CameraType::Orbit) {

      camera.OrbitZoom(-zoomDelta);
    }
  }
}

void Viewport::DrawGizmoToolbarOverlay(ImVec2 viewportBounds[2]) {
  ImVec2 toolbarPos =
      ImVec2(viewportBounds[0].x + 10, viewportBounds[0].y + 10);

  ImGui::SetNextWindowPos(toolbarPos);
  ImGui::SetNextWindowBgAlpha(0.5f);
  ImGuiWindowFlags overlayFlags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_AlwaysAutoResize;

  if (ImGui::Begin("GizmoToolbar", nullptr, overlayFlags)) {
    if (ImGui::Button(ICON_LC_AXIS_3D)) {
      m_GizmoOperation = ImGuizmo::TRANSLATE;
    }
    ImGui::SameLine();

    if (ImGui::Button(ICON_LC_ROTATE_3D)) {
      m_GizmoOperation = ImGuizmo::ROTATE;
    }
    ImGui::SameLine();

    if (ImGui::Button(ICON_LC_SCALE_3D)) {
      m_GizmoOperation = ImGuizmo::SCALE;
    }
    ImGui::SameLine();

    if (ImGui::Button(m_GizmoMode == ImGuizmo::WORLD ? "World" : "Local")) {
      m_GizmoMode =
          (m_GizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
    }
    ImGui::SameLine();

    ImGui::Checkbox("Show", &m_ShowGizmo);
    ImGui::SameLine();

    ImGui::Checkbox("Snap", &m_UseSnap);

    ImGui::SameLine();

    if (m_UseSnap) {
      switch (m_GizmoOperation) {
      case ImGuizmo::TRANSLATE:
        ImGui::SetNextItemWidth(80);
        ImGui::DragFloat("##TranslateSnap", &m_SnapValues[0], 0.1f, 0.1f, 10.0f,
                         "%.1f");
        break;
      case ImGuizmo::ROTATE:
        ImGui::SetNextItemWidth(80);
        ImGui::DragFloat("##RotateSnap", &m_RotationSnap, 1.0f, 1.0f, 90.0f,
                         "%.0f°");
        break;
      case ImGuizmo::SCALE:
        ImGui::SetNextItemWidth(80);
        ImGui::DragFloat("##ScaleSnap", &m_ScaleSnap, 0.01f, 0.01f, 1.0f,
                         "%.2f");
        break;
      }
    }
  }
  ImGui::End();
}
}; // namespace Editor::Panels