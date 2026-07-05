#pragma once

#include "Aquila/Scene/Entity.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "UI/Inspectors/IComponentUI.h"
#include "UI/Panels/IEditorPanel.h"

namespace Aquila::GFX {
class GfxContext;
}
namespace Aquila::UI::Core {
class Collapsible;
class DockPanel;
class PropertyGrid;
class TextInput;
class View;
} // namespace Aquila::UI::Core

namespace Editor {

class InspectorPanel : public IEditorPanel {
  public:
	explicit InspectorPanel(Aquila::GFX::GfxContext &context);
	void build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlay_root) override;
	void show_entity(Aquila::SceneManagement::Entity entity);

  private:
	void set_visible(Aquila::UI::Core::View *v, bool visible);

	Aquila::GFX::GfxContext &m_context;
	Aquila::UI::Core::View *m_scroll_view = nullptr;
	Aquila::UI::Core::TextInput *m_name_input = nullptr;

	struct Section {
		Aquila::UI::Core::Collapsible *collapsible = nullptr;
		Unique<IComponentUI> ui;
	};
	std::vector<Section> m_sections;
};

} // namespace Editor
