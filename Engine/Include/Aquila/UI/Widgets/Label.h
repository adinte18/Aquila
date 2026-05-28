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

	void SetText(std::string text);
	void SetFont(Text::FontAtlas *font);

	// Measures the text at the label's own computed fontSize.
	// Pass overrideFontSize > 0 to use a different size (e.g. from a parent that
	// hasn't yet propagated its style down to this label).
	[[nodiscard]] vec2 Measure(float overrideFontSize = 0.f) const;
	// Returns m_Font if set, otherwise resolves from FontRegistry via computed font-family.
	[[nodiscard]] Text::FontAtlas *ResolveFont() const;

	vec2 GetIntrinsicSize() const override;

	[[nodiscard]] const std::string &GetText() const { return m_Text; }
	[[nodiscard]] Text::FontAtlas *GetFont() const { return m_Font; }

	void OnDraw(Rendering::DrawList &drawList, vec2 origin) override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  private:
	std::string m_Text;
	Text::FontAtlas *m_Font = nullptr;
};

} // namespace Aquila::UI::Core
