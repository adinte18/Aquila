#pragma once

#include "Aquila/Platform/Input.h"
#include "Aquila/UI/Rendering/DrawList.h"
#include "Aquila/UI/Style/ComputedStyle.h"
#include "Aquila/UI/Style/StyleProperties.h"

namespace Aquila::UI::Core {

class View {
  public:
	View() = default;
	AQUILA_NONCOPYABLE(View);
	AQUILA_NONMOVEABLE(View);

	View *AddChild(Unique<View> child); // sets child->m_Parent, returns raw ptr
	void RemoveChild(View *child);
	[[nodiscard]] View *GetParent() const;
	[[nodiscard]] const std::vector<Unique<View>> &GetChildren() const;
	View *FindById(std::string_view id);
	View *HitTest(vec2 screenPos);

	[[nodiscard]] virtual std::string_view GetTypeName() const { return "View"; }
	[[nodiscard]] const std::string &GetId() const { return m_Id; }
	[[nodiscard]] const std::vector<std::string> &GetClasses() const { return m_Classes; }
	[[nodiscard]] const StyleProperties &GetStyle() const { return m_Style; }
	[[nodiscard]] const ComputedStyle &GetComputedStyle() const { return m_ComputedStyle; }
	[[nodiscard]] const Rect &GetLayoutRect() const { return m_LayoutRect; }

	void AddClass(std::string cls) { m_Classes.push_back(std::move(cls)); }

	void SetId(std::string id) { m_Id = std::move(id); }
	void SetStyle(StyleProperties props) { m_Style = props; }
	void SetComputedStyle(ComputedStyle style) { m_ComputedStyle = style; }
	void SetLayoutRect(Rect rect) { m_LayoutRect = rect; }
	void SetClayId(uint32 id) { m_ClayId = id; }
	[[nodiscard]] uint32 GetClayId() const { return m_ClayId; }

	virtual void OnDraw(Rendering::DrawList &drawList, vec2 originOffset);
	virtual void OnMouseEnter() {}
	virtual void OnMouseLeave() {}
	virtual void OnMousePress(Platform::MouseButton btn, vec2 pos) {}
	virtual void OnMouseRelease(Platform::MouseButton btn, vec2 pos) {}
	virtual void OnKeyPress(Platform::KeyCode key) {}
	virtual void OnKeyRelease(Platform::KeyCode key) {}
	virtual void OnFocusGained() {}
	virtual void OnFocusLost() {}
	virtual ~View() = default;

  private:
	View *m_Parent = nullptr;
	std::vector<Unique<View>> m_Children;

	std::string m_Id;
	std::vector<std::string> m_Classes;
	StyleProperties m_Style;
	ComputedStyle m_ComputedStyle;
	Rect m_LayoutRect; // clay based
	bool m_Visible = true;
	bool m_Enabled = true;
	uint32 m_ClayId = 0;
};
} // namespace Aquila::UI::Core
