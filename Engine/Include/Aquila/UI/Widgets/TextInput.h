#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Text/FontAtlas.h"
#include <string>

namespace Aquila::UI::Core {

class TextInput : public View {
  public:
	TextInput();
	explicit TextInput(std::string placeholder);

	[[nodiscard]] std::string_view GetTypeName() const override { return "TextInput"; }

	void SetText(const std::string &text);
	void SetFont(Text::FontAtlas *font);
	void SetPlaceholder(std::string text);
	void SetOnChanged(Delegate<void(const std::string &)> callback);
	void SetOnSubmit(Delegate<void(const std::string &)> callback);

	[[nodiscard]] const std::string &GetText() const { return m_Text; }

	void OnMousePress(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseMove(vec2 pos) override;
	void OnKeyPress(Platform::KeyCode key, int mods = 0) override;
	void OnCharInput(uint32 codepoint) override;
	void OnFocusGained() override;
	void OnFocusLost() override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  private:
	[[nodiscard]] Text::FontAtlas *ResolveFont() const;
	[[nodiscard]] float MeasureTextX(Text::FontAtlas *font, float scale, size_t pos) const;
	[[nodiscard]] size_t HitTestPos(float localX) const;
	[[nodiscard]] bool HasSelection() const { return m_SelectAnchor != m_Cursor; }
	[[nodiscard]] size_t SelectionMin() const { return std::min(m_SelectAnchor, m_Cursor); }
	[[nodiscard]] size_t SelectionMax() const { return std::max(m_SelectAnchor, m_Cursor); }
	void DeleteSelection();
	void SelectAll();

	std::string m_Text;
	std::string m_Placeholder;
	size_t m_Cursor = 0;
	size_t m_SelectAnchor = 0; // selection anchor — cursor moves, anchor stays
	Text::FontAtlas *m_Font = nullptr;

	Delegate<void(const std::string &)> m_OnChanged;
	Delegate<void(const std::string &)> m_OnSubmit;
};

} // namespace Aquila::UI::Core
