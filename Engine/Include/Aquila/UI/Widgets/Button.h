#pragma once

#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Style/StyleProperties.h"

namespace Aquila::UI::Core {

class Button : public View {
  public:
	Button();
	Button(std::string text, Text::FontAtlas *font = nullptr);

	[[nodiscard]] std::string_view GetTypeName() const override { return "Button"; }

	void SetText(std::string text);
	void SetFont(Text::FontAtlas *font) override;
	void SetOnClick(Delegate<void()> callback);

	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;
	void OnStyleResolved() override;
	void ApplyXmlTextContent(std::string_view text) override { SetText(std::string(text)); }

  private:
	Label *m_Label = nullptr; // owned by m_Children
	Delegate<void()> m_OnClick;
};

} // namespace Aquila::UI::Core
