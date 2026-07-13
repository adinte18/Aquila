#pragma once

#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/UI/Core/DrawCompositor.h"
#include "Aquila/UI/Core/InputRouter.h"
#include "Aquila/UI/Core/LayoutEngine.h"
#include "Aquila/UI/Core/StyleEngine.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StyleSheet.h"

namespace Aquila::UI::Core {

using namespace Aquila::UI::Rendering;

class Tooltip;

class Canvas {
	friend class InputRouter; // calls MarkDirty()/RequestLayout() on input

  public:
	Canvas(Uint32 width, Uint32 height);

	void on_event(Application::Events::Event &event);
	void update(float delta_time);
	void compute();
	void submit_to_quad_batcher(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd);
	void resize(Uint32 width, Uint32 height);

	StyleSheet &get_style_sheet();
	View *get_root();
	View *hit_test(Vec2 pos);
	void scroll_into_view(View *target);
	Uint32 get_width() const { return m_width; }
	Uint32 get_height() const { return m_height; }
	void notify_style_dirty(View *view);
	void notify_animation_started(View *view);
	void notify_draw_dirty(View *view);
	void notify_layout_dirty(View *view);
	void notify_focus_request(View *view);
	void notify_view_removed(View *view);
	void reload_styles();
	void mark_subtree_dirty(View *node);

	void register_popup(View *popup, Delegate<void()> on_dismiss);
	void unregister_popup(View *popup);

	void register_tick(View *view);
	void unregister_tick(View *view);

	bool is_draw_list_dirty() const { return m_draw_list_dirty; }
	void clear_draw_list_dirty() { m_draw_list_dirty = false; }

  private:
	void mark_node_draw_dirty(View *node);
	void dismiss_popups_outside(View *hit);

	struct OpenPopup {
		View *root;
		Delegate<void()> on_dismiss;
	};
	std::vector<OpenPopup> m_open_popups;
	std::vector<View *> m_ticking;
	View *m_scroll_target = nullptr;

	Unique<View> m_root;
	StyleEngine m_style_engine;
	LayoutEngine m_layout_engine;
	DrawCompositor m_draw_compositor;
	InputRouter m_input_router;
	Uint32 m_width, m_height;

	float m_delta_time = 0.F;

	std::vector<View *> m_active_anims;

	bool m_layout_dirty = true;
	bool m_dirty = true;
	bool m_draw_list_dirty = true; // true when draw list was rebuilt this frame

	void mark_dirty();
	void request_layout(); // mark layout dirty + request a frame (used by input/scroll)
	void style_pass();
	void animation_pass(F32 dt);
	void update_tooltip(F32 dt);

	Tooltip *m_tooltip = nullptr;
	View *m_tooltip_target = nullptr;
	float m_tooltip_timer = 0.F;
	bool m_tooltip_shown = false;
};
} // namespace Aquila::UI::Core
