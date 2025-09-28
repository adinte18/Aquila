#include "UI/Panels/Hierarchy.h"
#include "AquilaCore.h"
#include "AquilaPCH.h"
#include "Engine/Events/Event.h"
#include "Scene/Components/MetadataComponent.h"
#include "UI/UI.h"
#include "imgui_internal.h"

namespace Editor::Panels {

static ImGuiTextFilter s_HierarchyFilter;
static entt::entity s_DoubleClicked = entt::null;
static entt::entity s_HadRecentDroppedEntity = entt::null;
static entt::entity s_CurrentPrevious = entt::null;

void Hierarchy::DisplayEntityNode(Engine::AquilaScene *scene,
                                  entt::entity entity) {
  auto &registry = scene->GetRegistry();
  if (!registry.valid(entity) ||
      !registry.all_of<MetadataComponent, SceneNodeComponent>(entity))
    return;

  auto &data = registry.get<MetadataComponent>(entity);
  auto &node = registry.get<SceneNodeComponent>(entity);

  bool visible = ImGui::IsRectVisible(ImVec2(
      ImGui::GetContentRegionMax().x, ImGui::GetTextLineHeightWithSpacing()));
  if (!visible) {
    ImGui::NewLine();
    return;
  }

  bool show = true;
  if (s_HierarchyFilter.IsActive()) {
    if (!s_HierarchyFilter.PassFilter(data.Name.c_str())) {
      show = false;
    }
  }

  if (show) {
    ImGui::PushID(data.ID.ToString().c_str());

    bool noChildren = node.Children.empty();

    ImGuiTreeNodeFlags nodeFlags = 0;
    if (UIManager::Get()->GetSelectedEntity() == entity) {
      nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    nodeFlags |=
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding |
        ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (noChildren) {
      nodeFlags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool active = data.Visible;
    if (!active) {
      ImGui::PushStyleColor(ImGuiCol_Text,
                            ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    }

    bool dragging = false;
    bool doubleClicked = (entity == s_DoubleClicked);

    if (doubleClicked) {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {1.0f, 2.0f});
    }

    if (s_HadRecentDroppedEntity == entity) {
      ImGui::SetNextItemOpen(true);
      s_HadRecentDroppedEntity = entt::null;
    }

    const char *icon = ICON_LC_BOX;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

    bool nodeOpen =
        ImGui::TreeNodeEx((void *)(intptr_t)entity, nodeFlags, "%s", icon);

    if (ImGui::BeginDragDropSource()) {
      auto selected = entity;
      ImGui::TextUnformatted(data.Name.c_str());
      ImGui::SetDragDropPayload("Drag_Entity", &selected, sizeof(entt::entity));
      ImGui::EndDragDropSource();
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        ImGui::IsItemHovered() && !ImGui::IsItemToggledOpen()) {
      Engine::Entity{entity, scene}.GetComponent<MetadataComponent>().Selected =
          true;
      UIManager::Get()->SetSelectedEntity(entity);
    } else if (s_DoubleClicked == entity &&
               ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
               !ImGui::IsItemHovered()) {
      s_DoubleClicked = entt::null;
    }

    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
        ImGui::IsItemHovered()) {
      s_DoubleClicked = entity;
    }

    ImGui::PopStyleColor();
    ImGui::SameLine();

    if (!doubleClicked) {
      ImGui::TextUnformatted(data.Name.c_str());
    } else {
      static char nameBuffer[256];
      strcpy_s(nameBuffer, sizeof(nameBuffer), data.Name.c_str());

      ImGui::PushItemWidth(-1);
      if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer),
                           ImGuiInputTextFlags_EnterReturnsTrue)) {
        data.Name = nameBuffer;
        s_DoubleClicked = entt::null;
      }
      if (doubleClicked) {
        ImGui::PopStyleVar();
      }
    }

    if (!active) {
      ImGui::PopStyleColor();
    }

    bool deleteEntity = false;

    if (ImGui::BeginPopupContextItem(data.ID.ToString().c_str())) {
      Engine::Entity{entity, scene}.GetComponent<MetadataComponent>().Selected =
          true;
      UIManager::Get()->SetSelectedEntity(entity);
      Menu();
      ImGui::EndPopup();
    }

