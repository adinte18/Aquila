#include "Aquila/UI/Core/DrawCompositor.h"
#include "Aquila/Foundation/Math/Math.h"

namespace Aquila::UI::Core {

static void gather_floating_roots(View *node, std::vector<View *> &roots) {
	if (node->get_display_style().display == Display::None) {
		return;
	}
	if (node->has_floating()) {
		roots.push_back(node);
		return;
	}
	for (const auto &c : node->get_children()) {
		gather_floating_roots(c.get(), roots);
	}
}

static bool within_all_clip_ancestors(View *node, Vec2 pos) {
	View *p = node->get_parent();
	while (p != nullptr) {
		const Overflow overflow = p->get_display_style().overflow;
		if (overflow == Overflow::Scroll || overflow == Overflow::Hidden) {
			if (!p->get_absolute_rect().contains(pos)) {
				return false;
			}
		}
		p = p->get_parent();
	}
	return true;
}

void DrawCompositor::set_canvas_size(Uint32 width, Uint32 height) {
	m_width = width;
	m_height = height;
	m_draw_list.set_canvas_size(width, height);
}

void DrawCompositor::rebuild_lists(View *root) {
	for (auto &b : m_z_buckets) {
		b.clear();
	}
	m_canvas_items.clear();
	m_canvas_layers.clear();
	m_float_roots.clear();
	{
		const Rect canvas_bounds = { { 0.F, 0.F }, { static_cast<F32>(m_width), static_cast<F32>(m_height) } };
		cull(root, 0, &canvas_bounds);
	}
	gather_floating_roots(root, m_float_roots);
	std::ranges::stable_sort(m_float_roots.begin(), m_float_roots.end(),
							 [](View *a, View *b) { return a->get_floating().z_index < b->get_floating().z_index; });
	for (View *float_root : m_float_roots) {
		collect_layer(float_root);
	}
}

void DrawCompositor::invalidate_all(View *root) {
	m_per_node_cmds.clear();
	rebuild_lists(root);
	for (View *v : m_canvas_items) {
		v->set_draw_dirty();
	}
	for (View *v : m_canvas_layers) {
		v->set_draw_dirty();
	}
}

bool DrawCompositor::rebuild_dirty(View *root) {
	bool any_rebuilt = false;
	auto rebuild_node = [&](View *v) {
		if (v->is_draw_dirty()) {
			DrawList capture;
			capture.set_canvas_size(m_width, m_height);
			v->on_draw_self(capture);
			auto cmds = capture.take_commands();
			std::ranges::stable_sort(cmds.begin(), cmds.end(), [](const DrawCmd &a, const DrawCmd &b) {
				return draw_cmd_z_order(a) < draw_cmd_z_order(b);
			});
			m_per_node_cmds[v] = std::move(cmds);
			v->clear_draw_dirty();
			any_rebuilt = true;
		}
	};
	for (View *v : m_canvas_items) {
		rebuild_node(v);
	}
	for (View *v : m_canvas_layers) {
		rebuild_node(v);
	}

	if (any_rebuilt) {
		rebuild_lists(root);

		m_draw_list.clear();
		for (const auto &b : m_z_buckets) {
			for (const DrawCmd &cmd : b) {
				m_draw_list.append_cmd(cmd);
			}
		}
		for (View *float_root : m_float_roots) {
			emit_floating_layer(float_root, nullptr);
		}
	}

	return any_rebuilt;
}

void DrawCompositor::cull(View *node, Int32 parent_effective_z, const Rect *clip_rect) {
	if (node->get_display_style().display == Display::None) {
		return;
	}
	if (!node->is_visible()) {
		return;
	}
	if (node->has_floating()) {
		return;
	}

	if (clip_rect != nullptr && !clip_rect->overlaps(node->get_subtree_bounds())) {
		return;
	}

	const Int32 effective_z = parent_effective_z + node->get_computed_style().z_index;
	const Int32 bucket_idx =
		Math::clamp(effective_z, SharedConstants::Z_MIN, SharedConstants::Z_MAX) - SharedConstants::Z_MIN;

	if (clip_rect == nullptr || clip_rect->overlaps(node->get_absolute_rect())) {
		if (auto it = m_per_node_cmds.find(node); it != m_per_node_cmds.end()) {
			for (const DrawCmd &cmd : it->second) {
				m_z_buckets[bucket_idx].push_back(cmd);
			}
		}
		m_canvas_items.push_back(node);
	}

	const Rect *child_clip = clip_rect;
	Rect own_clip;
	bool clips_children = false;
	const Overflow overflow = node->get_display_style().overflow;
	if (overflow == Overflow::Scroll || overflow == Overflow::Hidden) {
		own_clip = node->get_absolute_rect();
		if (clip_rect != nullptr) {
			own_clip = clip_rect->intersect(own_clip);
			if (own_clip.is_empty()) {
				return;
			}
		}
		child_clip = &own_clip;
		clips_children = true;

		ClipPushCmd clip_push;
		clip_push.rect = own_clip;
		m_z_buckets[bucket_idx].push_back(clip_push);
	}

	for (const auto &child : node->get_children()) {
		cull(child.get(), effective_z, child_clip);
	}

	if (clips_children) {
		ClipPopCmd clip_pop;
		clip_pop.rect = clip_rect != nullptr ? *clip_rect : Rect{};
		m_z_buckets[bucket_idx].push_back(clip_pop);
	}
}

void DrawCompositor::collect_layer(View *node) {
	m_canvas_layers.push_back(node);
	for (const auto &child : node->get_children()) {
		collect_layer_subtree(child.get());
	}
}

void DrawCompositor::collect_layer_subtree(View *node) {
	if (node->get_display_style().display == Display::None) {
		return;
	}
	if (!node->is_visible()) {
		return;
	}
	m_canvas_layers.push_back(node);
	for (const auto &child : node->get_children()) {
		collect_layer_subtree(child.get());
	}
}

void DrawCompositor::emit_floating_layer(View *node, const Rect *clip_rect) {
	if (node->get_display_style().display == Display::None) {
		return;
	}
	if (!node->is_visible()) {
		return;
	}

	if (auto it = m_per_node_cmds.find(node); it != m_per_node_cmds.end()) {
		for (const DrawCmd &cmd : it->second) {
			m_draw_list.append_cmd(cmd);
		}
	}

	const Rect *child_clip = clip_rect;
	Rect own_clip;
	bool clips_children = false;
	const Overflow overflow = node->get_display_style().overflow;
	if (overflow == Overflow::Scroll || overflow == Overflow::Hidden) {
		own_clip = node->get_absolute_rect();
		if (clip_rect != nullptr) {
			own_clip = clip_rect->intersect(own_clip);
		}
		child_clip = &own_clip;
		clips_children = true;

		ClipPushCmd clip_push;
		clip_push.rect = own_clip;
		m_draw_list.append_cmd(clip_push);
	}

	for (const auto &child : node->get_children()) {
		emit_floating_layer(child.get(), child_clip);
	}

	if (clips_children) {
		ClipPopCmd clip_pop;
		clip_pop.rect = clip_rect != nullptr ? *clip_rect : Rect{};
		m_draw_list.append_cmd(clip_pop);
	}
}

View *DrawCompositor::hit_test(Vec2 pos) const {
	for (int i = static_cast<int>(m_canvas_layers.size()) - 1; i >= 0; --i) {
		View *v = m_canvas_layers[i];

		if (v->is_skipping_hit_test()) {
			continue;
		}

		if (!v->get_absolute_rect().contains(pos)) {
			continue;
		}

		View *p = v->get_parent();
		while (p) {
			if (p->is_input_leaf() && p->get_absolute_rect().contains(pos)) {
				v = p;
			}
			p = p->get_parent();
		}
		return v;
	}

	for (int i = static_cast<int>(m_canvas_items.size()) - 1; i >= 0; --i) {
		View *v = m_canvas_items[i];

		if (v->is_skipping_hit_test()) {
			continue;
		}

		if (!v->get_absolute_rect().contains(pos)) {
			continue;
		}
		if (!within_all_clip_ancestors(v, pos)) {
			continue;
		}
		View *p = v->get_parent();
		while (p) {
			if (p->is_input_leaf() && p->get_absolute_rect().contains(pos)) {
				v = p;
			}
			p = p->get_parent();
		}
		return v;
	}
	return nullptr;
}

void DrawCompositor::submit(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd) {
	if (m_draw_list.is_empty()) {
		return;
	}
	m_draw_list.submit(r2d, cmd);
}

} // namespace Aquila::UI::Core
