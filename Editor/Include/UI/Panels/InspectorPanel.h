#pragma once

#include "Aquila/Foundation/UUID.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "Aquila/UI/Widgets/ScrollView.h"
#include "UI/Inspectors/IComponentUI.h"
#include "UI/Panels/IEditorPanel.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace Aquila::GFX {
class GfxContext;
class GfxTexture;
} // namespace Aquila::GFX
namespace Aquila::UI::Core {
class Button;
class Collapsible;
class DockPanel;
class PopupMenu;
class PropertyGrid;
class TextInput;
class TextureCache;
class View;
} // namespace Aquila::UI::Core

namespace Editor {

class InspectorPanel : public IEditorPanel {
  public:
	InspectorPanel(Aquila::GFX::GfxContext &context, Aquila::UI::Core::TextureCache *texture_cache);
	void build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlay_root) override;
	Signal<void(Aquila::SceneManagement::Entity)> on_entity_renamed;
	void show_entity(Aquila::SceneManagement::Entity entity);
	void clear();
	void open_add_search(Vec2 canvas_pos);

  private:
	void set_visible(Aquila::UI::Core::View *v, bool visible);

	struct EntityLayout {
		std::vector<std::string> order;
		std::unordered_map<std::string, bool> expanded;
	};

	[[nodiscard]] EntityLayout default_layout() const;
	void apply_layout(const EntityLayout &layout);
	void capture_layout();

	struct AddableComponent {
		std::string name;
		std::string category;
		Delegate<bool(Aquila::SceneManagement::Entity)> present;
		Delegate<void(Aquila::SceneManagement::Entity)> attach;
	};

	struct ComponentCategory {
		std::string name;
		Aquila::GFX::GfxTexture *icon = nullptr;
	};

	void build_component_registry();
	void open_add_menu();
	void open_add_popup_at(Vec2 canvas_pos, bool swallow_first_char);
	void populate_add_menu(const std::string &query);

	void toggle_signal_observe(size_t section_index, size_t row_index);
	void on_signal_fired(size_t section_index, size_t row_index);
	void reset_signal_rows();

	Aquila::GFX::GfxContext &m_context;
	Aquila::UI::Core::TextureCache *m_texture_cache = nullptr;
	Aquila::UI::Core::ScrollView *m_scroll_view = nullptr;
	Aquila::UI::Core::View *m_section_list = nullptr;
	Aquila::UI::Core::View *m_empty_state = nullptr;
	Aquila::UI::Core::TextInput *m_name_input = nullptr;
	Aquila::UI::Core::Button *m_add_button = nullptr;
	Aquila::UI::Core::PopupMenu *m_add_popup = nullptr;
	Aquila::UI::Core::TextInput *m_add_search = nullptr;
	std::vector<AddableComponent> m_addable;
	std::vector<ComponentCategory> m_categories;

	struct SignalRow {
		const char *name = nullptr;
		Signal<void()> *signal = nullptr;
		Aquila::UI::Core::Button *button = nullptr;
		Signal<void()>::Connection connection;
		int fired = 0;
		bool observing = false;
	};

	struct Section {
		Aquila::UI::Core::Collapsible *collapsible = nullptr;
		std::string id;
		Unique<IComponentUI> ui;
		Aquila::UI::Core::Collapsible *signals_group = nullptr;
		Aquila::UI::Core::PropertyGrid *signals_grid = nullptr;
		std::vector<SignalRow> signal_rows;
	};
	std::vector<Section> m_sections;

	std::unordered_map<Aquila::Foundation::UUID, EntityLayout> m_entity_layouts;
	Aquila::Foundation::UUID m_current_uuid = Aquila::Foundation::UUID::null();
	Aquila::SceneManagement::Entity m_current_entity;
	bool m_has_current = false;
};

} // namespace Editor
