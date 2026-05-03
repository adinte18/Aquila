#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Text/FontAtlas.h"

#include <string>

namespace Aquila::UI::Core {

class Label : public View {
  public:
	Label() = default;
	explicit Label(std::string text, Text::FontAtlas *font = nullptr);

	[[nodiscard]] std::string_view GetTypeName() const override { return "Label"; }

	void SetText(std::string text) { m_Text = std::move(text); }
	void SetFont(Text::FontAtlas *font) { m_Font = font; }

	[[nodiscard]] vec2 Measure() const;

	[[nodiscard]] const std::string &GetText() const { return m_Text; }
	[[nodiscard]] Text::FontAtlas *GetFont() const { return m_Font; }

	void OnDraw(Rendering::DrawList &drawList, vec2 origin) override;

  private:
	std::string m_Text;
	Text::FontAtlas *m_Font = nullptr;
};

} // namespace Aquila::UI::Core
