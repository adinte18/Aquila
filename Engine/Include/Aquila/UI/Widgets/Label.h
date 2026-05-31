#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Text/FontAtlas.h"

namespace Aquila::UI::Core {

class Label : public View {
  public:
	Label() = default;
	explicit Label(std::string text, Text::FontAtlas *font = nullptr);

	[[nodiscard]] std::string_view GetTypeName() const override { return "Label"; }

	void SetText(std::string text);
	void SetFont(Text::FontAtlas *font) override;

	[[nodiscard]] vec2 Measure(float overrideFontSize = 0.f) const;

	[[nodiscard]] Text::FontAtlas *ResolveFont() const;

	vec2 GetIntrinsicSize() const override;

	[[nodiscard]] const std::string &GetText() const { return m_Text; }
	[[nodiscard]] Text::FontAtlas *GetFont() const { return m_Font; }

	void OnDrawSelf(Rendering::DrawList &drawList) override;
	void ApplyXmlTextContent(std::string_view text) override { SetText(std::string(text)); }

  private:
	std::string m_Text;
	Text::FontAtlas *m_Font = nullptr;
};

} // namespace Aquila::UI::Core
