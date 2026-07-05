#include "Aquila/UI/Widgets/ScrollView.h"

namespace Aquila::UI::Core {

ScrollView::ScrollView() {
	AddClass("scroll-view");

	auto inner = CreateUnique<View>();
	inner->AddClass("scroll-inner");
	m_Inner = AddChild(std::move(inner));
}

View *ScrollView::AddContent(Unique<View> child) {
	return m_Inner->AddChild(std::move(child));
}

void ScrollView::RemoveOldestContent() {
	const auto &children = m_Inner->GetChildren();
	if (!children.empty()) {
		m_Inner->RemoveChild(children.front().get());
	}
}

} // namespace Aquila::UI::Core
