#include "UI/Panels/InspectorPanel.h"
#include "UI/Inspectors/CameraComponentUI.h"
#include "UI/Inspectors/LightComponentUI.h"
#include "UI/Inspectors/MaterialComponentUI.h"
#include "UI/Inspectors/TransformComponentUI.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/Collapsible.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/TextInput.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;

InspectorPanel::InspectorPanel(GFX::GfxContext &context) : m_context(context) {}

void InspectorPanel::set_visible(UI::Core::View *v, bool visible) {
	v->set_hidden(!visible);
}

void InspectorPanel::build(UI::Core::DockPanel *panel, UI::Core::View *) {
	m_scroll_view = panel->find_by_id("inspector-scroll");
	if (m_scroll_view == nullptr) {
		AQUILA_LOG_ERROR("InspectorPanel: 'inspector-scroll' not found in layout");
		return;
	}

	m_name_input = panel->find_by_id<UI::Core::TextInput>("inspector-name");
	if (m_name_input != nullptr) {
		set_visible(m_name_input, false);
	}

	auto add_ui_component = [&](std::string_view section_id, Unique<IComponentUI> component_ui) {
		auto *section = panel->find_by_id<UI::Core::Collapsible>(section_id);
		if (section == nullptr) {
			AQUILA_LOG_ERROR("InspectorPanel: section '{}' not found in layout", section_id);
			return;
		}
		auto *grid = section->add_content<UI::Core::PropertyGrid>();
		component_ui->build(section, grid);
		set_visible(section, false);
		section->on_reordered.connect([this] { capture_layout(); });
		section->on_toggled.connect([this](bool) { capture_layout(); });
		m_sections.push_back({ section, std::string(section_id), std::move(component_ui) });
	};

	add_ui_component("section-transform", std::make_unique<TransformComponentUI>());
	add_ui_component("section-material", std::make_unique<MaterialComponentUI>(m_context));
	add_ui_component("section-light", std::make_unique<LightComponentUI>(m_context));
	add_ui_component("section-camera", std::make_unique<CameraComponentUI>());
}

void InspectorPanel::show_entity(Entity entity) {
	if (!m_scroll_view) {
		return;
	}

	set_visible(m_name_input, true);
	m_name_input->set_text(entity.get_name());

	for (auto &section : m_sections) {
		bool has = section.ui->matches(entity);
		set_visible(section.collapsible, has);
		if (has) {
			section.ui->show(entity);
		}
	}

	m_current_uuid = entity.get_uuid();
	m_has_current = true;

	auto it = m_entity_layouts.find(m_current_uuid);
	apply_layout(it != m_entity_layouts.end() ? it->second : default_layout());
}

void InspectorPanel::clear() {
	if (!m_scroll_view) {
		return;
	}
	m_has_current = false;
	set_visible(m_name_input, false);
	for (auto &section : m_sections) {
		set_visible(section.collapsible, false);
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
		m_scroll_view->reorder_child(section, anchor);
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
	for (const auto &child : m_scroll_view->get_children()) {
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

} // namespace Editor
