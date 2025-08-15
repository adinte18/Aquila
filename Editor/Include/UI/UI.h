#ifndef UI_H
#define UI_H

#include "UI/Elements/ContentBrowserElement.h"
#include "UI/Elements/HierarchyElement.h"
#include "UI/Elements/ViewportElement.h"
#include "UI/Elements/MenubarElement.h"
#include "UI/Elements/PropertiesElement.h"

#include "UI/UIConfig.h"

#include "UI/FontManager.h"
#include "UI/ThemeManager.h"

#include "ImGuizmo/ImGuizmo.h"
#include "nativefiledialog/src/nfd_common.h"


namespace Editor {
    class UIManager {
    public:
        void OnStart();
        void OnUpdate(VkCommandBuffer commandBuffer);
        void OnEnd();

        static UIManager& Get() {
            static UIManager instance;
            return instance;
        }

        entt::entity GetSelectedEntity() const { return m_SelectedEntity; }
        void SetSelectedEntity(entt::entity entity) { m_SelectedEntity = entity; }
                
        VkDescriptorSet GetFinalImage() const { return m_FinalImage; }
        void SetFinalImage(VkDescriptorSet image) { m_FinalImage = image; }

        VkDescriptorSet FetchRenderedImage() { return m_FinalImage; }

    private:
        UIManager() = default;

        Elements::MenubarElement m_Menubar{};
        Elements::ContentElement m_ContentBrowser{};
        Elements::HierarchyElement m_Hierarchy{};
        Elements::ViewportElement m_Viewport{};
        Elements::PropertiesElement m_Properties{};

        void SetupIMGUI();
        void SetupDescriptorSets();
        void SetupDockspace();

        entt::entity m_SelectedEntity = entt::null;

        Unique<Engine::DescriptorPool> m_DescriptorPool;

        VkDescriptorSet m_FinalImage{};
    };
}

#endif //UI_H
