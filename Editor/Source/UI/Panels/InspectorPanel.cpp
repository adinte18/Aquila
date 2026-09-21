#include "UI/Panels/InspectorPanel.h"
#include "Aquila/Scene/ComponentRegistry.h"
#include "Aquila/Scene/DefaultScenes.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/UI/Widgets/Label.h"
#include "UI/Inspectors/MaterialComponentUI.h"
#include "UI/Inspectors/MeshComponentUI.h"
#include "UI/Inspectors/ReflectedComponentUI.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Collapsible.h"
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

void InspectorPanel::build(UI::Core::View *panel, UI::Core::View *overlay_root) {
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

	m_actor_uuid = panel->find_by_id<UI::Core::Label>("inspector-uuid");
	if (m_actor_uuid != nullptr) {
		set_visible(m_actor_uuid, false);
	}

	build_component_registry();

	m_add_button = panel->find_by_id<UI::Core::Button>("inspector-add-component");
	if (m_add_button != nullptr) {
		m_add_button->on_click.connect([this] { open_add_menu(); });
	}

	if (overlay_root != nullptr) {
		auto popup = std::make_unique<UI::Core::PopupMenu>();
		m_add_popup = dynamic_cast<UI::Core::PopupMenu *>(overlay_root->add_child(std::move(popup)));
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

		if (component_ui->is_removable()) {
			IComponentUI *ui = component_ui.get();
			GFX::GfxTexture *trash = m_texture_cache != nullptr ? m_texture_cache->load("Engine/UI/Icons/trash.png") : nullptr;
			section->add_header_action(trash, "Remove component", [this, ui] {
				if (!m_has_current) {
					return;
				}
				ui->remove(m_current_entity);
				show_entity(m_current_entity);
				on_components_changed(m_current_entity);
			});
		}

		m_sections.push_back({ .collapsible = section, .id = std::string(section_id), .ui = std::move(component_ui) });
	};

	const auto &registry = ComponentRegistry::instance();
	auto reflected = [&](std::string_view component) {
		return std::make_unique<ReflectedComponentUI>(*registry.find(component), &m_context);
	};

	add_ui_component("section-transform", reflected("Transform"));
	add_ui_component("section-mesh", std::make_unique<MeshComponentUI>(*registry.find("Mesh")));
	add_ui_component("section-material", std::make_unique<MaterialComponentUI>(*registry.find("Material"), m_context));
	add_ui_component("section-light", reflected("Light"));
	add_ui_component("section-skylight", reflected("Sky Light"));
	add_ui_component("section-camera", reflected("Camera"));

	clear();
}

void InspectorPanel::show_entity(Entity entity) {
	if (m_scroll_view == nullptr) {
		return;
	}

	set_visible(m_empty_state, false);
	set_visible(m_scroll_view, true);

	set_visible(m_name_input, true);
	set_visible(m_actor_uuid, true);

	m_actor_uuid->set_text("UUID : " + entity.get_uuid().to_string());
	m_name_input->set_text(entity.get_name());

	for (Section &section : m_sections) {
		const bool has = section.ui->matches(entity);
		set_visible(section.collapsible, has);
		if (has) {
			section.ui->show(entity);
		}
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
		m_default_material = SceneManagement::make_default_material(m_context);
	}
	return m_default_material;
}

void InspectorPanel::attach_default_material(Entity entity) {
	if (entity.has_component<MaterialComponent>()) {
		return;
	}
	auto &material = entity.add_component<MaterialComponent>(ensure_default_material());
	material.surface_properties.albedo = Vec4(0.8F, 0.8F, 0.8F, 1.0F);
	material.surface_properties.metallic = 0.0F;
	material.surface_properties.roughness = 0.6F;
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
			on_components_changed(m_current_entity);
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
