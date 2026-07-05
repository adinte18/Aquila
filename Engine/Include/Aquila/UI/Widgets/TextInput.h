#pragma once

#include "Aquila/UI/Widgets/Control.h"
#include "Aquila/UI/Core/TextInputState.h"
#include "Aquila/UI/Text/FontAtlas.h"
#include <string>

namespace Aquila::UI::Core {

class TextInput : public Control {
  public:
	TextInput();
	explicit TextInput(std::string placeholder);

	[[nodiscard]] std::string_view GetTypeName() const override { return "TextInput"; }

	void SetText(const std::string &text);
	void SetFont(Text::FontAtlas *font) override;
	void SetPlaceholder(std::string text);
	Signal<void(const std::string &)> onChanged;
	Signal<void(const std::string &)> onSubmit;

	[[nodiscard]] const std::string &GetText() const { return m_State.text; }
	[[nodiscard]] vec2 GetIntrinsicSize() const override;

	void OnMousePress(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseMove(vec2 pos) override;
	void OnKeyPress(Platform::KeyCode key, int mods = 0) override;
	void OnCharInput(uint32 codepoint) override;
	void OnFocusGained() override;
	void OnFocusLost() override;
	void OnUpdate(f32 deltaTime) override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  protected:
	[[nodiscard]] Text::FontAtlas *ResolveFont() const;
	// Adjusts m_ScrollOffsetX so the cursor stays within the visible text area.
	void ClampScrollOffset(Text::FontAtlas *font, float scale, float visibleWidth);
	void ResetBlink();

	TextInputState m_State;
	std::string m_Placeholder;
	Text::FontAtlas *m_Font = nullptr;
	float m_ScrollOffsetX = 0.f; // horizontal scroll offset in pixels
	float m_BlinkTimer = 0.f;
	bool m_CaretVisible = true;

};

} // namespace Aquila::UI::Core
