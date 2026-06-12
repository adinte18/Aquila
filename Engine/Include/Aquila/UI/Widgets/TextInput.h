#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Core/TextInputState.h"
#include "Aquila/UI/Text/FontAtlas.h"
#include <string>

namespace Aquila::UI::Core {

class TextInput : public View {
  public:
	TextInput();
	explicit TextInput(std::string placeholder);

	[[nodiscard]] std::string_view GetTypeName() const override { return "TextInput"; }

	void SetText(const std::string &text);
	void SetFont(Text::FontAtlas *font) override;
	void SetPlaceholder(std::string text);
	void SetOnChanged(Delegate<void(const std::string &)> callback);
	void SetOnSubmit(Delegate<void(const std::string &)> callback);

	[[nodiscard]] const std::string &GetText() const { return m_State.text; }
	[[nodiscard]] vec2 GetIntrinsicSize() const override;

	void OnMousePress(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseMove(vec2 pos) override;
	void OnKeyPress(Platform::KeyCode key, int mods = 0) override;
	void OnCharInput(uint32 codepoint) override;
	void OnFocusGained() override;
	void OnFocusLost() override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  protected:
	[[nodiscard]] Text::FontAtlas *ResolveFont() const;
	// Adjusts m_ScrollOffsetX so the cursor stays within the visible text area.
	void ClampScrollOffset(Text::FontAtlas *font, float scale, float visibleWidth);

	TextInputState m_State;
	std::string m_Placeholder;
	Text::FontAtlas *m_Font = nullptr;
	float m_ScrollOffsetX = 0.f; // horizontal scroll offset in pixels

	Delegate<void(const std::string &)> m_OnChanged;
	Delegate<void(const std::string &)> m_OnSubmit;
};

} // namespace Aquila::UI::Core