    if (ImGui::BeginDragDropTarget()) {
      const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Drag_Entity");
      if (payload) {
        entt::entity droppedEntity = *(entt::entity *)payload->Data;
        if (droppedEntity != entity) {
          auto attachEvent = CreateUnique<Engine::AttachToEntityEvent>();
          attachEvent->entity = droppedEntity;
          attachEvent->parent = entity;
          Engine::EventBus::Get()->DispatchSync(std::move(attachEvent));
          s_HadRecentDroppedEntity = entity;
        }
      }
      ImGui::EndDragDropTarget();
    }

    if (deleteEntity) {
      auto deleteEntityEvent = CreateUnique<Engine::DeleteEntityEvent>();
      deleteEntityEvent->entity = entity;
      Engine::EventBus::Get()->DispatchSync(std::move(deleteEntityEvent));

      if (nodeOpen) {
        ImGui::TreePop();
      }
      ImGui::PopID();
      return;
    }

    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x -
                    ImGui::CalcTextSize(ICON_LC_EYE).x * 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));
    if (ImGui::Button(active ? ICON_LC_EYE : ICON_LC_EYE_CLOSED)) {
      data.Visible = !active;
      ToggleVisibility(registry, entity, data.Visible);
    }
    ImGui::PopStyleColor();

    if (!nodeOpen) {
      ImGui::PopID();
      return;
    }

    const ImColor TreeLineColor = ImColor(128, 128, 128, 128);
    const f32 SmallOffsetX = 6.0f;
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
    verticalLineStart.x += SmallOffsetX;
    ImVec2 verticalLineEnd = verticalLineStart;

    if (!noChildren) {
      for (auto child : node.Children) {
        if (!registry.valid(child) ||
            !registry.all_of<MetadataComponent, SceneNodeComponent>(child))
          continue;

        f32 HorizontalTreeLineSize = 20.0f;
        auto currentPos = ImGui::GetCursorScreenPos();
        ImGui::Indent(10.0f);

        DisplayEntityNode(scene, child);
        ImGui::Unindent(10.0f);

        const ImRect childRect = ImRect(
            currentPos,
            ImVec2(currentPos.x, currentPos.y + ImGui::GetFontSize() +
                                     (ImGui::GetStyle().FramePadding.y * 2)));
        const f32 midpoint = (childRect.Min.y + childRect.Max.y) * 0.5f;
        drawList->AddLine(
            ImVec2(verticalLineStart.x, midpoint),
            ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint),
            TreeLineColor);
        verticalLineEnd.y = midpoint;
      }
    }

    drawList->AddLine(verticalLineStart, verticalLineEnd, TreeLineColor);

    ImGui::TreePop();
    ImGui::PopID();
  }
}

