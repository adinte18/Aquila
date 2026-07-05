#include "Aquila/UI/Widgets/ContextMenu.h"

namespace Aquila::UI::Core {

ContextMenu::ContextMenu() : FloatingOverlay(4) {
	FloatingConfig fc;
	fc.attach_to = FloatingAttachTo::Root;
	fc.element_point = FloatingAttachPoint::LeftTop;
	fc.parent_point = FloatingAttachPoint::LeftTop;
	fc.offset = { 0.F, 0.F };
	fc.z_index = 5;
	set_floating(fc);
}

void ContextMenu::add_item(std::string text, Delegate<void()> callback) {
	m_items.push_back({ std::move(text), std::move(callback) });
}

void ContextMenu::clear_items() {
	for (View *v : m_item_views) {
		remove_child(v);
	}
	m_item_views.clear();
	m_items.clear();
	invalidate_layout();
}

void ContextMenu::rebuild() {
	for (View *v : m_item_views) {
		remove_child(v);
	}
	m_item_views.clear();

	for (auto &item : m_items) {
		auto btn = create_unique<Button>();
		btn->set_text(item.text);
		btn->add_class("context-item");
		btn->on_click.connect([this, cb = item.callback] {
			cb();
			close();
		});
		m_item_views.push_back(add_child(std::move(btn)));
	}

	invalidate_layout();
}

void ContextMenu::open_at(Vec2 pos) {
	FloatingConfig fc = get_floating();
	fc.offset = pos;
	set_floating(fc);

	rebuild();
	open();
}

} // namespace Aquila::UI::Core
