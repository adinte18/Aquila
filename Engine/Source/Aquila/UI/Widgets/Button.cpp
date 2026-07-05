#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/Foundation/Macros.h"

namespace Aquila::UI::Core {

Button::Button() {
	SetInputLeaf(true);
}

Button::Button(std::string text, Text::FontAtlas *font) {
	SetInputLeaf(true);
	m_Content = dynamic_cast<IconLabel *>(AddChild(CreateUnique<IconLabel>(std::move(text), font)));
}

void Button::EnsureContent() {
	if (m_Content == nullptr) {
		m_Content = static_cast<IconLabel *>(AddChild(CreateUnique<IconLabel>()));
	}
}

void Button::SetText(std::string text) {
	EnsureContent();
	m_Content->SetText(std::move(text));
}

void Button::SetFont(Text::FontAtlas *font) {
	if (m_Content == nullptr) {
		return;
	}
	m_Content->SetFont(font);
}

void Button::SetIcon(GFX::GfxTexture *texture) {
	EnsureContent();
	m_Content->SetIconTexture(texture);
}

void Button::OnMouseRelease(Platform::MouseButton btn, vec2 pos) {
	View::OnMouseRelease(btn, pos);
	if (btn == Platform::MouseButton::Left && m_IsHovered) {
		onClick();
	}
}

void Button::OnStyleResolved() {
	View::OnStyleResolved();
	if (m_Content == nullptr) {
		return;
	}
	if (Text::FontAtlas *font = GetResolvedFont()) {
		m_Content->SetFont(font);
	}
}

void Button::ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx) {
	if (name == "src" || name == "icon" || name == "bank" || name == "uv" || name == "tint") {
		EnsureContent();
		m_Content->ApplyXmlAttribute(name, value, loaderCtx);
		return;
	}
	if (name == "on-click") {
		if (auto *loader = static_cast<LayoutLoader *>(loaderCtx)) {
			if (Delegate<void()> command = loader->ResolveCommand(std::string(value))) {
				onClick.Connect(std::move(command));
			} else {
				AQUILA_LOG_WARNING("Button: on-click references unknown command '{}'", value);
			}
		}
		return;
	}
	View::ApplyXmlAttribute(name, value, loaderCtx);
}

} // namespace Aquila::UI::Core
