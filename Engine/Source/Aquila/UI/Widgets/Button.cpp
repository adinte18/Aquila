#include "Aquila/UI/Widgets/Button.h"

namespace Aquila::UI::Core {

Button::Button() {
	SetInputLeaf(true);
}

Button::Button(std::string text, Text::FontAtlas *font) {
	SetInputLeaf(true);
	auto label = CreateUnique<Label>(std::move(text), font);
	m_Label = dynamic_cast<Label *>(AddChild(std::move(label)));
}

void Button::SetText(std::string text) {
	if (m_Label == nullptr) {
		auto label = CreateUnique<Label>(text);
		m_Label = static_cast<Label *>(AddChild(std::move(label)));
		return;
	}
	m_Label->SetText(std::move(text));
}

void Button::SetFont(Text::FontAtlas *font) {
	if (m_Label == nullptr) {
		return;
	}
	m_Label->SetFont(font);
}

void Button::SetOnClick(Delegate<void()> callback) {
	m_OnClick = std::move(callback);
}

void Button::OnMouseRelease(Platform::MouseButton btn, vec2 pos) {
	if (btn == Platform::MouseButton::Left && m_IsHovered && m_OnClick) {
		m_OnClick();
	}
	View::OnMouseRelease(btn, pos);
}

void Button::OnStyleResolved() {
	View::OnStyleResolved();
	if (m_Label == nullptr) {
		return;
	}
	if (Text::FontAtlas *font = GetResolvedFont()) {
		m_Label->SetFont(font);
	}
}

} // namespace Aquila::UI::Core
