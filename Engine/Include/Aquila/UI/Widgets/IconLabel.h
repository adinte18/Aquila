#pragma once

#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/Image.h"

namespace Aquila::UI::Core {

class IconLabel : public View {
  public:
	IconLabel();
	explicit IconLabel(std::string text, Text::FontAtlas *font = nullptr);

	[[nodiscard]] std::string_view GetTypeName() const override { return "IconLabel"; }

	void SetText(std::string text);
	void SetFont(Text::FontAtlas *font) override;

	void SetIconTexture(GFX::GfxTexture *texture);
	void SetIconTint(vec4 tint);

	[[nodiscard]] Label *GetLabel() const { return m_Label; }
	[[nodiscard]] Image *GetIcon() const { return m_Icon; }

	void OnStyleResolved() override;
	void ApplyXmlTextContent(std::string_view text) override { SetText(std::string(text)); }
	void ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx = nullptr) override;

  private:
	void UpdateIconVisibility();

	Image *m_Icon = nullptr;
	Label *m_Label = nullptr;
};

} // namespace Aquila::UI::Core
