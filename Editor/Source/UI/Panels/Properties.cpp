#include "UI/Panels/Properties.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/LightComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/MetadataComponent.h"
#include "UI/UI.h"
namespace Editor::Panels {

bool Properties::DrawComponentHeader(
    const char *icon, const char *label, const char *menuId,
    const std::vector<ComponentMenuAction> &menuActions) {

  bool headerOpen = ImGui::CollapsingHeader(
      (std::string(icon) + " " + std::string(label)).c_str(),
      ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);

  f32 buttonSize = ImGui::GetFrameHeight();
  ImVec2 buttonPos = ImGui::GetItemRectMax();
  buttonPos.x -= buttonSize;
  buttonPos.y = ImGui::GetItemRectMin().y;

  ImGui::SetCursorScreenPos(buttonPos);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.8f));

  std::string buttonId =
      std::string(ICON_LC_ELLIPSIS_VERTICAL) + "##" + std::string(menuId);
  if (ImGui::Button(buttonId.c_str(), ImVec2(buttonSize, buttonSize))) {
    ImGui::OpenPopup(menuId);
  }

  ImGui::PopStyleColor(3);
  ImGui::PopStyleVar();

  if (ImGui::BeginPopup(menuId)) {
    for (const auto &action : menuActions) {
      bool isDestructive =
          (action.type == ComponentMenuAction::REMOVE_COMPONENT);

      if (isDestructive) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
      }

      std::string menuLabel =
          std::string(action.icon) + " " + std::string(action.label);
      if (ImGui::MenuItem(menuLabel.c_str())) {
        if (action.callback) {
          action.callback();
        }
      }

      if (isDestructive) {
        ImGui::PopStyleColor();
      }
    }
    ImGui::EndPopup();
  }

  return headerOpen;
}

