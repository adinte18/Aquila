#include "UI/Panels/StatusBar.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/Label.h"

namespace Editor {

using namespace Aquila;
using Aquila::SceneManagement::SceneStatistics;

namespace {

std::string with_separators(Uint64 value) {
	std::string digits = std::to_string(value);
	std::string result;
	result.reserve(digits.size() + (digits.size() / 3));
	for (Usize i = 0; i < digits.size(); ++i) {
		if (i > 0 && (digits.size() - i) % 3 == 0) {
			result.push_back(',');
		}
		result.push_back(digits[i]);
	}
	return result;
}

std::string selected_of_total(Uint64 selected, Uint64 total, bool has_selection) {
	if (!has_selection) {
		return with_separators(total);
	}
	return with_separators(selected) + " / " + with_separators(total);
}

} // namespace

void StatusBar::build(UI::Core::View *layout_root) {
	m_root = layout_root->find_by_id("hud-status");
	if (m_root == nullptr) {
		AQUILA_LOG_ERROR("StatusBar: 'hud-status' not found in layout");
		return;
	}

	m_selection.label = layout_root->find_by_id<UI::Core::Label>("status-selection");
	m_objects.label = layout_root->find_by_id<UI::Core::Label>("stat-objects");
	m_lights.label = layout_root->find_by_id<UI::Core::Label>("stat-lights");
	m_vertices.label = layout_root->find_by_id<UI::Core::Label>("stat-vertices");
	m_triangles.label = layout_root->find_by_id<UI::Core::Label>("stat-triangles");
	set_visible(m_visible);
}

void StatusBar::update(const SceneStatistics &statistics, const std::string &selected_name) {
	const bool has_selection = statistics.selected_objects > 0;
	show(m_selection, has_selection ? selected_name : std::string("No selection"));
	show(m_objects, selected_of_total(statistics.selected_objects, statistics.objects, has_selection));
	show(m_lights, with_separators(statistics.lights));
	show(m_vertices, selected_of_total(statistics.selected.vertices, statistics.total.vertices, has_selection));
	show(m_triangles, selected_of_total(statistics.selected.triangles, statistics.total.triangles, has_selection));
}

void StatusBar::set_visible(bool visible) {
	m_visible = visible;
	if (m_root != nullptr) {
		m_root->set_hidden(!visible);
	}
}

void StatusBar::show(Field &field, std::string text) {
	if (field.label == nullptr || field.text == text) {
		return;
	}
	field.text = text;
	field.label->set_text(std::move(text));
}

} // namespace Editor
