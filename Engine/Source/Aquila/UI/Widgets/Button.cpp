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
	RefreshLabelSize();
}

void Button::SetText(std::string text) {
	if (m_Label == nullptr) {
		auto label = CreateUnique<Label>();
		m_Label = static_cast<Label *>(AddChild(std::move(label)));
	}
	m_Label->SetText(std::move(text));
	RefreshLabelSize();
}

void Button::SetFont(Text::FontAtlas *font) {
	if (m_Label == nullptr) {
		return;
	}
	m_Label->SetFont(font);
	RefreshLabelSize();
}

void Button::SetOnClick(std::function<void()> callback) {
	m_OnClick = std::move(callback);
}

void Button::OnMouseEnter() {
	View::OnMouseEnter(); // sets m_IsHovered = true
}

void Button::OnMouseLeave() {
	View::OnMouseLeave(); // sets m_IsHovered = false, m_IsPressed = false
}

void Button::OnMousePress(Platform::MouseButton btn, vec2 pos) {
	View::OnMousePress(btn, pos); // sets m_IsPressed = true
}

void Button::OnMouseRelease(Platform::MouseButton btn, vec2 pos) {
	if (btn == Platform::MouseButton::Left && m_IsHovered && m_OnClick) {
		m_OnClick();
	}
	View::OnMouseRelease(btn, pos); // sets m_IsPressed = false
}

void Button::OnStyleResolved() {
	if (m_Label == nullptr) {
		return;
	}

	bool changed = false;

	if (m_Label->GetFont() == nullptr) {
		const std::string &family = GetComputedStyle().fontFamily;
		if (!family.empty()) {
			if (Text::FontAtlas *atlas = FontRegistry::Resolve(family)) {
				m_Label->SetFont(atlas);
				changed = true;
			}
		}
	}

	const float fontSize = GetComputedStyle().fontSize;
	if (fontSize != m_LastResolvedFontSize) {
		m_LastResolvedFontSize = fontSize;
		changed = true;
	}

	if (changed) {
		RefreshLabelSize();
	}
}

void Button::RefreshLabelSize() {
	if (m_Label == nullptr) {
		return;
	}
	const float fontSize = GetComputedStyle().fontSize;
	const vec2 measured = m_Label->Measure(fontSize);
	if (measured.x <= 0.f && measured.y <= 0.f) {
		return;
	}

	const StyleProperties &current = m_Label->GetStyle();

	const bool widthSame =
		current.width.has_value() && current.width->unit == LengthUnit::Pixel && current.width->value == measured.x;
	const bool heightSame =
		current.height.has_value() && current.height->unit == LengthUnit::Pixel && current.height->value == measured.y;
	if (widthSame && heightSame) {
		return;
	}

	StyleProperties labelStyle = current;
	labelStyle.width = StyleLength::Pixel(measured.x);
	labelStyle.height = StyleLength::Pixel(measured.y);
	m_Label->SetStyle(labelStyle);
}
} // namespace Aquila::UI::Core
