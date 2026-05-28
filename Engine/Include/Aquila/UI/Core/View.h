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
	void PropagateCallbacks(View *node);

	[[nodiscard]] virtual std::string_view GetTypeName() const { return "View"; }
	[[nodiscard]] const std::string &GetId() const { return m_Id; }
	[[nodiscard]] const std::vector<std::string> &GetClasses() const { return m_Classes; }
	[[nodiscard]] const StyleProperties &GetStyle() const { return m_Style; }
	[[nodiscard]] const ComputedStyle &GetComputedStyle() const { return m_ComputedStyle; }
	[[nodiscard]] const ComputedStyle &GetDisplayStyle() const { return m_DisplayStyle; }
	[[nodiscard]] const Rect &GetLayoutRect() const { return m_LayoutRect; }
	[[nodiscard]] vec2 GetAbsolutePosition() const { return m_AbsolutePosition; }

	void AddClass(std::string cls) { m_Classes.push_back(std::move(cls)); }

	void SetId(std::string id) { m_Id = std::move(id); }
	void SetStyle(StyleProperties props) { m_Style = std::move(props); }
	// Overlay only the fields that are explicitly set in |overlay|, leaving everything else unchanged.
	void MergeStyle(const StyleProperties &overlay);
	void SetComputedStyle(ComputedStyle style); // detects changes and starts transitions
	void SetLayoutRect(Rect rect) { m_LayoutRect = rect; }
	void SetAbsolutePosition(vec2 pos) { m_AbsolutePosition = pos; }
	void SetClayId(uint32 id) { m_ClayId = id; }
	void SetCapturesInput(bool v) { m_CapturesInput = v; }
	void SetPassThroughScroll(bool v) { m_PassThroughScroll = v; }
	[[nodiscard]] bool GetPassThroughScroll() const { return m_PassThroughScroll; }
	void SetContextView(Delegate<void(vec2)> cb) { m_OnContextMenu = std::move(cb); }
	bool IsAnimationFinished() const { return m_IsAnimationFinished; }

	[[nodiscard]] bool IsHovered() const { return m_IsHovered; }
	[[nodiscard]] bool IsPressed() const { return m_IsPressed; }
	[[nodiscard]] bool IsFocused() const { return m_IsFocused; }
	[[nodiscard]] uint32 GetClayId() const { return m_ClayId; }
	[[nodiscard]] bool IsStyleDirty() const { return m_IsDirty; }

	// Returns the preferred content size for leaf nodes (e.g. measured text for Label).
	// Return {-1,-1} to let Clay size via the normal CSS rules.
	virtual vec2 GetIntrinsicSize() const { return { -1.f, -1.f }; }

	virtual void OnDraw(Rendering::DrawList &drawList, vec2 originOffset);
	virtual void OnDrawSelf(Rendering::DrawList &drawList);
	virtual void OnMouseEnter();
	virtual void OnMouseLeave();
	virtual void OnMousePress(Platform::MouseButton btn, vec2 pos);
	virtual void OnMouseRelease(Platform::MouseButton btn, vec2 pos);
	virtual void OnKeyPress(Platform::KeyCode key, int mods = 0) {}
	virtual void OnKeyRelease(Platform::KeyCode key) {}
	virtual void OnMouseMove(vec2 pos) {}
	virtual void OnCharInput(uint32 codepoint) {}
	virtual void OnFocusGained();
	virtual void OnFocusLost();
	virtual void OnStyleResolved() {}

	void QueueRedraw();

	void SetFloating(FloatingConfig cfg) { m_Floating = cfg; }
	void ClearFloating() { m_Floating.reset(); }
	bool HasFloating() const { return m_Floating.has_value(); }
	const FloatingConfig &GetFloating() const { return *m_Floating; }

	virtual View *HitTestAbsolute(vec2 canvasPos);

	void SetDrawDirty() { m_DrawDirty = true; }
	void ClearDrawDirty() { m_DrawDirty = false; }
	[[nodiscard]] bool IsDrawDirty() const { return m_DrawDirty; }

	void InvalidateLayout();

	void SetDirtyCallback(Delegate<void(View *)> cb) { m_OnDirty = std::move(cb); }
	void SetAnimationCallback(Delegate<void(View *)> cb) { m_OnAnimationStarted = std::move(cb); }
	void SetDrawDirtyCallback(Delegate<void(View *)> cb) { m_OnDrawDirty = std::move(cb); }
	void SetLayoutDirtyCallback(Delegate<void(View *)> cb) { m_OnLayoutDirty = std::move(cb); }

	void UpdateAnimation(float deltaTime);
	virtual ~View() = default;

  protected:
	bool m_IsHovered = false;
	bool m_IsPressed = false;
	bool m_IsFocused = false;

  private:
	View *m_Parent = nullptr;
	std::vector<Unique<View>> m_Children;

	ComputedStyle m_DisplayStyle;  // interpolated toward m_ComputedStyle each frame
	ComputedStyle m_AnimationFrom; // snapshot taken when a transition starts
	float m_TransitionTimer = 0.f;
	bool m_DisplayStyleInitialized = false;

	std::string m_Id;
	std::vector<std::string> m_Classes;
	StyleProperties m_Style;
	ComputedStyle m_ComputedStyle;
	Rect m_LayoutRect;
	vec2 m_AbsolutePosition; // canvas-space top-left, set by Canvas after layout
	bool m_Visible = true;
	bool m_Enabled = true;
	bool m_CapturesInput = false;
	bool m_PassThroughScroll = false;
	Delegate<void(vec2)> m_OnContextMenu;
	uint32 m_ClayId = 0;

	Option<FloatingConfig> m_Floating;

	bool m_IsDirty = true;
	bool m_IsAnimationFinished = false;
	bool m_DrawDirty = true;
	Delegate<void(View *)> m_OnDirty;
	Delegate<void(View *)> m_OnAnimationStarted;
	Delegate<void(View *)> m_OnDrawDirty;
	Delegate<void(View *)> m_OnLayoutDirty;
};
} // namespace Aquila::UI::Core