void Properties::Draw() {
  auto *scene = Engine::Controller::Get()->GetSceneManager().GetActiveScene();
  if (!scene)
    return;

  auto &registry = scene->GetRegistry();
  entt::entity selected = UIManager::Get()->GetSelectedEntity();

  ImGui::Begin("Properties");

  if (selected == entt::null || !registry.valid(selected)) {
    ImGui::Text("No entity selected.");
    ImGui::End();
    return;
  }

  if (registry.any_of<MetadataComponent>(selected))
    DrawComponent_Metadata(registry, selected);

  if (registry.any_of<TransformComponent>(selected))
    DrawComponent_Transform(registry, selected);

  if (registry.any_of<MeshComponent>(selected))
    DrawComponent_Mesh(registry, selected);

  if (registry.any_of<CameraComponent>(selected))
    DrawComponent_Camera(registry, selected);

  if (registry.any_of<LightComponent>(selected))
    DrawComponent_Light(registry, selected);

  ImGui::Separator();
  ImGui::Spacing();

  if (ImGui::Button(ICON_LC_PLUS " Add Component", ImVec2(-1, 0))) {
    ImGui::OpenPopup("AddComponentPopup");
  }

  if (ImGui::BeginPopup("AddComponentPopup")) {
    if (!registry.any_of<MetadataComponent>(selected)) {
      if (ImGui::MenuItem(ICON_LC_FILE_CODE_2 " Metadata")) {
        registry.emplace<MetadataComponent>(selected);
        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::Separator();

    if (!registry.any_of<TransformComponent>(selected)) {
      if (ImGui::MenuItem(ICON_LC_MOVE_3D " Transform")) {
        registry.emplace<TransformComponent>(selected);
        ImGui::CloseCurrentPopup();
      }
    }

    if (!registry.any_of<MeshComponent>(selected)) {
      if (ImGui::MenuItem(ICON_LC_GRID_3X3 " Mesh")) {
        registry.emplace<MeshComponent>(selected);
        if (!registry.any_of<TransformComponent>(selected)) {
          registry.emplace<TransformComponent>(selected);
        }
        ImGui::CloseCurrentPopup();
      }
    }

    if (!registry.any_of<CameraComponent>(selected)) {
      if (ImGui::MenuItem(ICON_LC_CAMERA " Camera")) {
        registry.emplace<CameraComponent>(selected);
        ImGui::CloseCurrentPopup();
      }
    }

    if (!registry.any_of<LightComponent>(selected)) {
      if (ImGui::MenuItem(ICON_LC_LIGHTBULB " Light")) {
        registry.emplace<LightComponent>(selected);
        if (!registry.any_of<TransformComponent>(selected)) {
          registry.emplace<TransformComponent>(selected);
        }

        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::EndPopup();
  }

  ImGui::End();
}

void UpdateChildrenTransforms(entt::registry &registry,
                              entt::entity parentEntity) {
  if (!registry.valid(parentEntity) ||
      !registry.all_of<SceneNodeComponent, TransformComponent>(parentEntity))
    return;

  auto &parentNode = registry.get<SceneNodeComponent>(parentEntity);
  auto &parentTransform = registry.get<TransformComponent>(parentEntity);
  const glm::mat4 &parentWorldMatrix = parentTransform.GetWorldMatrix();

  for (entt::entity child : parentNode.Children) {
    if (!registry.valid(child) || !registry.all_of<TransformComponent>(child))
      continue;

    auto &childTransform = registry.get<TransformComponent>(child);
    childTransform.UpdateWorldMatrix(parentWorldMatrix);

    UpdateChildrenTransforms(registry, child);
  }
}

void Properties::DrawComponent_Transform(entt::registry &registry,
                                         entt::entity entity) {
  auto &transform = registry.get<TransformComponent>(entity);
  auto &position = transform.GetLocalPosition();
  auto &rotation = transform.GetLocalRotation();
  auto &scale = transform.GetLocalScale();

  std::vector<ComponentMenuAction> actions = {
      ComponentMenuAction(ComponentMenuAction::RESET, ICON_LC_REFRESH_CW,
                          "Reset Transform",
                          [&transform]() {
                            transform.SetLocalPosition(vec3(0.0f));
                            transform.SetLocalRotation(vec3(0.0f));
                            transform.SetLocalScale(vec3(1.0f));
                          }),
  };

  bool headerOpen = DrawComponentHeader(ICON_LC_MOVE_3D, "TRANSFORM",
                                        "TransformMenu", actions);

  if (headerOpen) {
    ImGui::Indent();
    bool changed = false;

    auto DrawVector3Control = [](const char *label, vec3 &values,
                                 f32 resetValue = 0.0f) -> bool {
      bool modified = false;
      ImGui::PushID(label);
      ImGui::Columns(2);
      ImGui::SetColumnWidth(0, 100);
      ImGui::Text("%s", label);
      ImGui::NextColumn();

      const char *axisLabels[] = {"X", "Y", "Z"};
      ImVec4 axisColors[] = {{0.8f, 0.1f, 0.15f, 1.0f},
                             {0.2f, 0.7f, 0.2f, 1.0f},
                             {0.2f, 0.3f, 0.9f, 1.0f}};

      constexpr f32 buttonSize = 20.0f;
      constexpr f32 inputWidth = 40.0f;

      for (int i = 0; i < 3; i++) {
        ImGui::PushStyleColor(ImGuiCol_Button, axisColors[i]);
        if (ImGui::Button(axisLabels[i], ImVec2(buttonSize, buttonSize))) {
          values[i] = resetValue;
          modified = true;
        }
        ImGui::PopStyleColor();

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::SetNextItemWidth(inputWidth);
        modified |= ImGui::DragFloat(
            ("##" + std::string(axisLabels[i]) + label).c_str(), &values[i],
            0.1f, -FLT_MAX, FLT_MAX, "%.2f");

        if (i < 2)
          ImGui::SameLine();
      }

      ImGui::Columns(1);
      ImGui::PopID();
      return modified;
    };

    changed |= DrawVector3Control("Position", position);

    vec3 eulerRotation = glm::degrees(glm::eulerAngles(rotation));
    if (DrawVector3Control("Rotation", eulerRotation)) {
      rotation = glm::quat(glm::radians(eulerRotation));
      changed = true;
    }

    changed |= DrawVector3Control("Scale", scale, 1.0f);

    if (changed) {
      glm::mat4 parentWorldMatrix = glm::mat4(1.0f);
      if (registry.all_of<SceneNodeComponent>(entity)) {
        entt::entity parent = registry.get<SceneNodeComponent>(entity).Parent;
        if (parent != entt::null && registry.valid(parent) &&
            registry.all_of<TransformComponent>(parent))
          parentWorldMatrix =
              registry.get<TransformComponent>(parent).GetWorldMatrix();
      }

      transform.UpdateWorldMatrix(parentWorldMatrix);
      UpdateChildrenTransforms(registry, entity);
    }
    ImGui::Unindent();
  }
}

void Properties::DrawComponent_Metadata(entt::registry &registry,
                                        entt::entity entity) {
  auto &meta = registry.get<MetadataComponent>(entity);
  char buffer[256];

#ifdef AQUILA_PLATFORM_WINDOWS
  strncpy_s(buffer, meta.Name.c_str(), sizeof(buffer));
#elif defined(AQUILA_PLATFORM_LINUX)
  strncpy(buffer, meta.Name.c_str(), sizeof(buffer));
  buffer[sizeof(buffer) - 1] = '\0';
#endif

  bool headerOpen =
      DrawComponentHeader(ICON_LC_FILE_CODE_2, "METADATA", "MetadataMenu");

  if (headerOpen) {
    ImGui::Text("Name");
    ImGui::SameLine();
    ImGui::InputText("##Name", buffer, sizeof(buffer));
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      meta.Name = std::string(buffer);
    }

    ImGui::PushFont(UI::FontManager::Get().GetFonts().Font14);
    ImGui::Text("UUID: %s", meta.ID.ToString().c_str());
    ImGui::PopFont();
  }
}

void Properties::DrawComponent_Camera(entt::registry &registry,
                                      entt::entity entity) {
  auto &component = registry.get<CameraComponent>(entity);

  std::vector<ComponentMenuAction> actions = {ComponentMenuAction(
      ComponentMenuAction::REMOVE_COMPONENT, ICON_LC_TRASH_2,
      "Remove Component",
      [&registry, entity]() { registry.remove<CameraComponent>(entity); })};

  bool headerOpen =
      DrawComponentHeader(ICON_LC_CAMERA, "CAMERA", "CameraMenu", actions);

  if (headerOpen) {

    ImGui::Checkbox("Primary Camera", &component.primary);
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Mark this camera as the main camera for rendering");
    }

    ImGui::Separator();

    if (ImGui::Checkbox("Orthographic", &component.isOrthographic)) {
    }

    ImGui::Separator();

    if (component.isOrthographic) {

      ImGui::Text("Orthographic Settings");
      ImGui::Indent();

      ImGui::DragFloat("Left", &component.orthoLeft, 0.1f, -100.0f, 0.0f,
                       "%.2f");
      ImGui::DragFloat("Right", &component.orthoRight, 0.1f, 0.0f, 100.0f,
                       "%.2f");
      ImGui::DragFloat("Top", &component.orthoTop, 0.1f, 0.0f, 100.0f, "%.2f");
      ImGui::DragFloat("Bottom", &component.orthoBottom, 0.1f, -100.0f, 0.0f,
                       "%.2f");

      if (component.orthoLeft >= component.orthoRight) {
        component.orthoLeft = component.orthoRight - 0.1f;
      }
      if (component.orthoBottom >= component.orthoTop) {
        component.orthoBottom = component.orthoTop - 0.1f;
      }

      ImGui::Unindent();
    } else {

      ImGui::Text("Perspective Settings");
      ImGui::Indent();

      ImGui::SliderFloat("Field of View", &component.fov, 1.0f, 179.0f,
                         "%.1f°");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Vertical field of view in degrees");
      }

      ImGui::DragFloat("Aspect Ratio", &component.aspectRatio, 0.01f, 0.1f,
                       10.0f, "%.3f");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Width / Height ratio (e.g., 1.778 for 16:9)");
      }

      ImGui::SameLine();
      if (ImGui::Button("16:9")) {
        component.aspectRatio = 16.0f / 9.0f;
      }
      ImGui::SameLine();
      if (ImGui::Button("4:3")) {
        component.aspectRatio = 4.0f / 3.0f;
      }
      ImGui::SameLine();
      if (ImGui::Button("1:1")) {
        component.aspectRatio = 1.0f;
      }

      ImGui::Unindent();
    }

    ImGui::Separator();

    ImGui::Text("Clipping Planes");
    ImGui::Indent();

    ImGui::DragFloat("Near Plane", &component.nearPlane, 0.01f, 0.001f, 10.0f,
                     "%.3f");
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Objects closer than this distance won't be rendered");
    }

    ImGui::DragFloat("Far Plane", &component.farPlane, 1.0f, 1.0f, 10000.0f,
                     "%.1f");
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Objects farther than this distance won't be rendered");
    }

    if (component.nearPlane >= component.farPlane) {
      component.nearPlane = component.farPlane - 0.1f;
    }

    ImGui::Unindent();

    if (ImGui::CollapsingHeader("Debug Info")) {
      ImGui::Text("Projection Type: %s",
                  component.isOrthographic ? "Orthographic" : "Perspective");
      ImGui::Text("Clip Range: %.3f - %.1f", component.nearPlane,
                  component.farPlane);

      if (!component.isOrthographic) {
        ImGui::Text("FOV: %.1f°", component.fov);
        ImGui::Text("Aspect: %.3f", component.aspectRatio);
      } else {
        float orthoWidth = component.orthoRight - component.orthoLeft;
        float orthoHeight = component.orthoTop - component.orthoBottom;
        ImGui::Text("Ortho Size: %.2f x %.2f", orthoWidth, orthoHeight);
      }
    }
  }
}
void Properties::DrawComponent_Light(entt::registry &registry,
                                     entt::entity entity) {
  auto &component = registry.get<LightComponent>(entity);

  std::vector<ComponentMenuAction> actions = {ComponentMenuAction(
      ComponentMenuAction::REMOVE_COMPONENT, ICON_LC_TRASH_2,
      "Remove Component",
      [&registry, entity]() { registry.remove<LightComponent>(entity); })};

  bool headerOpen =
      DrawComponentHeader(ICON_LC_LIGHTBULB, "LIGHT", "LightMenu", actions);

  if (headerOpen) {

    const char *lightTypeNames[] = {"Point", "Directional", "Spot"};

    ImGui::Combo("Light type", &reinterpret_cast<int &>(component.m_Type),
                 lightTypeNames, IM_ARRAYSIZE(lightTypeNames));

    if (component.m_Type == LightComponent::Type::Spot) {
      ImGui::SliderFloat("Inner Cone Angle", &component.m_InnerConeAngle, 0.0f,
                         90.0f);
      ImGui::SliderFloat("Outer Cone Angle", &component.m_OuterConeAngle, 0.0f,
                         90.0f);
    } else if (component.m_Type == LightComponent::Type::Directional) {
      ImGui::SliderFloat3("Light Direction",
                          glm::value_ptr(component.m_Direction), -1.0f, 1.0f);
    } else {
      component.m_InnerConeAngle = 0.0f;
      component.m_OuterConeAngle = 45.0f;
    }

    ImGui::SeparatorText("General Properties");

    ImGui::Checkbox("Is active", &component.m_IsActive);

    ImGui::ColorEdit3("Color", glm::value_ptr(component.m_Color));
    ImGui::SliderFloat("Intensity", &component.m_Intensity, 0.0f, 10.0f);
  }
}

