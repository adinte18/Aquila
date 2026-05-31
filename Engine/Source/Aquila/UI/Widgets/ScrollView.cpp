#include "Aquila/UI/Widgets/ScrollView.h"

namespace Aquila::UI::Core {

ScrollView::ScrollView() {
	StyleProperties sp;
	sp.overflow = Overflow::Scroll;
	sp.flexDirection = FlexDirection::Column;
	sp.width = StyleLength::Grow();
	sp.height = StyleLength::Grow();
	SetStyle(sp);

	auto inner = CreateUnique<View>();
	StyleProperties ip;
	ip.flexDirection = FlexDirection::Column;
	ip.width = StyleLength::Grow();
	inner->SetStyle(ip);
	m_Inner = AddChild(std::move(inner));
}

View *ScrollView::AddContent(Unique<View> child) {
	return m_Inner->AddChild(std::move(child));
}

} // namespace Aquila::UI::Core
