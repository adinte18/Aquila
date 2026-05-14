#include "Aquila/UI/Core/View.h"

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
		return t * t * (3.f - 2.f * t); // smoothstep
	case UI::TransitionEasing::Linear:
	default:
		return t;
	}
}

void View::SetComputedStyle(UI::ComputedStyle style) {
	if (!m_DisplayStyleInitialized) {
		// First frame — snap display style directly, no animation.
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
	m_IsDirty = false;
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

	// Copy everything from computed first so layout/display/overflow/etc are always current.
	m_DisplayStyle = m_ComputedStyle;

	// Then overwrite animatable paint properties with lerped values.
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

View *View::AddChild(Unique<View> child) {
	View *raw = child.get();
	raw->m_Parent = this;
	PropagateCallbacks(raw);
	m_Children.push_back(std::move(child));
	if (raw->m_OnDirty) {
		raw->m_OnDirty(raw);
	}
	return raw;
}

void View::PropagateCallbacks(View *node) {
	node->m_OnDirty = m_OnDirty;
	node->m_OnAnimationStarted = m_OnAnimationStarted;
	for (const auto &child : node->GetChildren()) {
		PropagateCallbacks(child.get());
	}
}
void View::RemoveChild(View *child) {
	auto iterator = std::ranges::find_if(m_Children, [child](const Unique<View> &view) { return view.get() == child; });
	if (iterator != m_Children.end()) {
		m_Children.erase(iterator); // destructor recursively destroys entire subtree
	}
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
		return; // already hovered, nothing changed
	}
	m_IsHovered = true;
	m_IsDirty = true;
	if (m_OnDirty) {
		m_OnDirty(this);
	}
}
void View::OnMouseLeave() {
	if (!m_IsHovered) {
		return; // already not hovered
	}
	m_IsHovered = false;
	m_IsPressed = false;
	m_IsDirty = true;
	if (m_OnDirty) {
		m_OnDirty(this);
	}
}
void View::OnMousePress(Platform::MouseButton btn, vec2) {
	if (btn == Platform::MouseButton::Left) {
		m_IsPressed = true;
		m_IsDirty = true;
		if (m_OnDirty) {
			m_OnDirty(this);
		}
	}
}
void View::OnMouseRelease(Platform::MouseButton btn, vec2) {
	if (btn == Platform::MouseButton::Left) {
		m_IsPressed = false;
		m_IsDirty = true;
		if (m_OnDirty) {
			m_OnDirty(this);
		}
	}
}
void View::OnFocusGained() {
	m_IsFocused = true;
	m_IsDirty = true;
	if (m_OnDirty) {
		m_OnDirty(this);
	}
}
void View::OnFocusLost() {
	m_IsFocused = false;
	m_IsDirty = true;
	if (m_OnDirty) {
		m_OnDirty(this);
	}
}

View *View::HitTest(vec2 screenPos) {
	if (!m_Visible) {
		return nullptr;
	}
	if (!m_LayoutRect.Contains(screenPos)) {
		return nullptr;
	}

	// Interactive views consume the hit; children cannot steal input from them.
	if (!m_CapturesInput) {
		// m_LayoutRect.position is parent-relative, so translate into our local space before recursing.
		vec2 localPos = screenPos - m_LayoutRect.position;
		for (int i = static_cast<int>(m_Children.size()) - 1; i >= 0; i--) {
			if (View *hit = m_Children[i]->HitTest(localPos)) {
				return hit;
			}
		}
	}

	return this;
}

void View::OnDrawSelf(Rendering::DrawList &drawList) {
	if (!m_Visible || m_ComputedStyle.display == Display::None) {
		return;
	}

	const Rect worldRect = { .position = m_AbsolutePosition, .size = m_LayoutRect.size };
	const int32 z = m_DisplayStyle.zIndex * 4;

	for (const auto &shadow : m_DisplayStyle.boxShadows) {
		drawList.DrawShadow(worldRect, shadow.offset, shadow.blur, shadow.spread, shadow.color,
							m_DisplayStyle.borderRadius, z + 0);
	}

	if (m_DisplayStyle.backgroundColor.a > 0.f || m_DisplayStyle.borderWidth > 0.f) {
		drawList.DrawRect(worldRect, m_DisplayStyle.backgroundColor, m_DisplayStyle.borderRadius,
						  m_DisplayStyle.borderWidth, m_DisplayStyle.borderColor, z + 1);
	}
}

void View::OnDraw(Rendering::DrawList &drawList, vec2 origin) {
	if (!m_Visible || m_ComputedStyle.display == Display::None) {
		return;
	}

	const Rect worldRect = { .position = m_LayoutRect.position + origin, .size = m_LayoutRect.size };
	const int32 z = m_DisplayStyle.zIndex * 4;

	for (const auto &shadow : m_DisplayStyle.boxShadows) {
		drawList.DrawShadow(worldRect, shadow.offset, shadow.blur, shadow.spread, shadow.color,
							m_DisplayStyle.borderRadius, z + 0);
	}

	if (m_DisplayStyle.backgroundColor.a > 0.f || m_DisplayStyle.borderWidth > 0.f) {
		drawList.DrawRect(worldRect, m_DisplayStyle.backgroundColor, m_DisplayStyle.borderRadius,
						  m_DisplayStyle.borderWidth, m_DisplayStyle.borderColor, z + 1);
	}

	for (const auto &child : m_Children) {
		child->OnDraw(drawList, origin + m_LayoutRect.position);
	}
}
} // namespace Aquila::UI::Core
