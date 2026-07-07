#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/Rendering/FrameScheduler.h"

namespace Aquila::UI::Core {

using namespace Aquila::UI::Rendering;

Canvas::Canvas(Uint32 width, Uint32 height)
	: m_width(width), m_height(height), m_layout_engine(width, height), m_input_router(*this, m_draw_compositor) {
	m_root = std::make_unique<View>();
	m_root->set_canvas(this);

	m_draw_compositor.set_canvas_size(width, height);

	StyleProperties root_style;
	root_style.width = StyleLength::grow();
	root_style.height = StyleLength::grow();
	m_root->set_style(root_style);
	notify_style_dirty(m_root.get());
	style_pass();
}

void Canvas::mark_dirty() {
	m_dirty = true;
	Aquila::Rendering::FrameScheduler::get()->request_frame();
}

void Canvas::request_layout() {
	m_layout_dirty = true;
	mark_dirty();
}

void Canvas::notify_style_dirty(View *view) {
	m_style_engine.invalidate(view);
	mark_dirty();
}

void Canvas::notify_animation_started(View *view) {
	for (const View *v : m_active_anims) {
		if (v == view) {
			return;
		}
	}
	m_active_anims.push_back(view);
	mark_dirty();
}

void Canvas::notify_draw_dirty(View *view) {
	mark_node_draw_dirty(view);
}

void Canvas::notify_layout_dirty(View *view) {
	m_layout_dirty = true;
	mark_node_draw_dirty(view);
}

void Canvas::notify_focus_request(View *view) {
	m_input_router.set_focus(view);
}

void Canvas::notify_view_removed(View *view) {
	m_style_engine.remove(view);
	m_input_router.on_view_removed(view);
	unregister_popup(view);
	unregister_tick(view);
	if (m_scroll_target == view) {
		m_scroll_target = nullptr;
	}
	if (auto it = std::ranges::find(m_active_anims, view); it != m_active_anims.end()) {
		m_active_anims.erase(it);
	}
}

void Canvas::register_popup(View *popup, Delegate<void()> on_dismiss) {
	unregister_popup(popup);
	m_open_popups.push_back({ popup, std::move(on_dismiss) });
}

void Canvas::unregister_popup(View *popup) {
	std::erase_if(m_open_popups, [popup](const OpenPopup &p) { return p.root == popup; });
}

void Canvas::register_tick(View *view) {
	if (std::ranges::find(m_ticking, view) == m_ticking.end()) {
		m_ticking.push_back(view);
	}
}

void Canvas::unregister_tick(View *view) {
	std::erase(m_ticking, view);
}

static bool is_within(View *node, View *root) {
	for (View *v = node; v; v = v->get_parent()) {
		if (v == root) {
			return true;
		}
	}
	return false;
}

void Canvas::dismiss_popups_outside(View *hit) {
	if (m_open_popups.empty()) {
		return;
	}
	std::vector<Delegate<void()>> to_dismiss;
	for (const auto &popup : m_open_popups) {
		if (!is_within(hit, popup.root)) {
			to_dismiss.push_back(popup.on_dismiss);
		}
	}
	for (auto &dismiss : to_dismiss) {
		if (dismiss) {
			dismiss();
		}
	}
}

void Canvas::reload_styles() {
	mark_subtree_dirty(m_root.get());
}

void Canvas::mark_subtree_dirty(View *node) {
	notify_style_dirty(node);
	for (const auto &child : node->get_children()) {
		mark_subtree_dirty(child.get());
	}
}

void Canvas::style_pass() {
	const StyleEngine::ResolveResult result = m_style_engine.resolve(m_width, m_height, m_layout_dirty);
	m_layout_dirty = result.layout_affected;
	if (result.changed) {
		mark_dirty();
	}
}

void Canvas::animation_pass(F32 dt) {
	auto it = m_active_anims.begin();
	while (it != m_active_anims.end()) {
		View *v = *it;
		v->update_animation(dt);
		mark_node_draw_dirty(v);
		if (v->is_animation_finished()) {
			it = m_active_anims.erase(it);
		} else {
			++it;
		}
	}
}

void Canvas::compute() {
	if (!m_root || !m_dirty) {
		return;
	}

	if (m_layout_dirty) {
		m_layout_engine.run_layout(m_root.get(), m_input_router.mouse_pos(), m_input_router.mouse_down(),
								   m_input_router.take_scroll_delta(), m_delta_time);
		m_layout_dirty = false;

		// @container rules depend on element sizes — re-resolve immediately after
		// layout so rules see the current frame's container sizes.
		if (m_style_engine.get_style_sheet().has_container_blocks()) {
			mark_subtree_dirty(m_root.get());
			style_pass();
		}

		m_draw_compositor.invalidate_all(m_root.get());
	}

	if (m_scroll_target) {
		m_layout_engine.scroll_into_view(m_scroll_target);
		m_scroll_target = nullptr;
		m_layout_engine.run_layout(m_root.get(), m_input_router.mouse_pos(), m_input_router.mouse_down(), {},
								   m_delta_time);
		m_draw_compositor.invalidate_all(m_root.get());
	}

	if (m_draw_compositor.rebuild_dirty(m_root.get())) {
		m_draw_list_dirty = true;
	}
	m_dirty = false;
}

void Canvas::mark_node_draw_dirty(View *node) {
	node->set_draw_dirty();
	mark_dirty();
}

void Canvas::submit_to_quad_batcher(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd) {
	if (!m_root) {
		return;
	}
	m_draw_compositor.submit(r2d, cmd);
}

void Canvas::on_event(Application::Events::Event &e) {
	m_input_router.on_event(e);
}

View *Canvas::hit_test(Vec2 pos) {
	return m_draw_compositor.hit_test(pos);
}

void Canvas::scroll_into_view(View *target) {
	m_scroll_target = target;
	m_layout_dirty = true;
	mark_dirty();
}

void Canvas::update(F32 delta_time) {
	m_delta_time = delta_time;
	if (!m_ticking.empty()) {
		for (View *view : m_ticking) {
			view->on_update(delta_time);
		}
		Aquila::Rendering::FrameScheduler::get()->request_frame();
	}
	style_pass();
	animation_pass(delta_time);
}

void Canvas::resize(Uint32 width, Uint32 height) {
	m_width = width;
	m_height = height;
	m_draw_compositor.set_canvas_size(width, height);
	m_layout_dirty = true;
	mark_dirty();
	// @media rules depend on viewport size — re-resolve all styles on resize.
	if (m_style_engine.get_style_sheet().has_media_blocks()) {
		mark_subtree_dirty(m_root.get());
	}
	m_layout_engine.set_dimensions(width, height);
}

StyleSheet &Canvas::get_style_sheet() {
	return m_style_engine.get_style_sheet();
}

View *Canvas::get_root() {
	return m_root.get();
}

} // namespace Aquila::UI::Core
