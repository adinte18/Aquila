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
	void SetFont(Text::FontAtlas *font);
	void SetOnClick(Delegate<void()> callback);

	void OnMouseEnter() override;
	void OnMouseLeave() override;
	void OnMousePress(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;
	void OnStyleResolved() override;

  private:
	void RefreshLabelSize();

	Label *m_Label = nullptr; // owned by m_Children
	Delegate<void()> m_OnClick;
	float m_LastResolvedFontSize = -1.f;
};

} // namespace Aquila::UI::Core
