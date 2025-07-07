#include "Elements/HierarchyElement.h"
#include "UI.h"

namespace Editor::UIManagement {
    void HierarchyElement::DisplayEntityNode(Engine::AquilaScene* scene, entt::entity entity)
    {
        auto& registry = scene->GetRegistry();
        if (!registry.valid(entity) || !registry.all_of<MetadataComponent, SceneNodeComponent>(entity))
            return;

        auto& data = registry.get<MetadataComponent>(entity);
        auto& node = registry.get<SceneNodeComponent>(entity);

        std::string label = ICON_LC_CUBOID + std::string(" ") + data.Name;

        ImGui::PushID(data.ID.ToString().c_str());
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DrawLinesToNodes;

            if (UIManager::s_SelectedEntity == entity)
                flags |= ImGuiTreeNodeFlags_Selected;

            if (node.Children.empty())
                flags |= ImGuiTreeNodeFlags_Leaf;

            bool opened = ImGui::TreeNodeEx(label.c_str(), flags, "%s", label.c_str());

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                UIManager::s_SelectedEntity = entity;

            if (ImGui::BeginPopupContextItem())
            {
                UIManager::s_SelectedEntity = entity;
                Menu();
                ImGui::EndPopup();
            }

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() * 0.5f);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Entity");

            ImGui::TableSetColumnIndex(2);

            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0,0,0,0));

            float columnWidth = ImGui::GetColumnWidth();
            float iconWidth = 20.0f;
            float offsetX = (columnWidth - iconWidth) * 0.5f;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

            bool clicked = ImGui::Selectable(data.Enabled ? ICON_LC_EYE : ICON_LC_EYE_CLOSED, false, ImGuiSelectableFlags_None, ImVec2(iconWidth, 0));

            ImGui::PopStyleColor(3);

            if (clicked)
            {
                data.Enabled = !data.Enabled;
                ToggleVisibility(registry, entity, data.Enabled);
            }

            if (opened)
            {
                auto childrenCopy = node.Children;
                for (auto child : childrenCopy)
                {
                    DisplayEntityNode(scene, child);
                }
                ImGui::TreePop();
            }
        }
        ImGui::PopID();
    }

    void HierarchyElement::ToggleVisibility(entt::registry& registry, entt::entity& entity, bool& value){
        if (!registry.valid(entity) || !registry.all_of<MetadataComponent, SceneNodeComponent>(entity))
            return;

        auto& node = registry.get<SceneNodeComponent>(entity);
        if (!node.Children.empty()){
            for (auto& child : node.Children){
                auto& meta = registry.get<MetadataComponent>(child);
                meta.Enabled = value;
                ToggleVisibility(registry, child, value);
            }
        }
    }

    void HierarchyElement::DisplayHierarchy(Engine::AquilaScene* scene)
    {
        auto& registry = scene->GetRegistry();
        auto view = registry.view<SceneNodeComponent>();

        if (ImGui::BeginTable("EntityHierarchyTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Label");
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Visibility", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableHeadersRow();

            for (auto entity : view)
            {
                auto& node = registry.get<SceneNodeComponent>(entity);
                if (node.Parent == entt::null)
                {
                    DisplayEntityNode(scene, entity);
                }
            }

            ImGui::EndTable();
        }
    }

    void HierarchyElement::Menu() {
        if (UIManager::s_SelectedEntity != entt::null){
            ImGui::SeparatorText("Family");

            if (ImGui::MenuItem(ICON_LC_PLUS " Create empty child entity")){

                Engine::EventBus::Get().Dispatch(UICommandEvent{
                    UICommand::CreateChildEntity,
                    { {"entity", UIManager::s_SelectedEntity} }
                });
            }

            bool hasParent = false;

            Engine::EventBus::Get().Dispatch(QueryEvent{
                QueryCommand::EntityHasParent,
                { {"entity", UIManager::s_SelectedEntity} },
                [&hasParent](UIEventResult result, CommandParam payload) {
                    if (result == UIEventResult::Failure) {
                        hasParent = true;
                    }
                }
            });

            std::vector<Engine::AqEntity> attachableEntities;

            Engine::EventBus::Get().Dispatch(QueryEvent{
                QueryCommand::GetAttachableEntities,
                { {"entity", UIManager::s_SelectedEntity} },
                [&attachableEntities](UIEventResult result, CommandParam payload) {
                    if (result == UIEventResult::Success) {
                        attachableEntities = std::get<std::vector<Engine::AqEntity>>(payload);
                    }
                }
            });

            if (!attachableEntities.empty()) {
                if (ImGui::BeginMenu(ICON_LC_LINK " Attach to")) {
                    for (auto& entity : attachableEntities) {
                        ImGui::PushID(entity.GetUUID().ToString().c_str());
                        if (ImGui::MenuItem(entity.GetName().c_str())) {
                            Engine::EventBus::Get().Dispatch(UICommandEvent{
                                UICommand::AttachToEntity,
                                { {"entity", UIManager::s_SelectedEntity}, {"parent", entity.GetHandle()} }
                            });
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndMenu();
                }
            }


            if (hasParent) {
                if (ImGui::MenuItem(ICON_LC_UNLINK " Disown entity")) {
                    Engine::EventBus::Get().Dispatch(UICommandEvent{
                        UICommand::DisownEntity,
                        { {"entity", UIManager::s_SelectedEntity} }
                    });
                }
            }


            ImGui::SeparatorText("Primitives");

            if (ImGui::BeginMenu("Create primitive")) {
                if (ImGui::MenuItem("Cube")) {}
                if (ImGui::MenuItem("Sphere")) {}
                if (ImGui::MenuItem("Cylinder")) {}
                if (ImGui::MenuItem("Capsule")) {}
                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem(ICON_LC_TRASH " Delete entity")) {
                Engine::EventBus::Get().Dispatch(UICommandEvent{
                    UICommand::DeleteEntity,
                    { {"entity", UIManager::s_SelectedEntity} }
                });
            }
        } else {
            if (ImGui::MenuItem(ICON_LC_PLUS " Create empty entity")) {
                Engine::EventBus::Get().Dispatch(UICommandEvent{
                    UICommand::AddEntity
                });
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("Primitives")) {
                if (ImGui::MenuItem("Cube")) {}
                if (ImGui::MenuItem("Sphere")) {}
                if (ImGui::MenuItem("Cylinder")) {}
                if (ImGui::MenuItem("Capsule")) {}
                ImGui::EndMenu();
            }
        }
    }

    void HierarchyElement::PopupMenu(){
        if (ImGui::BeginPopup("SceneHierarchyPopup", ImGuiWindowFlags_NoMove)){

            Menu();

            ImGui::EndPopup();
        }
    }


    void HierarchyElement::Draw(void* data){
        Engine::AquilaScene* scene = static_cast<Engine::AquilaScene*>(data);
        if (!scene) return;

        ImGui::Begin(ICON_LC_LAYERS_2 " Scene Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse);
        
        DisplayHierarchy(scene);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            UIManager::s_SelectedEntity = entt::null;
            ImGui::OpenPopup("SceneHierarchyPopup");
        }

        PopupMenu();

        ImGui::End();
    }
}