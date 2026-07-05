#pragma once

#include "Aquila/Foundation/Signal.h"
#include "Aquila/Platform/Input.h"
#include "Aquila/UI/Rendering/DrawList.h"
#include "Aquila/UI/Style/ComputedStyle.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "Aquila/UI/Core/DragState.h"

namespace Aquila::UI::Text {
class FontAtlas;
}

namespace Aquila::UI::Core {

class Canvas;

class View {
  public:
	View();
	AQUILA_NONCOPYABLE(View);
	AQUILA_NONMOVEABLE(View);

	virtual View *add_child(Unique<View> child);

	template <typename T, typename... Args> T *add_child(Args &&...args) {
		return static_cast<T *>(add_child(create_unique<T>(std::forward<Args>(args)...)));
	}

	void remove_child(View *child);
	Unique<View> detach_child(View *child);
	View *replace_child(View *old, Unique<View> new_child);
	[[nodiscard]] View *get_parent() const;
	[[nodiscard]] View *get_first_draggable_parent() const;
	[[nodiscard]] View *get_first_parent_that_accepts_drop() const;
	[[nodiscard]] const std::vector<Unique<View>> &get_children() const;
	View *find_by_id(std::string_view id);

	template <typename T> T *find_by_id(std::string_view id) {
		View *v = find_by_id(id);
		return v ? dynamic_cast<T *>(v) : nullptr;
	}

	void set_canvas(Canvas *canvas);
	[[nodiscard]] Canvas *get_canvas() const { return m_canvas; }

	[[nodiscard]] virtual std::string_view get_type_name() const { return "View"; }
	[[nodiscard]] const std::string &get_id() const { return m_id; }
	[[nodiscard]] const std::vector<std::string> &get_classes() const { return m_classes; }
	[[nodiscard]] const StyleProperties &get_style() const { return m_style; }
	[[nodiscard]] const ComputedStyle &get_computed_style() const { return m_computed_style; }
	[[nodiscard]] const ComputedStyle &get_display_style() const { return m_display_style; }
	[[nodiscard]] const Rect &get_layout_rect() const { return m_layout_rect; }
	[[nodiscard]] Vec2 get_absolute_position() const { return m_absolute_position; }

	void add_class(std::string cls);
	void remove_class(std::string_view cls);

	void set_class(std::string cls, bool enabled);
	void set_hidden(bool hidden);

	void set_id(std::string id) { m_id = std::move(id); }
	void set_style(StyleProperties props) { m_style = std::move(props); }

	void merge_style(const StyleProperties &overlay);
	void set_computed_style(ComputedStyle style);
	void set_layout_rect(Rect rect) {
		if (m_layout_rect == rect) {
			return;
		}
		m_layout_rect = rect;
		mark_subtree_bounds_dirty();
	}
	void set_absolute_position(Vec2 pos) {
		if (m_absolute_position == pos) {
			return;
		}
		m_absolute_position = pos;
		mark_subtree_bounds_dirty();
	}
	void set_clay_id(Uint32 id) { m_clay_id = id; }

	void set_input_leaf(bool v) { m_is_input_leaf = v; }
	[[nodiscard]] bool is_input_leaf() const { return m_is_input_leaf; }
	[[nodiscard]] bool is_visible() const { return m_visible; }

	void set_enabled(bool enabled);
	[[nodiscard]] bool is_enabled() const { return m_enabled; }

	[[nodiscard]] Text::FontAtlas *get_resolved_font() const { return m_resolved_font; }

	void request_focus();
	void set_pass_through_scroll(bool v) { m_pass_through_scroll = v; }
	[[nodiscard]] bool get_pass_through_scroll() const { return m_pass_through_scroll; }

	[[nodiscard]] Rect get_absolute_rect() const { return { m_absolute_position, m_layout_rect.size }; }

	void mark_subtree_bounds_dirty() {
		m_subtree_bounds_dirty = true;
		if (m_parent != nullptr) {
			m_parent->mark_subtree_bounds_dirty();
		}
	}
	const Rect &get_subtree_bounds();

	Signal<void(Vec2)> on_context_menu;
	bool is_animation_finished() const { return m_is_animation_finished; }

