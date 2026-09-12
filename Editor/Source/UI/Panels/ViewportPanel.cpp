#include "UI/Panels/ViewportPanel.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/Image.h"

namespace Editor {

using namespace Aquila;

ViewportPanel::ViewportPanel(GFX::GfxTexture &initial_texture) : m_initial_texture(initial_texture) {}

void ViewportPanel::build(UI::Core::DockPanel *panel, UI::Core::View *) {
	m_image = panel->find_by_id<UI::Core::Image>("viewport");
	if (m_image == nullptr) {
		AQUILA_LOG_ERROR("ViewportPanel: 'viewport' image not found in layout");
		return;
	}

	m_image->set_texture(&m_initial_texture);
	m_image->set_pass_through_scroll(true);
	m_image->set_skip_hit_test(false);

	m_image->on_pressed.connect([this](Vec2 position) {
		const Rect rect = m_image->get_absolute_rect();
		if (rect.size.x <= 0.F || rect.size.y <= 0.F) {
			return;
		}
		const Vec2 local = position - rect.position;
		on_clicked_uv({ local.x / rect.size.x, local.y / rect.size.y });
	});
}

void ViewportPanel::set_texture(GFX::GfxTexture *texture) {
	if (m_image != nullptr) {
		m_image->set_texture(texture);
	}
}

Rect ViewportPanel::get_content_rect() const {
	return (m_image != nullptr) ? m_image->get_absolute_rect() : Rect{};
}

} // namespace Editor
