#ifndef UI_H
#define UI_H

#include "UI/Panels/ContentBrowser.h"
#include "UI/Panels/Hierarchy.h"
#include "UI/Panels/IPanel.h"
#include "UI/Panels/Menubar.h"
#include "UI/Panels/Properties.h"
#include "UI/Panels/Viewport.h"

#include "UI/UIConfig.h"

#include "UI/FontManager.h"
#include "UI/ThemeManager.h"

#include "ImGuizmo/ImGuizmo.h"
#include "nativefiledialog/src/nfd_common.h"

namespace Editor {
class UIManager : public Utility::Singleton<UIManager> {
  friend class Singleton<UIManager>;

public:
  void OnStart();
  void OnEnd();

  void Render(VkCommandBuffer commandBuffer);

  entt::entity GetSelectedEntity() const { return m_SelectedEntity; }
  void SetSelectedEntity(entt::entity entity) { m_SelectedEntity = entity; }

  VkDescriptorSet GetFinalImage() const { return m_FinalImage; }
  void SetFinalImage(VkDescriptorSet image) { m_FinalImage = image; }

  VkDescriptorSet FetchRenderedImage() { return m_FinalImage; }

private:
  UIManager() = default;

  std::vector<Unique<Panels::IPanel>> m_Panels;

  void SetupIMGUI();
  void SetupDescriptorSets();
  void SetupDockspace();

  entt::entity m_SelectedEntity = entt::null;

  Unique<Engine::DescriptorPool> m_DescriptorPool;

  VkDescriptorSet m_FinalImage{};
};
} // namespace Editor

#endif // UI_H