void Properties::DrawComponent_Mesh(entt::registry &registry,
                                    entt::entity entity) {
  auto &component = registry.get<MeshComponent>(entity);

  std::vector<ComponentMenuAction> actions = {ComponentMenuAction(
      ComponentMenuAction::REMOVE_COMPONENT, ICON_LC_TRASH_2,
      "Remove Component",
      [&registry, entity]() { registry.remove<MeshComponent>(entity); })};

  bool headerOpen =
      DrawComponentHeader(ICON_LC_GRID_3X3, "MESH", "MeshMenu", actions);

  if (headerOpen) {
    ImGui::Text("Mesh:");

    ImGui::SameLine();

    std::string meshLabel =
        (component.data != nullptr && !component.data->GetPath().empty())
            ? component.data->GetPath()
            : "None";
    if (ImGui::Button(meshLabel.c_str(), ImVec2(200, 0))) {
      ImGui::Dummy(ImVec2(200, 100));
    }

    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload *payload =
              ImGui::AcceptDragDropPayload("MESH_ASSET")) {
        const char *outPath = static_cast<const char *>(payload->Data);
        if (outPath) {

          auto meshLoadEvent = CreateUnique<Engine::LoadMeshEvent>();
          meshLoadEvent->targetEntity = entity;
          meshLoadEvent->meshPath = outPath;
          Engine::EventBus::Get()->DispatchSync(std::move(meshLoadEvent));
        }
      }
      ImGui::EndDragDropTarget();
    }
  }
}
} // namespace Editor::Panels
