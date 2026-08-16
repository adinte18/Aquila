#include "UI/Panels/InspectorPanel.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "UI/Inspectors/CameraComponentUI.h"
#include "UI/Inspectors/LightComponentUI.h"
#include "UI/Inspectors/MaterialComponentUI.h"
#include "UI/Inspectors/MeshComponentUI.h"
#include "UI/Inspectors/SkyLightComponentUI.h"
#include "UI/Inspectors/TransformComponentUI.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Collapsible.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/PopupMenu.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/TextInput.h"
#include "Aquila/UI/Core/TextureCache.h"

#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/SkyLightComponent.h"
#include "Aquila/Graphics/Material/MaterialFactory.h"
#include "Aquila/Foundation/SharedConstants.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

InspectorPanel::InspectorPanel(GFX::GfxContext &context, UI::Core::TextureCache *texture_cache)
	: m_context(context), m_texture_cache(texture_cache) {}

void InspectorPanel::set_visible(UI::Core::View *v, bool visible) {
	v->set_hidden(!visible);
}

void InspectorPanel::build(UI::Core::DockPanel *panel, UI::Core::View *overlay_root) {
	m_scroll_view = panel->find_by_id<UI::Core::ScrollView>("inspector-scroll");
	if (m_scroll_view == nullptr) {
		AQUILA_LOG_ERROR("InspectorPanel: 'inspector-scroll' not found in layout");
		return;
	}

	m_section_list = panel->find_by_id("inspector-list");
	if (m_section_list == nullptr) {
		AQUILA_LOG_ERROR("InspectorPanel: 'inspector-list' not found in layout");
		return;
	}

	m_empty_state = panel->find_by_id("inspector-empty");

	m_name_input = panel->find_by_id<UI::Core::TextInput>("inspector-name");
	if (m_name_input != nullptr) {
		set_visible(m_name_input, false);
		m_name_input->on_submit.connect([this](const std::string &name) {
			if (!m_has_current) {
				return;
			}
			m_current_entity.set_name(name);
			on_entity_renamed(m_current_entity);
		});
	}

	build_component_registry();

	m_add_button = panel->find_by_id<UI::Core::Button>("inspector-add-component");
	if (m_add_button != nullptr) {
		m_add_button->on_click.connect([this] { open_add_menu(); });
	}

	if (overlay_root != nullptr) {
		auto popup = std::make_unique<UI::Core::PopupMenu>();
		m_add_popup = static_cast<UI::Core::PopupMenu *>(overlay_root->add_child(std::move(popup)));
		if (m_texture_cache != nullptr) {
			m_add_popup->set_submenu_icon(m_texture_cache->load("Engine/UI/Icons/chevron-right.png"));
		}
		m_add_search = m_add_popup->enable_search("Search components...");
		m_add_search->on_changed.connect([this](const std::string &query) { populate_add_menu(query); });
	}

	auto add_ui_component = [&](std::string_view section_id, Unique<IComponentUI> component_ui) {
		auto *section = panel->find_by_id<UI::Core::Collapsible>(section_id);
		if (section == nullptr) {
			AQUILA_LOG_ERROR("InspectorPanel: section '{}' not found in layout", section_id);
			return;
		}
		auto *grid = section->add_child<UI::Core::PropertyGrid>();
		component_ui->build(section, grid);
		set_visible(section, false);
		section->on_reordered.connect([this] { capture_layout(); });
		section->on_toggled.connect([this](bool) { capture_layout(); });

		auto *signals_group = section->add_child<UI::Core::Collapsible>("Signals");
		auto *signals_grid = signals_group->add_child<UI::Core::PropertyGrid>();
		set_visible(signals_group, false);

		m_sections.push_back(
			{ section, std::string(section_id), std::move(component_ui), signals_group, signals_grid, {} });
	};

	add_ui_component("section-transform", std::make_unique<TransformComponentUI>());
	add_ui_component("section-mesh", std::make_unique<MeshComponentUI>());
	add_ui_component("section-material", std::make_unique<MaterialComponentUI>(m_context));
	add_ui_component("section-light", std::make_unique<LightComponentUI>(m_context));
	add_ui_component("section-skylight", std::make_unique<SkyLightComponentUI>(m_context));
	add_ui_component("section-camera", std::make_unique<CameraComponentUI>());

	clear();
}

