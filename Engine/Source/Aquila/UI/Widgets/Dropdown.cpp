#include "Aquila/UI/Widgets/Dropdown.h"

namespace Aquila::UI::Core {

Dropdown::Dropdown() {
	StyleProperties sp;
	sp.flexDirection = FlexDirection::Column;
	SetStyle(sp);

	auto header = CreateUnique<Button>();
	{
		StyleProperties hp;
		hp.width = StyleLength::Grow();
		header->SetStyle(hp);
		header->AddClass("dropdown-header");
	}
	header->SetOnClick([this] { m_Popup->Toggle(); });
	m_Header = static_cast<Button *>(AddChild(std::move(header)));

	auto popup = CreateUnique<Popup>();
	{
		StyleProperties pp;
		pp.flexDirection = FlexDirection::Column;
		popup->MergeStyle(pp);
		popup->AddClass("dropdown-popup");
	}
	m_Popup = static_cast<Popup *>(AddChild(std::move(popup)));

	UpdateHeaderText();
}

void Dropdown::AddOption(std::string value, std::string display) {
	m_Options.push_back({ std::move(value), std::move(display) });
	Rebuild();
}

void Dropdown::ClearOptions() {
	m_Options.clear();
	m_Value.clear();
	Rebuild();
	UpdateHeaderText();
}

void Dropdown::SetValue(const std::string &value) {
	for (const auto &opt : m_Options) {
		if (opt.value == value) {
			m_Value = value;
			UpdateHeaderText();
			return;
		}
	}
}

void Dropdown::SetPlaceholder(std::string text) {
	m_Placeholder = std::move(text);
	UpdateHeaderText();
}

void Dropdown::SetOnChanged(Delegate<void(const std::string &)> callback) {
	m_OnChanged = std::move(callback);
}

void Dropdown::Rebuild() {
	for (View *v : m_OptionButtons) {
		m_Popup->RemoveChild(v);
	}
	m_OptionButtons.clear();

	for (const auto &opt : m_Options) {
		auto btn = CreateUnique<Button>();
		btn->SetText(opt.Label());
		btn->AddClass("dropdown-option");
		{
			StyleProperties bp;
			bp.width = StyleLength::Grow();
			btn->SetStyle(bp);
		}
		btn->SetOnClick([this, value = opt.value] {
			Select(value);
			m_Popup->Close();
		});
		m_OptionButtons.push_back(m_Popup->AddChild(std::move(btn)));
	}
}

void Dropdown::Select(const std::string &value) {
	m_Value = value;
	UpdateHeaderText();
	if (m_OnChanged) {
		m_OnChanged(m_Value);
	}
}

void Dropdown::UpdateHeaderText() {
	if (!m_Value.empty()) {
		for (const auto &opt : m_Options) {
			if (opt.value == m_Value) {
				m_Header->SetText(opt.Label());
				return;
			}
		}
	}
	m_Header->SetText(m_Placeholder);
}

} // namespace Aquila::UI::Core
