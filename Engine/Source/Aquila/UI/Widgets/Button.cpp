#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Core/FontRegistry.h"

namespace Aquila::UI::Core {

Button::Button() {
	SetCapturesInput(true);
	// Label is created lazily via SetText() so that <Button><Image/>…</Button> works.
}

Button::Button(std::string text, Text::FontAtlas *font) {
	SetCapturesInput(true);
	auto label = CreateUnique<Label>(std::move(text), font);
	m_Label = dynamic_cast<Label *>(AddChild(std::move(label)));
}

void Button::SetText(std::string text) {
	if (m_Label == nullptr) {
		auto label = CreateUnique<Label>();
		m_Label = static_cast<Label *>(AddChild(std::move(label)));
	}
	m_Label->SetText(std::move(text));
}

void Button::SetFont(Text::FontAtlas *font) {
	if (m_Label == nullptr) {
		return;
	}
	m_Label->SetFont(font);
}

void Button::SetOnClick(std::function<void()> callback) {
	m_OnClick = std::move(callback);
}

void Button::OnMouseEnter() {
	View::OnMouseEnter();
}

void Button::OnMouseLeave() {
	View::OnMouseLeave();
}

void Button::OnMousePress(Platform::MouseButton btn, vec2 pos) {
	View::OnMousePress(btn, pos);
}

void Button::OnMouseRelease(Platform::MouseButton btn, vec2 pos) {
	if (btn == Platform::MouseButton::Left && m_IsHovered && m_OnClick) {
		m_OnClick();
	}
	View::OnMouseRelease(btn, pos);
}

void Button::OnStyleResolved() {
	if (m_Label == nullptr) {
		return;
	}

	if (m_Label->GetFont() == nullptr) {
		const std::string &family = GetComputedStyle().fontFamily;
		if (!family.empty()) {
			if (Text::FontAtlas *atlas = FontRegistry::Resolve(family)) {
				m_Label->SetFont(atlas);
			}
		}
	}
}

} // namespace Aquila::UI::Core