	[[nodiscard]] bool is_hovered() const { return m_is_hovered; }
	[[nodiscard]] bool is_pressed() const { return m_is_pressed; }
	[[nodiscard]] bool is_focused() const { return m_is_focused; }
	[[nodiscard]] bool is_skipping_hit_test() const { return m_should_skip_hit_test; }
	[[nodiscard]] bool is_accepting_payload() const { return m_is_accepting_payload; }
	[[nodiscard]] bool is_draggable() const { return m_is_draggable; }
	[[nodiscard]] Uint32 get_clay_id() const { return m_clay_id; }

	[[nodiscard]] Uint32 get_stable_id() const { return m_stable_id; }

	virtual Vec2 get_intrinsic_size() const { return { -1.F, -1.F }; }

	virtual void on_draw_self(Rendering::DrawList &draw_list);
	virtual void on_mouse_enter();
	virtual void on_mouse_leave();
	virtual void on_mouse_press(Platform::MouseButton btn, Vec2 pos);
	virtual void on_mouse_release(Platform::MouseButton btn, Vec2 pos);
	virtual void on_key_press(Platform::KeyCode key, int mods = 0) {}
	virtual void on_key_release(Platform::KeyCode key) {}
	virtual void on_mouse_move(Vec2 pos) {}
	virtual void on_char_input(Uint32 codepoint) {}
	virtual void on_focus_gained();
	virtual void on_focus_lost();
	virtual void on_drag_start(DragState &d_state);
	virtual void on_drop(DragState &d_state);
	virtual void on_drag_enter(DragState &d_state);
	virtual void on_drag_leave(DragState &d_state);

	virtual void on_style_resolved();

	virtual void on_update(F32 delta_time) { (void)delta_time; }

	virtual void apply_xml_attribute(std::string_view name, std::string_view value, void *loader_ctx = nullptr);

	virtual void apply_xml_text_content(std::string_view text) { (void)text; }

	virtual void on_xml_loaded() {}

	virtual void set_font(Text::FontAtlas * /*font*/) {}

	void queue_redraw();

	void set_floating(FloatingConfig cfg) { m_floating = cfg; }
	void clear_floating() { m_floating.reset(); }
	bool has_floating() const { return m_floating.has_value(); }
	const FloatingConfig &get_floating() const { return *m_floating; }

	virtual View *hit_test_absolute(Vec2 canvas_pos);

	void set_draw_dirty() { m_draw_dirty = true; }
	void clear_draw_dirty() { m_draw_dirty = false; }
	[[nodiscard]] bool is_draw_dirty() const { return m_draw_dirty; }

	void set_effective_z(Int32 z) { m_effective_z = z; }
	[[nodiscard]] Int32 get_effective_z() const { return m_effective_z; }

	void invalidate_layout();

	void update_animation(float delta_time);
	virtual ~View() = default;

  protected:
	bool m_is_hovered = false;
	bool m_is_pressed = false;
	bool m_is_focused = false;
	bool m_should_skip_hit_test = false;
	bool m_is_accepting_payload = false;
	bool m_is_draggable = false;

  private:
	void notify_removed(View *node);

	View *m_parent = nullptr;
	std::vector<Unique<View>> m_children;

	ComputedStyle m_display_style;
	ComputedStyle m_animation_from;
	float m_transition_timer = 0.F;
	bool m_display_style_initialized = false;

	std::string m_id;
	std::vector<std::string> m_classes;
	StyleProperties m_style;
	ComputedStyle m_computed_style;
	Rect m_layout_rect;
	Vec2 m_absolute_position;
	bool m_visible = true;
	bool m_enabled = true;
	bool m_is_input_leaf = false;
	bool m_pass_through_scroll = false;
	Uint32 m_clay_id = 0;

	Option<FloatingConfig> m_floating;

	bool m_is_animation_finished = false;
	bool m_draw_dirty = true;
	Int32 m_effective_z = 0;
	Rect m_subtree_bounds{};
	bool m_subtree_bounds_dirty = true;
	Text::FontAtlas *m_resolved_font = nullptr;
	Canvas *m_canvas = nullptr;
	Uint32 m_stable_id = 0;
};
} // namespace Aquila::UI::Core
