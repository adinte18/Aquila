#pragma once

#include "Aquila/UI/Widgets/Control.h"
#include "Aquila/UI/Widgets/IconLabel.h"
#include "Aquila/UI/Style/StyleProperties.h"

namespace Aquila::UI::Core {

class Button : public Control {
  public:
	Button();
	Button(std::string text, Text::FontAtlas *font = nullptr);

	[[nodiscard]] std::string_view GetTypeName() const override { return "Button"; }

	void SetText(std::string text);
	void SetFont(Text::FontAtlas *font) override;
	void SetIcon(GFX::GfxTexture *texture);
	Signal<void()> onClick;

	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;
	void OnStyleResolved() override;
	void ApplyXmlTextContent(std::string_view text) override { SetText(std::string(text)); }
	void ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx = nullptr) override;

  private:
	void EnsureContent();

	IconLabel *m_Content = nullptr;
};

} // namespace Aquila::UI::Core
