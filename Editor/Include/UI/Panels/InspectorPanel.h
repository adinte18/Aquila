#pragma once

#include "Aquila/Foundation/UUID.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "UI/Inspectors/IComponentUI.h"
#include "UI/Panels/IEditorPanel.h"

#include <string>
#include <unordered_map>
#include <vector>

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
	void clear();

  private:
	void set_visible(Aquila::UI::Core::View *v, bool visible);

	struct EntityLayout {
		std::vector<std::string> order;
		std::unordered_map<std::string, bool> expanded;
	};

	[[nodiscard]] EntityLayout default_layout() const;
	void apply_layout(const EntityLayout &layout);
	void capture_layout();

	Aquila::GFX::GfxContext &m_context;
	Aquila::UI::Core::View *m_scroll_view = nullptr;
	Aquila::UI::Core::TextInput *m_name_input = nullptr;

	struct Section {
		Aquila::UI::Core::Collapsible *collapsible = nullptr;
		std::string id;
		Unique<IComponentUI> ui;
	};
	std::vector<Section> m_sections;

	std::unordered_map<Aquila::Foundation::UUID, EntityLayout> m_entity_layouts;
	Aquila::Foundation::UUID m_current_uuid = Aquila::Foundation::UUID::null();
	bool m_has_current = false;
};

} // namespace Editor
