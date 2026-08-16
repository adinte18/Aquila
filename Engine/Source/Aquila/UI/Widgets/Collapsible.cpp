#include "Aquila/UI/Widgets/Collapsible.h"

#include "Aquila/Foundation/Math/Math.h"
#include "Aquila/Platform/Input.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Style/StyleTypes.h"

namespace Aquila::UI::Core {

namespace {

constexpr F32 K_FLIP_SPEED = 16.0F;
constexpr F32 K_FLIP_EPSILON = 0.5F;

class CollapsibleGrip : public View {
  public:
	explicit CollapsibleGrip(Collapsible *owner) : m_owner(owner) {
		m_is_draggable = true;
		set_input_leaf(true);
	}

	[[nodiscard]] std::string_view get_type_name() const override { return "CollapsibleGrip"; }

	void on_drag_start(DragState &state) override {
		state.payload = static_cast<View *>(m_owner);
		m_owner->begin_drag();
	}

	void on_draw_self(Rendering::DrawList &draw_list) override {
		const Rect rect = get_absolute_rect();
		const Vec4 color = get_display_style().color;
		const F32 radius = 1.5F;
		const F32 column_pitch = 5.0F;
		const F32 row_pitch = 5.0F;
		const Vec2 center = { rect.position.x + (rect.size.x * 0.5F), rect.position.y + (rect.size.y * 0.5F) };

		for (Int32 col = 0; col < 2; ++col) {
			for (Int32 row = 0; row < 3; ++row) {
				const F32 dot_x = center.x + ((static_cast<F32>(col) - 0.5F) * column_pitch);
				const F32 dot_y = center.y + ((static_cast<F32>(row) - 1.0F) * row_pitch);
				const Rect dot = { .position = { dot_x - radius, dot_y - radius },
								   .size = { radius * 2.0F, radius * 2.0F } };
				draw_list.draw_rect(dot, color, Vec4(radius), 0.0F, Vec4(0.0F), 2);
			}
		}
	}