void InspectorPanel::show_entity(Entity entity) {
	if (m_scroll_view == nullptr) {
		return;
	}

	set_visible(m_empty_state, false);
	set_visible(m_scroll_view, true);

	set_visible(m_name_input, true);
	m_name_input->set_text(entity.get_name());

	reset_signal_rows();

	for (size_t si = 0; si < m_sections.size(); ++si) {
		Section &section = m_sections[si];
		const bool has = section.ui->matches(entity);
		set_visible(section.collapsible, has);
		if (!has) {
			continue;
		}
		section.ui->show(entity);

		for (const ComponentSignal &sig : section.ui->signals(entity)) {
			auto *button = section.signals_grid->add_row<UI::Core::Button>(sig.name);
			button->set_text("Observe");
			SignalRow row;
			row.name = sig.name;
			row.signal = sig.signal;
			row.button = button;
			section.signal_rows.push_back(row);
		}
		for (size_t ri = 0; ri < section.signal_rows.size(); ++ri) {
			section.signal_rows[ri].button->on_click.connect([this, si, ri] { toggle_signal_observe(si, ri); });
		}
		set_visible(section.signals_group, !section.signal_rows.empty());
	}

	m_current_entity = entity;
	m_current_uuid = entity.get_uuid();
	m_has_current = true;

	auto it = m_entity_layouts.find(m_current_uuid);
	apply_layout(it != m_entity_layouts.end() ? it->second : default_layout());
}

void InspectorPanel::clear() {
	if (m_scroll_view == nullptr) {
		return;
	}
	m_has_current = false;
	m_current_entity = Entity::null();
	reset_signal_rows();
	if (m_add_popup != nullptr) {
		m_add_popup->dismiss();
	}
	set_visible(m_name_input, false);
	for (auto &section : m_sections) {
		set_visible(section.collapsible, false);
	}
	set_visible(m_scroll_view, false);
	set_visible(m_empty_state, true);
}

void InspectorPanel::toggle_signal_observe(size_t section_index, size_t row_index) {
	if (section_index >= m_sections.size()) {
		return;
	}
	auto &rows = m_sections[section_index].signal_rows;
	if (row_index >= rows.size()) {
		return;
	}
	SignalRow &row = rows[row_index];
	if (row.signal == nullptr) {
		return;
	}

	if (row.observing) {
		row.signal->disconnect(row.connection);
		row.observing = false;
		row.fired = 0;
		row.button->set_text("Observe");
		return;
	}

	row.connection =
		row.signal->connect([this, section_index, row_index] { on_signal_fired(section_index, row_index); });
	row.observing = true;
	row.fired = 0;
	row.button->set_text("Observing (0)");
}

void InspectorPanel::on_signal_fired(size_t section_index, size_t row_index) {
	if (section_index >= m_sections.size()) {
		return;
	}
	auto &rows = m_sections[section_index].signal_rows;
	if (row_index >= rows.size()) {
		return;
	}
	SignalRow &row = rows[row_index];
	row.fired++;
	row.button->set_text("Observing (" + std::to_string(row.fired) + ")");
	AQUILA_LOG_INFO("Signal '{}' fired ({})", row.name != nullptr ? row.name : "", row.fired);
}

void InspectorPanel::reset_signal_rows() {
	for (auto &section : m_sections) {
		for (auto &row : section.signal_rows) {
			if (row.signal != nullptr && row.observing) {
				row.signal->disconnect(row.connection);
			}
		}
		section.signal_rows.clear();
		if (section.signals_grid != nullptr) {
			while (!section.signals_grid->get_children().empty()) {
				section.signals_grid->remove_child(section.signals_grid->get_children().front().get());
			}
		}
		if (section.signals_group != nullptr) {
			set_visible(section.signals_group, false);
		}
	}
}

InspectorPanel::EntityLayout InspectorPanel::default_layout() const {
	EntityLayout layout;
	for (const auto &section : m_sections) {
		layout.order.push_back(section.id);
		layout.expanded[section.id] = true;
	}
	return layout;
}

void InspectorPanel::apply_layout(const EntityLayout &layout) {
	UI::Core::View *anchor = nullptr;
	for (auto it = layout.order.rbegin(); it != layout.order.rend(); ++it) {
		UI::Core::Collapsible *section = nullptr;
		for (auto &candidate : m_sections) {
			if (candidate.id == *it) {
				section = candidate.collapsible;
				break;
			}
		}
		if (section == nullptr) {
			continue;
		}
		m_section_list->reorder_child(section, anchor);
		anchor = section;

		auto expanded = layout.expanded.find(*it);
		section->set_expanded(expanded != layout.expanded.end() ? expanded->second : true);
	}
}

void InspectorPanel::capture_layout() {
	if (!m_has_current || m_scroll_view == nullptr) {
		return;
	}
	EntityLayout layout;
	for (const auto &child : m_section_list->get_children()) {
		for (const auto &section : m_sections) {
			if (section.collapsible == child.get()) {
				layout.order.push_back(section.id);
				layout.expanded[section.id] = section.collapsible->is_expanded();
				break;
			}
		}
	}
	m_entity_layouts[m_current_uuid] = std::move(layout);
}

