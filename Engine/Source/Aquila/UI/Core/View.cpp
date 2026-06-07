#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Core/FontRegistry.h"
#include "Aquila/UI/Rendering/DrawCmd.h"
#include "Aquila/UI/Style/StyleParserHelper.h"

namespace Aquila::UI::Core {

static float ApplyEasing(float t, UI::TransitionEasing easing) {
	switch (easing) {
	case UI::TransitionEasing::EaseIn:
		return t * t;
	case UI::TransitionEasing::EaseOut:
		return 1.f - (1.f - t) * (1.f - t);
	case UI::TransitionEasing::EaseInOut:
		return t < 0.5f ? 2.f * t * t : 1.f - (-2.f * t + 2.f) * (-2.f * t + 2.f) * 0.5f;
	case UI::TransitionEasing::Ease:
		return t * t * (3.f - 2.f * t);
	case UI::TransitionEasing::Linear:
	default:
		return t;
	}
}

void View::SetComputedStyle(UI::ComputedStyle style) {
	if (!m_DisplayStyleInitialized) {
		m_DisplayStyle = style;
		m_AnimationFrom = style;
		m_DisplayStyleInitialized = true;
	} else if (m_ComputedStyle != style) {
		m_AnimationFrom = m_DisplayStyle;
		m_TransitionTimer = 0.f;
		m_IsAnimationFinished = false;
		if (m_OnAnimationStarted) {
			m_OnAnimationStarted(this);
		}
	}
	m_ComputedStyle = std::move(style);
}

void View::UpdateAnimation(float dt) {
	const float durationSec = m_ComputedStyle.transitionDuration / 1000.f;

	if (m_IsAnimationFinished) {
		return;
	}

	if (durationSec <= 0.0001f) {
		m_DisplayStyle = m_ComputedStyle;
		m_TransitionTimer = 0.f;
		m_IsAnimationFinished = true;
		return;
	}

	m_TransitionTimer = std::min(m_TransitionTimer + dt, durationSec);
	const float raw = m_TransitionTimer / durationSec;
	const float alpha = ApplyEasing(raw, m_ComputedStyle.transitionEasing);

	m_DisplayStyle = m_ComputedStyle;

	m_DisplayStyle.backgroundColor = glm::mix(m_AnimationFrom.backgroundColor, m_ComputedStyle.backgroundColor, alpha);
	m_DisplayStyle.borderColor = glm::mix(m_AnimationFrom.borderColor, m_ComputedStyle.borderColor, alpha);
	m_DisplayStyle.borderRadius = glm::mix(m_AnimationFrom.borderRadius, m_ComputedStyle.borderRadius, alpha);
	m_DisplayStyle.borderWidth = glm::mix(m_AnimationFrom.borderWidth, m_ComputedStyle.borderWidth, alpha);
	m_DisplayStyle.opacity = glm::mix(m_AnimationFrom.opacity, m_ComputedStyle.opacity, alpha);
	m_DisplayStyle.color = glm::mix(m_AnimationFrom.color, m_ComputedStyle.color, alpha);

	if (m_TransitionTimer >= durationSec) {
		m_IsAnimationFinished = true;
	}
}

void View::ApplyXmlAttribute(std::string_view name, std::string_view value, void * /*loaderCtx*/) {
	StyleProperties props;
	UI::ParserHelper::ApplyDeclaration(props, name, value);
	MergeStyle(props);
}

void View::OnStyleResolved() {
	const std::string &family = GetComputedStyle().fontFamily;
	m_ResolvedFont = family.empty() ? nullptr : FontRegistry::Resolve(family);
}

const Rect &View::GetSubtreeBounds() {
	if (!m_SubtreeBoundsDirty) {
		return m_SubtreeBounds;
	}
	m_SubtreeBounds = GetAbsoluteRect();
	for (const auto &child : m_Children) {
		if (child->GetDisplayStyle().display != Display::None && child->IsVisible()) {
			m_SubtreeBounds = m_SubtreeBounds.Union(child->GetSubtreeBounds());
		}
	}
	m_SubtreeBoundsDirty = false;
	return m_SubtreeBounds;
}

void View::SetEnabled(bool enabled) {
	if (m_Enabled == enabled) {
		return;
	}
	m_Enabled = enabled;
	if (m_OnDirty) {
		m_OnDirty(this);
	}
}

void View::RequestFocus() {
	if (m_OnFocusRequest) {
		m_OnFocusRequest(this);
	}
}

void View::MergeStyle(const StyleProperties &o) {
	StyleProperties &s = m_Style;
#define MERGE(f) \
	if (o.f)     \
	s.f = o.f
	MERGE(backgroundColor);
	MERGE(borderColor);
	MERGE(borderWidth);
	MERGE(borderRadius);
	MERGE(opacity);
	MERGE(overflow);
	MERGE(display);
	MERGE(width);
	MERGE(height);
	MERGE(min);
	MERGE(max);
	MERGE(minWidth);
	MERGE(maxWidth);
	MERGE(minHeight);
	MERGE(maxHeight);
	MERGE(padding);
	MERGE(paddingLeft);
	MERGE(paddingRight);
	MERGE(paddingTop);
	MERGE(paddingBottom);
	MERGE(gap);
	MERGE(flexDirection);
	MERGE(justifyContent);
	MERGE(alignItems);
	MERGE(flexWrap);
	MERGE(flexGrow);
	MERGE(position);
	MERGE(top);
	MERGE(right);
	MERGE(bottom);
	MERGE(left);
	MERGE(zIndex);
	MERGE(color);
	MERGE(accentColor);
	MERGE(fontSize);
	MERGE(fontFamily);
	MERGE(textAlign);
	MERGE(boxShadows);
	MERGE(transitionDuration);
	MERGE(transitionEasing);
#undef MERGE
	if (m_OnDirty) {
		m_OnDirty(this);
	}
}

View *View::AddChild(Unique<View> child) {
	View *raw = child.get();
	raw->m_Parent = this;
	PropagateCallbacks(raw);
	m_Children.push_back(std::move(child));
	MarkSubtreeBoundsDirty();
	if (raw->m_OnDirty) {
		raw->m_OnDirty(raw);
	}
	raw->SetDrawDirty();
	if (raw->m_OnDrawDirty) {
		raw->m_OnDrawDirty(raw);
	}
	return raw;
}

void View::QueueRedraw() {
	if (m_OnDrawDirty) {
		m_OnDrawDirty(this);
	}
}

void View::InvalidateLayout() {
	if (m_OnLayoutDirty) {
		m_OnLayoutDirty(this);
	}
}

void View::PropagateCallbacks(View *node) {
	node->m_OnDirty = m_OnDirty;
	node->m_OnAnimationStarted = m_OnAnimationStarted;
	node->m_OnDrawDirty = m_OnDrawDirty;
	node->m_OnLayoutDirty = m_OnLayoutDirty;
	node->m_OnFocusRequest = m_OnFocusRequest;
	node->m_OnRemoved = m_OnRemoved;
	for (const auto &child : node->GetChildren()) {
		PropagateCallbacks(child.get());
	}
}

void View::NotifyRemoved(View *node) {
	for (const auto &child : node->m_Children) {
		NotifyRemoved(child.get());
	}
	if (node->m_OnRemoved) {
		node->m_OnRemoved(node);
	}
}

void View::RemoveChild(View *child) {
	auto iterator = std::ranges::find_if(m_Children, [child](const Unique<View> &view) { return view.get() == child; });
	if (iterator != m_Children.end()) {
		NotifyRemoved(child);
		m_Children.erase(iterator);
		MarkSubtreeBoundsDirty();
	}
}

Unique<View> View::DetachChild(View *child) {
	auto it = std::ranges::find_if(m_Children, [child](const Unique<View> &view) { return view.get() == child; });
	if (it == m_Children.end()) {
		return nullptr;
	}
	NotifyRemoved(child);
	child->m_Parent = nullptr;
	auto owned = std::move(*it);
	m_Children.erase(it);
	MarkSubtreeBoundsDirty();
	return owned;
}

View *View::ReplaceChild(View *old, Unique<View> newChild) {
	auto it = std::ranges::find_if(m_Children, [old](const Unique<View> &v) { return v.get() == old; });
	if (it == m_Children.end()) {
		return nullptr;
	}
	NotifyRemoved(old);
	*it = std::move(newChild);
	View *raw = it->get();
	raw->m_Parent = this;
	PropagateCallbacks(raw);
	if (raw->m_OnDirty) {
		raw->m_OnDirty(raw);
	}
	raw->SetDrawDirty();
	if (raw->m_OnDrawDirty) {
		raw->m_OnDrawDirty(raw);
	}
	return raw;
}

View *View::GetParent() const {
	return m_Parent;
}

const std::vector<Unique<View>> &View::GetChildren() const {
	return m_Children;
}

View *View::FindById(std::string_view id) {
	if (m_Id == id) {
		return this;
	}

	for (const auto &child : m_Children) {
		if (View *found = child->FindById(id)) {
			return found;
		}
	}

	return nullptr;
}

void View::OnMouseEnter() {
	if (m_IsHovered) {
		return;
	}
	m_IsHovered = true;
	if (m_OnDirty) {
		m_OnDirty(this);
	}
}
void View::OnMouseLeave() {
	if (!m_IsHovered) {
		return;
	}
	m_IsHovered = false;
	if (m_OnDirty) {
		m_OnDirty(this);
	}
}
void View::OnMousePress(Platform::MouseButton btn, vec2 pos) {
	if (btn == Platform::MouseButton::Left) {
		m_IsPressed = true;
		if (m_OnDirty) {
			m_OnDirty(this);
		}
	}
	if (btn == Platform::MouseButton::Right && m_OnContextMenu) {
		m_OnContextMenu(pos);
	}
}
void View::OnMouseRelease(Platform::MouseButton btn, vec2) {
	if (btn == Platform::MouseButton::Left) {
		m_IsPressed = false;
		if (m_OnDirty) {
			m_OnDirty(this);
		}
	}
}
void View::OnFocusGained() {
	m_IsFocused = true;
	if (m_OnDirty) {
		m_OnDirty(this);
	}
}
void View::OnFocusLost() {
	m_IsFocused = false;
	if (m_OnDirty) {
		m_OnDirty(this);
	}
}

View *View::HitTestAbsolute(vec2 canvasPos) {
	if (m_ComputedStyle.display == Display::None || !m_Visible) {
		return nullptr;
	}

	if (m_IsInputLeaf) {
		const Rect r = GetAbsoluteRect();
		return r.Contains(canvasPos) ? this : nullptr;
	}

	for (int i = static_cast<int>(m_Children.size()) - 1; i >= 0; --i) {
		if (View *hit = m_Children[i]->HitTestAbsolute(canvasPos)) {
			return hit;
		}
	}

	const Rect r = GetAbsoluteRect();
	return r.Contains(canvasPos) ? this : nullptr;
}

void View::OnDrawSelf(Rendering::DrawList &drawList) {
	if (!m_Visible || m_ComputedStyle.display == Display::None) {
		return;
	}

	const Rect worldRect = { .position = m_AbsolutePosition, .size = m_LayoutRect.size };
	const int32 z = 0;

	for (const auto &shadow : m_DisplayStyle.boxShadows) {
		drawList.DrawShadow(worldRect, shadow.offset, shadow.blur, shadow.spread, shadow.color,
							m_DisplayStyle.borderRadius, z + 0);
	}

	if (m_DisplayStyle.backgroundColor.a > 0.f || m_DisplayStyle.borderWidth > 0.f) {
		drawList.DrawRect(worldRect, m_DisplayStyle.backgroundColor, m_DisplayStyle.borderRadius,
						  m_DisplayStyle.borderWidth, m_DisplayStyle.borderColor, z + 1);
	}
}

} // namespace Aquila::UI::Core
