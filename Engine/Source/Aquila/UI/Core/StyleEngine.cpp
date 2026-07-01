#include "Aquila/UI/Core/StyleEngine.h"
#include "Aquila/Foundation/Profiler.h"
#include "Aquila/UI/Style/StylePropertyList.h"
#include <unordered_set>

namespace Aquila::UI::Core {

static bool AffectsLayout(const ComputedStyle &a, const ComputedStyle &b) {
#define AQ_STYLE_PROP(css, sp, cs, layout, anim, inherit) \
	AQ_STYLE_WHEN(layout, if (a.cs != b.cs) { return true; })
	AQ_STYLE_PROPERTY_LIST
#undef AQ_STYLE_PROP
	return false;
}

void StyleEngine::Invalidate(View *view) {
	m_StyleCache.Invalidate(view);
	m_DirtyViews.MarkDirty(view);
}

void StyleEngine::Remove(View *view) {
	m_StyleCache.Remove(view);
	m_DirtyViews.Remove(view);
}

StyleEngine::ResolveResult StyleEngine::Resolve(uint32 viewportWidth, uint32 viewportHeight, bool layoutAlreadyDirty) {
	ResolveResult result;
	result.layoutAffected = layoutAlreadyDirty;
	if (m_DirtyViews.IsEmpty()) {
		return result;
	}
	PROFILE_SCOPE("StyleEngine::Resolve");

	auto depth = [](const View *view) {
		int result = 0;
		while (view->GetParent()) {
			view = view->GetParent();
			result++;
		}
		return result;
	};

	std::vector<View *> queue = m_DirtyViews.GetOrdered();
	std::ranges::stable_sort(queue.begin(), queue.end(), [&](View *a, View *b) { return depth(a) < depth(b); });
	std::unordered_set<View *> inQueue(queue.begin(), queue.end());

	for (size_t i = 0; i < queue.size(); i++) {
		View *node = queue[i];

		const ComputedStyle *parentStyle = nullptr;
		if (View *parent = node->GetParent()) {
			parentStyle = m_StyleCache.Peek(parent);
			if (!parentStyle) {
				parentStyle = &parent->GetComputedStyle();
			}
			m_StyleCache.RegisterDependency(node, parent);
		}

		StyleSheet::ResolveContext ctx;
		ctx.viewportSize = { static_cast<f32>(viewportWidth), static_cast<f32>(viewportHeight) };
		if (const View *parent = node->GetParent()) {
			ctx.containerSize = parent->GetLayoutRect().size;
		}

		const ComputedStyle old = node->GetComputedStyle();
		const ComputedStyle &resolved =
			m_StyleCache.Get(node, [&]() { return m_StyleSheet.Resolve(*node, parentStyle, ctx); });

		if (resolved != old) {
			result.changed = true;
			if (!result.layoutAffected && AffectsLayout(old, resolved)) {
				result.layoutAffected = true;
			}
			node->SetComputedStyle(resolved);
			node->OnStyleResolved();
			node->SetDrawDirty();

			for (const auto &child : node->GetChildren()) {
				View *c = child.get();
				if (inQueue.insert(c).second) {
					queue.push_back(c);
				}
			}
		} else {
			node->SetComputedStyle(resolved);
			node->OnStyleResolved();
		}
	}

	m_DirtyViews.Clear();
	return result;
}

} // namespace Aquila::UI::Core
