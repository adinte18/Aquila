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
		m_sections.push_back({ section, std::move(component_ui) });
	};

	add_ui_component("section-transform", create_unique<TransformComponentUI>());
	add_ui_component("section-material", create_unique<MaterialComponentUI>(m_context));
	add_ui_component("section-light", create_unique<LightComponentUI>(m_context));
	add_ui_component("section-camera", create_unique<CameraComponentUI>());
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
}

} // namespace Editor