void InspectorPanel::build_component_registry() {
	auto icon = [this](const char *path) -> GFX::GfxTexture * {
		return m_texture_cache != nullptr ? m_texture_cache->load(path) : nullptr;
	};

	m_categories.push_back({ "Rendering", icon("Engine/UI/Icons/box.png") });
	m_categories.push_back({ "Lighting", icon("Engine/UI/Icons/lightbulb.png") });
	m_categories.push_back({ "Camera", icon("Engine/UI/Icons/video.png") });

	m_addable.push_back({ "Mesh", "Rendering", [](Entity e) { return e.has_component<MeshComponent>(); },
						  [this](Entity e) {
							  e.add_component<MeshComponent>();
							  attach_default_material(e);
						  } });
	m_addable.push_back({ "Material", "Rendering", [](Entity e) { return e.has_component<MaterialComponent>(); },
						  [this](Entity e) { attach_default_material(e); } });
	m_addable.push_back({ "Light", "Lighting", [](Entity e) { return e.has_component<LightComponent>(); },
						  [](Entity e) { e.add_component<LightComponent>(); } });
	m_addable.push_back({ "Sky Light", "Lighting", [](Entity e) { return e.has_component<SkyLightComponent>(); },
						  [](Entity e) { e.add_component<SkyLightComponent>(); } });
	m_addable.push_back({ "Camera", "Camera", [](Entity e) { return e.has_component<CameraComponent>(); },
						  [](Entity e) { e.add_component<CameraComponent>(); } });
}

Ref<Graphics::Material> InspectorPanel::ensure_default_material() {
	if (!m_default_material) {
		m_default_material = Graphics::MaterialFactory::get()->create(
			m_context, SharedConstants::SHADERS_DIR + "Basic.slang",
			{
				.type = Graphics::MaterialType::Lit,
				.color_formats = { RHI::TextureFormat::RGBA16F },
				.depth_test = true,
				.depth_write = false,
			});
	}
	return m_default_material;
}

void InspectorPanel::attach_default_material(Entity entity) {
	if (entity.has_component<MaterialComponent>()) {
		return;
	}
	auto &material = entity.add_component<MaterialComponent>(ensure_default_material());
	material.surface_properties.albedo = Vec4(0.8f, 0.8f, 0.8f, 1.0f);
	material.surface_properties.metallic = 0.0f;
	material.surface_properties.roughness = 0.6f;
}

void InspectorPanel::open_add_menu() {
	if (m_add_button == nullptr) {
		return;
	}
	const Rect rect = m_add_button->get_absolute_rect();
	open_add_popup_at({ rect.position.x, rect.position.y + rect.size.y }, false);
}

void InspectorPanel::open_add_search(Vec2 canvas_pos) {
	open_add_popup_at(canvas_pos, true);
}

void InspectorPanel::open_add_popup_at(Vec2 canvas_pos, bool swallow_first_char) {
	if (!m_has_current || m_add_popup == nullptr) {
		return;
	}
	if (m_add_search != nullptr) {
		m_add_search->set_text("");
	}
	populate_add_menu("");
	m_add_popup->open_at(canvas_pos);
	if (m_add_search != nullptr) {
		m_add_search->request_focus();
		if (swallow_first_char) {
			m_add_search->ignore_next_char();
		}
	}
}

void InspectorPanel::populate_add_menu(const std::string &query) {
	if (m_add_popup == nullptr) {
		return;
	}
	m_add_popup->clear_items();

	std::string needle = query;
	std::ranges::transform(needle, needle.begin(), [](unsigned char c) { return std::tolower(c); });

	auto attach_item = [this](UI::Core::PopupMenu *menu, const AddableComponent &entry) {
		menu->add_item(entry.name, [this, name = entry.name, attach = entry.attach] {
			if (!m_has_current) {
				return;
			}
			attach(m_current_entity);
			AQUILA_LOG_INFO("Added {} component to '{}'", name, m_current_entity.get_name());
			show_entity(m_current_entity);
		});
	};

	if (!needle.empty()) {
		for (const auto &entry : m_addable) {
			if (entry.present(m_current_entity)) {
				continue;
			}
			std::string name = entry.name;
			std::ranges::transform(name, name.begin(), [](unsigned char c) { return std::tolower(c); });
			if (name.find(needle) != std::string::npos) {
				attach_item(m_add_popup, entry);
			}
		}
		m_add_popup->refresh();
		return;
	}

	m_add_popup->add_separator();

	for (const auto &category : m_categories) {
		UI::Core::PopupMenu *submenu = nullptr;
		for (const auto &entry : m_addable) {
			if (entry.category != category.name || entry.present(m_current_entity)) {
				continue;
			}
			if (submenu == nullptr) {
				submenu = m_add_popup->add_submenu(category.name, category.icon);
			}
			attach_item(submenu, entry);
		}
	}

	m_add_popup->refresh();
}

} // namespace Editor
