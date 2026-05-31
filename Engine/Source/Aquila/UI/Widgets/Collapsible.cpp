#include "Aquila/UI/Widgets/Collapsible.h"

namespace Aquila::UI::Core {

Collapsible::Collapsible(std::string title) {
	StyleProperties sp;
	sp.flexDirection = FlexDirection::Column;
	sp.width = StyleLength::Grow();
	SetStyle(sp);

	auto header = CreateUnique<Button>();
	header->SetText(std::move(title));
	header->AddClass("collapsible-header");
	{
		StyleProperties hp;
		hp.width = StyleLength::Grow();
		header->SetStyle(hp);
	}
	header->SetOnClick([this] {
		SetExpanded(!m_Expanded);
		if (m_OnToggled) {
			m_OnToggled(m_Expanded);
		}
	});
	m_Header = static_cast<Button *>(AddChild(std::move(header)));

	auto content = CreateUnique<View>();
	{
		StyleProperties cp;
		cp.flexDirection = FlexDirection::Column;
		cp.width = StyleLength::Grow();
		content->SetStyle(cp);
		content->AddClass("collapsible-content");
	}
	m_Content = AddChild(std::move(content));
	ApplyState(); // apply initial collapsed/expanded display state
}

void Collapsible::SetTitle(std::string title) {
	m_Header->SetText(std::move(title));
}

void Collapsible::SetExpanded(bool expanded) {
	if (expanded == m_Expanded) {
		return;
	}
	m_Expanded = expanded;
	ApplyState();
}

void Collapsible::SetOnToggled(Delegate<void(bool)> callback) {
	m_OnToggled = std::move(callback);
}

View *Collapsible::AddContent(Unique<View> child) {
	return m_Content->AddChild(std::move(child));
}

void Collapsible::ApplyState() {
	StyleProperties p;
	p.display = m_Expanded ? Display::Flex : Display::None;
	m_Content->MergeStyle(p);
}

} // namespace Aquila::UI::Core
