#include "Aquila/UI/Widgets/SearchBox.h"

namespace Aquila::UI::Core {

SearchBox::SearchBox() {
	AddClass("search-box");

	StyleProperties sp;
	sp.flexDirection = FlexDirection::Row;
	sp.alignItems = AlignItems::Center;
	sp.width = StyleLength::Grow();
	MergeStyle(sp);

	auto input = CreateUnique<TextInput>("Search...");
	input->AddClass("search-input");

	StyleProperties inputStyle;
	inputStyle.flexGrow = 1.f;
	input->MergeStyle(inputStyle);

	input->onChanged.Connect([this](const std::string &text) {
		UpdateClearButton();
		onChanged(text);
	});
	m_Input = static_cast<TextInput *>(AddChild(std::move(input)));

	auto clearBtn = CreateUnique<Button>("×");
	clearBtn->AddClass("search-clear");
	clearBtn->onClick.Connect([this] { Clear(); });
	m_ClearButton = static_cast<Button *>(AddChild(std::move(clearBtn)));

	UpdateClearButton();
}

void SearchBox::SetPlaceholder(std::string placeholder) {
	m_Input->SetPlaceholder(std::move(placeholder));
}


const std::string &SearchBox::GetText() const {
	return m_Input->GetText();
}

void SearchBox::Clear() {
	m_Input->SetText("");
	UpdateClearButton();
	onChanged("");
}

void SearchBox::UpdateClearButton() {
	StyleProperties sp;
	sp.display = m_Input->GetText().empty() ? Display::None : Display::Flex;
	m_ClearButton->MergeStyle(sp);
}

} // namespace Aquila::UI::Core
