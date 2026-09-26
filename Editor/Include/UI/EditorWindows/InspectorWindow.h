#pragma once

#include "Core/EditorWindow.h"

namespace Editor {

class InspectorPanel;

class InspectorWindow final : public EditorWindow {
  public:
	explicit InspectorWindow(EditorContext &context);
	~InspectorWindow() override;

	void build(Aquila::UI::Core::View &content) override;
	void open_add_search(Vec2 canvas_pos);

  private:
	void show(Aquila::SceneManagement::Entity entity);

	Unique<InspectorPanel> m_panel;
};

}
