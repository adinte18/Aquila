#include "Aquila/UI/Widgets/ScrollView.h"

namespace Aquila::UI::Core {

ScrollView::ScrollView() {
	add_class("scroll-view");

	auto inner = std::make_unique<View>();
	inner->add_class("scroll-inner");
	m_inner = add_child(std::move(inner));
}

View *ScrollView::add_content(Unique<View> child) {
	return m_inner->add_child(std::move(child));
}

void ScrollView::remove_oldest_content() {
	const auto &children = m_inner->get_children();
	if (!children.empty()) {
		m_inner->remove_child(children.front().get());
	}
}

} // namespace Aquila::UI::Core
