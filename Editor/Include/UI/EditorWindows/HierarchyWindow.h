#pragma once

#include "Core/EditorWindow.h"

namespace Editor {

class HierarchyPanel;

class HierarchyWindow final : public EditorWindow {
  public:
	explicit HierarchyWindow(EditorContext &context);
	~HierarchyWindow() override;

	void build(Aquila::UI::Core::View &content) override;

  private:
	void show_selection(Aquila::SceneManagement::Entity entity);

	Unique<HierarchyPanel> m_panel;
	bool m_syncing = false;
};

}
