#include "Aquila/UI/Widgets/Menubar.h"

namespace Aquila::UI::Core {

MenuDropdown::MenuDropdown(MenuBar *owner) : FloatingOverlay(10), m_owner(owner) {
	add_class("menu-dropdown");
	set_dismiss_on_click_away(true);

	FloatingConfig fc;
	fc.attach_to = FloatingAttachTo::Root;
	fc.element_point = FloatingAttachPoint::LeftTop;
	fc.parent_point = FloatingAttachPoint::LeftTop;
	fc.z_index = 50;
	set_floating(fc);
}

void MenuDropdown::add_item(std::string text, Delegate<void()> callback) {
	m_items.push_back({ std::move(text), std::move(callback), false });
}

void MenuDropdown::add_separator() {
	m_items.push_back({ {}, {}, true });
}

void MenuDropdown::clear_items() {
	for (View *v : m_item_views) {
		remove_child(v);
	}
	m_item_views.clear();
	m_items.clear();
}

void MenuDropdown::open_below(Vec2 button_abs_pos, float button_height) {
	FloatingConfig fc = get_floating();
	fc.offset = { button_abs_pos.x, button_abs_pos.y + button_height };
	set_floating(fc);

	rebuild();
	open();
}

void MenuDropdown::rebuild() {
	for (View *v : m_item_views) {
		remove_child(v);
	}
	m_item_views.clear();

	for (auto &item : m_items) {
		if (item.is_separator) {
			auto sep = std::make_unique<Separator>();
			sep->add_class("menu-separator");
			m_item_views.push_back(add_child(std::move(sep)));
			continue;
		}

		auto btn = std::make_unique<Button>();
		btn->set_text(item.text);
		btn->add_class("menu-item");
		btn->on_click.connect([this, cb = item.callback] {
			if (cb) {
				cb();
			}
			if (m_owner) {
				m_owner->close_all();
			}
		});
		m_item_views.push_back(add_child(std::move(btn)));
	}

	invalidate_layout();
}

MenuBar::MenuBar() {
	add_class("menu-bar");
}

MenuDropdown *MenuBar::add_menu(std::string title) {
	auto btn = std::make_unique<Button>();
	btn->set_text(title);
	btn->add_class("menu-bar-item");

	auto dropdown = std::make_unique<MenuDropdown>(this);
	MenuDropdown *dropdown_ptr = dropdown.get();

	btn->on_click.connect([this, dropdown_ptr] {
		if (m_open_dropdown == dropdown_ptr) {
			close_all();
			return;
		}
		for (auto &entry : m_entries) {
			if (entry.dropdown == dropdown_ptr) {
				Rect rect = entry.button->get_absolute_rect();
				open_dropdown(dropdown_ptr, rect.position, rect.size.y);
				return;
			}
		}
	});

	Button *btn_ptr = static_cast<Button *>(add_child(std::move(btn)));
	View *dropdown_parent = m_overlay_root ? m_overlay_root : static_cast<View *>(this);
	dropdown_parent->add_child(std::move(dropdown));

	m_entries.push_back({ btn_ptr, dropdown_ptr });
	return dropdown_ptr;
}

void MenuBar::set_overlay_root(View *root) {
	m_overlay_root = root;
}

void MenuBar::close_all() {
	if (m_open_dropdown) {
		m_open_dropdown->close();
		m_open_dropdown = nullptr;
	}
}

void MenuBar::open_dropdown(MenuDropdown *dropdown, Vec2 button_abs_pos, float button_height) {
	if (m_open_dropdown && m_open_dropdown != dropdown) {
		m_open_dropdown->close();
	}
	m_open_dropdown = dropdown;
	dropdown->open_below(button_abs_pos, button_height);
}

} // namespace Aquila::UI::Core
