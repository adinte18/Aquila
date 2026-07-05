#include "Aquila/UI/Widgets/Collapsible.h"

namespace Aquila::UI::Core {

Collapsible::Collapsible(std::string title) {
	auto header = CreateUnique<Button>();
	header->SetText(std::move(title));
	header->AddClass("collapsible-header");
	header->onClick.Connect([this] {
		SetExpanded(!m_Expanded);
		onToggled(m_Expanded);
	});
	m_Header = static_cast<Button *>(AddChild(std::move(header)));

	auto content = CreateUnique<View>();
	content->AddClass("collapsible-content");
	m_Content = AddChild(std::move(content));
	ApplyState(); // apply initial collapsed/expanded display state
}

void Collapsible::SetTitle(std::string title) {
	m_Header->SetText(std::move(title));
}

void Collapsible::ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx) {
	if (name == "src" || name == "icon" || name == "bank" || name == "uv" || name == "tint") {
		m_Header->ApplyXmlAttribute(name, value, loaderCtx);
		return;
	}
	View::ApplyXmlAttribute(name, value, loaderCtx);
}

void Collapsible::SetExpanded(bool expanded) {
	if (expanded == m_Expanded) {
		return;
	}
	m_Expanded = expanded;
	ApplyState();
}


View *Collapsible::AddContent(Unique<View> child) {
	return m_Content->AddChild(std::move(child));
}

void Collapsible::ApplyState() {
	m_Content->SetHidden(!m_Expanded);
}

} // namespace Aquila::UI::Core