  private:
	Collapsible *m_owner;
};

} // namespace

Collapsible::Collapsible(std::string title) {
	auto bar = std::make_unique<View>();
	bar->add_class("collapsible-header");
	m_header_bar = View::add_child(std::move(bar));

	auto button = std::make_unique<Button>();
	button->set_text(std::move(title));
	button->add_class("collapsible-title");
	button->on_click.connect([this] {
		set_expanded(!m_expanded);
		on_toggled(m_expanded);
	});
	m_title_button = dynamic_cast<Button *>(m_header_bar->add_child(std::move(button)));

	auto grip = std::make_unique<CollapsibleGrip>(this);
	grip->add_class("collapsible-grip");
	m_grip = m_header_bar->add_child(std::move(grip));

	auto content = std::make_unique<View>();
	content->add_class("collapsible-content");
	m_content = View::add_child(std::move(content));
	apply_state();
}

void Collapsible::set_title(std::string title) {
	m_title_button->set_text(std::move(title));
}

void Collapsible::apply_xml_attribute(std::string_view name, std::string_view value, IResourceResolver *resolver) {
	if (name == "src" || name == "uv" || name == "tint") {
		m_title_button->apply_xml_attribute(name, value, resolver);
		return;
	}
	View::apply_xml_attribute(name, value, resolver);
}

void Collapsible::set_expanded(bool expanded) {
	if (expanded == m_expanded) {
		return;
	}
	m_expanded = expanded;
	apply_state();
}

View *Collapsible::add_child(Unique<View> child) {
	return m_content->View::add_child(std::move(child));
}

void Collapsible::apply_state() {
	m_content->set_hidden(!m_expanded);
}

void Collapsible::begin_drag() {
	if (m_dragging) {
		return;
	}
	m_container = get_parent();
	if (m_container == nullptr) {
		return;
	}
	m_dragging = true;

	const Vec2 cursor = Platform::Input::get_mouse_position();
	m_grab_offset = cursor - get_absolute_position();

	auto placeholder = std::make_unique<View>();
	placeholder->add_class("collapsible-drop-slot");
	StyleProperties slot_style;
	slot_style.height = StyleLength::pixel(get_absolute_rect().size.y);
	placeholder->set_style(slot_style);
	m_placeholder = m_container->add_child(std::move(placeholder));
	m_container->reorder_child(m_placeholder, this);
	m_drop_anchor = this;

	StyleProperties drag_style;
	drag_style.width = StyleLength::pixel(get_absolute_rect().size.x);
	merge_style(drag_style);

	FloatingConfig floating;
	floating.attach_to = FloatingAttachTo::Parent;
	floating.element_point = FloatingAttachPoint::LeftTop;
	floating.parent_point = FloatingAttachPoint::LeftTop;
	floating.z_index = 100;
	floating.offset = get_absolute_position() - m_container->get_absolute_position();
	m_locked_x = floating.offset.x;
	set_floating(floating);

	seed_flip_homes();
	start_ticking();
	invalidate_layout();
}

void Collapsible::seed_flip_homes() {
	m_flip_home.clear();
	for (const auto &child : m_container->get_children()) {
		m_flip_home[child.get()] = child->get_layout_home();
	}
}

bool Collapsible::on_update(F32 delta_time) {
	if (m_dragging) {
		update_drag();
	}
	tick_flip(delta_time);
	return m_ticking;
}

void Collapsible::update_drag() {
	if (!Platform::Input::is_mouse_button_pressed(Platform::MouseButton::Left)) {
		finalize_drag();
		return;
	}

	const Vec2 cursor = Platform::Input::get_mouse_position();
	const F32 desired_top = cursor.y - m_grab_offset.y;

	const F32 container_height = m_container->get_absolute_rect().size.y;
	const F32 item_height = get_absolute_rect().size.y;
	const F32 max_offset_y = std::max(0.0F, container_height - item_height);
	const F32 offset_y = Math::clamp(desired_top - m_container->get_absolute_position().y, 0.0F, max_offset_y);

	FloatingConfig floating = get_floating();
	floating.offset = { m_locked_x, offset_y };
	set_floating(floating);

	update_drop_target();
	invalidate_layout();
}

void Collapsible::update_drop_target() {
	const F32 cursor_y = Platform::Input::get_mouse_position().y;
	View *anchor = nullptr;
	for (const auto &child : m_container->get_children()) {
		View *view = child.get();
		if (view == this || view == m_placeholder) {
			continue;
		}
		if (!view_is<Collapsible>(view)) {
			continue;
		}
		const Rect rect = view->get_absolute_rect();
		const F32 midpoint = rect.position.y + (rect.size.y * 0.5F);
		if (cursor_y < midpoint) {
			anchor = view;
			break;
		}
	}

	if (anchor == m_drop_anchor) {
		return;
	}
	m_drop_anchor = anchor;
	m_container->reorder_child(m_placeholder, anchor);
}

void Collapsible::finalize_drag() {
	m_dragging = false;

	if (m_container != nullptr && m_placeholder != nullptr) {
		m_flip_home[this] = get_layout_home();
		m_container->reorder_child(this, m_placeholder);
		m_container->remove_child(m_placeholder);
		m_flip_home.erase(m_placeholder);
	}
	m_placeholder = nullptr;
	m_drop_anchor = nullptr;

	StyleProperties restore_style;
	restore_style.width = StyleLength::percent(100.0F);
	merge_style(restore_style);

	clear_floating();
	invalidate_layout();
	on_reordered();
}

void Collapsible::tick_flip(F32 delta_time) {
	if (!m_ticking) {
		return;
	}

	const F32 blend = std::min(delta_time * K_FLIP_SPEED, 1.0F);
	bool any_active = false;

	for (const auto &child : m_container->get_children()) {
		View *view = child.get();
		if (view == m_placeholder) {
			continue;
		}
		if (m_dragging && view == this) {
			continue;
		}

		const Vec2 new_home = view->get_layout_home();
		auto entry = m_flip_home.find(view);
		if (entry == m_flip_home.end()) {
			m_flip_home[view] = new_home;
			continue;
		}

		Vec2 offset = view->get_layout_anim_offset();
		if (new_home != entry->second) {
			offset += entry->second - new_home;
			entry->second = new_home;
		}

		if (Math::length(offset) > K_FLIP_EPSILON) {
			offset -= offset * blend;
			if (Math::length(offset) <= K_FLIP_EPSILON) {
				offset = { 0.0F, 0.0F };
			}
			view->set_layout_anim_offset(offset);
			any_active = true;
		}
	}

	if (any_active) {
		invalidate_layout();
	} else if (!m_dragging) {
		stop_ticking();
	}
}

void Collapsible::start_ticking() {
	if (m_ticking) {
		return;
	}
	if (Canvas *canvas = get_canvas()) {
		canvas->register_tick(this);
		m_ticking = true;
	}
}

void Collapsible::stop_ticking() {
	if (!m_ticking) {
		return;
	}
	if (Canvas *canvas = get_canvas()) {
		canvas->unregister_tick(this);
	}
	m_ticking = false;
	m_flip_home.clear();
}

} // namespace Aquila::UI::Core
