#include "Aquila/UI/Core/View.h"


namespace Aquila::UI::Core {
View *View::AddChild(Unique<View> child) {
	View *raw = child.get();
	raw->m_Parent = this;
	m_Children.push_back(std::move(child));
	return raw;
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

View *View::HitTest(vec2 screenPos) {
	if (!m_Visible) {
		return nullptr;
	}
	if (!m_LayoutRect.Contains(screenPos)) {
		return nullptr;
	}

	for (int i = m_Children.size() - 1; i >= 0; i--) {
		if (View *hit = m_Children[i]->HitTest(screenPos)) {
			return hit;
		}
	}

	return this;
}

void View::OnDraw(Rendering::DrawList &drawList, vec2 origin) {
	if (!m_Visible) {
		return;
	}

	Rect worldRect{ .position = m_LayoutRect.position + origin, .size = m_LayoutRect.size };

	if (m_ComputedStyle.backgroundColor.a > 0.F || m_ComputedStyle.borderWidth > 0.F) {
		drawList.DrawRect(worldRect, m_ComputedStyle.backgroundColor, m_ComputedStyle.borderRadius,
						  m_ComputedStyle.borderWidth, m_ComputedStyle.borderColor);
	}

	for (const auto &child : m_Children) {
		child->OnDraw(drawList, origin + m_LayoutRect.position);
	}
}
} // namespace Aquila::UI::Core