void Hierarchy::Draw() {
  auto scene = Engine::Controller::Get()->GetSceneManager().GetActiveScene();
  if (!scene)
    return;

  auto flags = ImGuiWindowFlags_NoCollapse;
  s_CurrentPrevious = entt::null;

  if (ImGui::Begin((std::string(ICON_LC_LAYERS_2) + " Scene Hierarchy").c_str(),
                   nullptr, flags)) {
    auto &registry = scene->GetRegistry();

    ImGui::PushStyleColor(ImGuiCol_MenuBarBg,
                          ImGui::GetStyleColorVec4(ImGuiCol_TabActive));

    ImGui::TextUnformatted(ICON_LC_SEARCH);
    ImGui::SameLine();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,
                          ImGui::GetColorU32(ImGuiCol_TitleBg));
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetColorU32(ImGuiCol_Border));
    s_HierarchyFilter.Draw("##HierarchyFilter",
                           ImGui::GetContentRegionAvail().x -
                               ImGui::GetStyle().IndentSpacing);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    if (!s_HierarchyFilter.IsActive() && !ImGui::IsItemActive()) {
      ImVec2 pos = ImGui::GetItemRectMin();
      ImVec2 size = ImGui::GetItemRectSize();
      ImDrawList *drawList = ImGui::GetWindowDrawList();
      ImVec2 textPos = pos;
      textPos.x += ImGui::GetStyle().FramePadding.x + 4.0f;
      textPos.y += (size.y - ImGui::GetFontSize()) * 0.5f;
      drawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_TextDisabled),
                        "Search...");
    }

    ImGui::PopStyleColor();
    ImGui::Unindent();

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
        ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
      UIManager::Get()->SetSelectedEntity(entt::null);
      ImGui::OpenPopup("SceneHierarchyPopup");
    }

    PopupMenu();

    ImGui::Indent();

    auto view = registry.view<SceneNodeComponent>();
    for (auto entity : view) {
      auto &node = registry.get<SceneNodeComponent>(entity);
      if (node.Parent == entt::null) {
        DisplayEntityNode(scene, entity);
      }
    }

    ImVec2 minSpace = ImGui::GetWindowContentRegionMin();
    ImVec2 maxSpace = ImGui::GetWindowContentRegionMax();
    minSpace.x += ImGui::GetWindowPos().x + 1.0f;
    minSpace.y += ImGui::GetWindowPos().y + 1.0f;
    maxSpace.x += ImGui::GetWindowPos().x - 1.0f;
    maxSpace.y += ImGui::GetWindowPos().y - 1.0f;
    ImRect windowRect = {minSpace, maxSpace};

    if (ImGui::BeginDragDropTargetCustom(windowRect,
                                         ImGui::GetCurrentWindow()->ID)) {
      const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(
          "Drag_Entity", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
      if (payload) {
        entt::entity droppedEntity = *(entt::entity *)payload->Data;
        auto disownEvent = CreateUnique<Engine::DisownEntityEvent>();
        disownEvent->entity = droppedEntity;
        Engine::EventBus::Get()->DispatchSync(std::move(disownEvent));
      }
      ImGui::EndDragDropTarget();
    }

    if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
      entt::entity selected = UIManager::Get()->GetSelectedEntity();
      if (selected != entt::null) {
        auto deleteEntityEvent = CreateUnique<Engine::DeleteEntityEvent>();
        deleteEntityEvent->entity = selected;
        Engine::EventBus::Get()->DispatchSync(std::move(deleteEntityEvent));
      }
    }
  }
  ImGui::End();
}
void Hierarchy::Menu() {
  entt::entity selected = UIManager::Get()->GetSelectedEntity();

  if (selected != entt::null) {
    ImGui::SeparatorText("Family");

    if (ImGui::MenuItem(ICON_LC_PLUS " Create empty child entity")) {
      auto createEntityEvent = CreateUnique<Engine::CreateChildEntityEvent>();
      createEntityEvent->parentEntity = selected;
      Engine::EventBus::Get()->DispatchSync(std::move(createEntityEvent));
    }

    bool hasParent = false;

    auto hasParentQuery = CreateUnique<Engine::EntityHasParentQuery>();
    hasParentQuery->entity = selected;
    hasParentQuery->callback = [&hasParent](const Engine::EventResult &result) {
      if (result.IsSuccess()) {
        auto parentExists = result.GetData<bool>();
        if (parentExists && *parentExists) {
          hasParent = true;
        }
      } else {
        AQUILA_LOG_WARNING("Failed to check if entity has parent: {}",
                           result.errorMessage);
      }
    };

    auto hasParentResult =
        Engine::EventBus::Get()->DispatchSync(std::move(hasParentQuery));
    if (!hasParentResult.IsSuccess()) {
      AQUILA_LOG_ERROR("Failed to query entity parent status: {}",
                       hasParentResult.errorMessage);
    }

    std::vector<Engine::Entity> attachableEntities;

    auto attachableQuery = CreateUnique<Engine::GetAttachableEntitiesQuery>();
    attachableQuery->entity = selected;
    attachableQuery->callback =
        [&attachableEntities](const Engine::EventResult &result) {
          if (result.IsSuccess()) {
            auto entities = result.GetData<std::vector<Engine::Entity>>();
            if (entities) {
              attachableEntities = *entities;
            }
          } else {
            AQUILA_LOG_WARNING("Failed to get attachable entities: {}",
                               result.errorMessage);
          }
        };

    auto attachableResult =
        Engine::EventBus::Get()->DispatchSync(std::move(attachableQuery));
    if (!attachableResult.IsSuccess()) {
      AQUILA_LOG_ERROR("Failed to query attachable entities: {}",
                       attachableResult.errorMessage);
    }

    if (!attachableEntities.empty()) {
      if (ImGui::BeginMenu(ICON_LC_LINK " Attach to")) {
        for (auto &entity : attachableEntities) {
          ImGui::PushID(entity.GetUUID().ToString().c_str());
          if (ImGui::MenuItem(entity.GetName().c_str())) {
            auto attachEvent = CreateUnique<Engine::AttachToEntityEvent>();
            attachEvent->entity = selected;
            attachEvent->parent = entity.GetHandle();
            attachEvent->callback = [entityName = entity.GetName()](
                                        const Engine::EventResult &result) {
              if (result.IsSuccess()) {
                AQUILA_LOG_INFO("Successfully attached entity to {}",
                                entityName);
              } else {
                AQUILA_LOG_ERROR("Failed to attach entity to {}: {}",
                                 entityName, result.errorMessage);
              }
            };

            Engine::EventBus::Get()->DispatchSync(std::move(attachEvent));
          }
          ImGui::PopID();
        }
        ImGui::EndMenu();
      }
    }

    if (hasParent) {
      if (ImGui::MenuItem(ICON_LC_UNLINK " Disown entity")) {
        auto disownEvent = CreateUnique<Engine::DisownEntityEvent>();
        disownEvent->entity = selected;
        disownEvent->callback = [](const Engine::EventResult &result) {
          if (result.IsSuccess()) {
            AQUILA_LOG_INFO("Successfully disowned entity");
          } else {
            AQUILA_LOG_ERROR("Failed to disown entity: {}",
                             result.errorMessage);
          }
        };

        Engine::EventBus::Get()->DispatchAsyncNoResult(std::move(disownEvent));
      }
    }

    ImGui::SeparatorText("Create Objects");

    if (ImGui::MenuItem(ICON_LC_PLUS " Create empty entity")) {
      auto addEntityEvent = CreateUnique<Engine::AddEntityEvent>();
      Engine::EventBus::Get()->DispatchSync(std::move(addEntityEvent));
    }

    ImGui::Separator();

    if (ImGui::BeginMenu("3D Objects")) {
      if (ImGui::MenuItem("Cube")) {
      }
      if (ImGui::MenuItem("Sphere")) {
      }
      if (ImGui::MenuItem("Cylinder")) {
      }
      if (ImGui::MenuItem("Capsule")) {
      }
      if (ImGui::MenuItem("Plane")) {
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Lights")) {
      if (ImGui::MenuItem("Directional Light")) {
      }
      if (ImGui::MenuItem("Point Light")) {
      }
      if (ImGui::MenuItem("Spot Light")) {
      }
      if (ImGui::MenuItem(" Environment Light")) {
      }
      if (ImGui::MenuItem("Area Light")) {
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Cameras")) {
      if (ImGui::MenuItem("Perspective Camera")) {
      }
      if (ImGui::MenuItem("Orthographic Camera")) {
      }
      ImGui::EndMenu();
    }

    ImGui::Separator();

    if (ImGui::MenuItem(ICON_LC_TRASH " Delete entity")) {
      auto deleteEntityEvent = CreateUnique<Engine::DeleteEntityEvent>();
      deleteEntityEvent->entity = selected;
      Engine::EventBus::Get()->DispatchSync(std::move(deleteEntityEvent));
    }
  } else {

    if (ImGui::MenuItem(ICON_LC_PLUS " Create empty entity")) {
      auto addEntityEvent = CreateUnique<Engine::AddEntityEvent>();

      Engine::EventBus::Get()->DispatchSync(std::move(addEntityEvent));
    }

    ImGui::Separator();

    if (ImGui::BeginMenu("3D Objects")) {
      if (ImGui::MenuItem("Cube")) {
        auto addEntityEvent = CreateUnique<Engine::AddEntityEvent>();
        addEntityEvent->entityName = "Cube";
        addEntityEvent->preset = Engine::EntityPreset::Cube;
        Engine::EventBus::Get()->DispatchSync(std::move(addEntityEvent));
      }
      if (ImGui::MenuItem("Sphere")) {
        auto addEntityEvent = CreateUnique<Engine::AddEntityEvent>();

        addEntityEvent->entityName = "Sphere";
        addEntityEvent->preset = Engine::EntityPreset::Sphere;
        Engine::EventBus::Get()->DispatchSync(std::move(addEntityEvent));
      }
      if (ImGui::MenuItem("Cylinder")) {
        auto addEntityEvent = CreateUnique<Engine::AddEntityEvent>();

        addEntityEvent->entityName = "Cylinder";
        addEntityEvent->preset = Engine::EntityPreset::Cylinder;
        Engine::EventBus::Get()->DispatchSync(std::move(addEntityEvent));
      }
      if (ImGui::MenuItem("Plane")) {
        auto addEntityEvent = CreateUnique<Engine::AddEntityEvent>();

        addEntityEvent->entityName = "Plane";
        addEntityEvent->preset = Engine::EntityPreset::Plane;
        Engine::EventBus::Get()->DispatchSync(std::move(addEntityEvent));
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Lights")) {
      if (ImGui::MenuItem("Directional Light")) {
        auto addEntityEvent = CreateUnique<Engine::AddEntityEvent>();

        addEntityEvent->entityName = "Directional Light";
        addEntityEvent->preset = Engine::EntityPreset::DirectionalLight;
        Engine::EventBus::Get()->DispatchSync(std::move(addEntityEvent));
      }
      if (ImGui::MenuItem("Point Light")) {
        auto addEntityEvent = CreateUnique<Engine::AddEntityEvent>();

        addEntityEvent->entityName = "Point Light";
        addEntityEvent->preset = Engine::EntityPreset::PointLight;
        Engine::EventBus::Get()->DispatchSync(std::move(addEntityEvent));
      }
      if (ImGui::MenuItem("Spot Light")) {
        auto addEntityEvent = CreateUnique<Engine::AddEntityEvent>();

        addEntityEvent->entityName = "Spot Light";
        addEntityEvent->preset = Engine::EntityPreset::SpotLight;
        Engine::EventBus::Get()->DispatchSync(std::move(addEntityEvent));
      }
      if (ImGui::MenuItem(" Environment Light")) {
      }
      if (ImGui::MenuItem("Area Light")) {
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Cameras")) {
      if (ImGui::MenuItem("Perspective Camera")) {
        auto addEntityEvent = CreateUnique<Engine::AddEntityEvent>();

        addEntityEvent->entityName = "Perspective Camera";
        addEntityEvent->preset = Engine::EntityPreset::PerspectiveCamera;
        Engine::EventBus::Get()->DispatchSync(std::move(addEntityEvent));
      }
      if (ImGui::MenuItem("Orthographic Camera")) {
        auto addEntityEvent = CreateUnique<Engine::AddEntityEvent>();

        addEntityEvent->entityName = "Orthographic Camera";
        addEntityEvent->preset = Engine::EntityPreset::OrthographicCamera;
        Engine::EventBus::Get()->DispatchSync(std::move(addEntityEvent));
      }
      ImGui::EndMenu();
    }
  }
}

void Hierarchy::ToggleVisibility(entt::registry &registry, entt::entity &entity,
                                 bool &value) {
  if (!registry.valid(entity) ||
      !registry.all_of<MetadataComponent, SceneNodeComponent>(entity))
    return;

  auto &node = registry.get<SceneNodeComponent>(entity);
  if (!node.Children.empty()) {
    for (auto &child : node.Children) {
      auto &meta = registry.get<MetadataComponent>(child);
      meta.Visible = value;
      ToggleVisibility(registry, child, value);
    }
  }
}

void Hierarchy::PopupMenu() {
  if (ImGui::BeginPopup("SceneHierarchyPopup", ImGuiWindowFlags_NoMove)) {

    Menu();

    ImGui::EndPopup();
  }
}
} // namespace Editor::Panels