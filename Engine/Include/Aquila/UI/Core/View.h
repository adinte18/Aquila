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
	// The interpolated style used for rendering — lerps toward GetComputedStyle() over transition-duration.
	[[nodiscard]] const ComputedStyle &GetDisplayStyle() const { return m_DisplayStyle; }
	[[nodiscard]] const Rect &GetLayoutRect() const { return m_LayoutRect; }
	// Canvas-space (absolute) top-left of this node — set by Canvas after layout.
	[[nodiscard]] vec2 GetAbsolutePosition() const { return m_AbsolutePosition; }

	void AddClass(std::string cls) { m_Classes.push_back(std::move(cls)); }

	void SetId(std::string id) { m_Id = std::move(id); }
	void SetStyle(StyleProperties props) { m_Style = props; }
	void SetComputedStyle(ComputedStyle style); // detects changes and starts transitions
	void SetLayoutRect(Rect rect) { m_LayoutRect = rect; }
	void SetAbsolutePosition(vec2 pos) { m_AbsolutePosition = pos; }
	void SetClayId(uint32 id) { m_ClayId = id; }
	void SetCapturesInput(bool v) { m_CapturesInput = v; }
	bool IsAnimationFinished() const { return m_IsAnimationFinished; }

	// Interaction state — read by StyleSheet to match :hover / :pressed / :focus rules.
	[[nodiscard]] bool IsHovered() const { return m_IsHovered; }
	[[nodiscard]] bool IsPressed() const { return m_IsPressed; }
	[[nodiscard]] bool IsFocused() const { return m_IsFocused; }
	[[nodiscard]] uint32 GetClayId() const { return m_ClayId; }
	[[nodiscard]] bool IsStyleDirty() const { return m_IsDirty; }

	// Returns the preferred content size for leaf nodes (e.g. measured text for Label).
	// Return {-1,-1} to let Clay size via the normal CSS rules.
	virtual vec2 GetIntrinsicSize() const { return { -1.f, -1.f }; }

	virtual void OnDraw(Rendering::DrawList &drawList, vec2 originOffset);
	// Generates draw commands for THIS node only (no child traversal).
	// Canvas drives the tree traversal and caches per-node output via OnDrawSelf.
	virtual void OnDrawSelf(Rendering::DrawList &drawList);
	virtual void OnMouseEnter();
	virtual void OnMouseLeave();
	virtual void OnMousePress(Platform::MouseButton btn, vec2 pos);
	virtual void OnMouseRelease(Platform::MouseButton btn, vec2 pos);
	virtual void OnKeyPress(Platform::KeyCode key) {}
	virtual void OnKeyRelease(Platform::KeyCode key) {}
	virtual void OnFocusGained();
	virtual void OnFocusLost();
	virtual void OnStyleResolved() {}

	void SetDirtyCallback(Delegate<void(View *)> cb) { m_OnDirty = std::move(cb); }
	void SetAnimationCallback(Delegate<void(View *)> cb) { m_OnAnimationStarted = std::move(cb); }

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
	uint32 m_ClayId = 0;

	bool m_IsDirty = true;
	bool m_IsAnimationFinished = false;
	Delegate<void(View *)> m_OnDirty;
	Delegate<void(View *)> m_OnAnimationStarted;
};
} // namespace Aquila::UI::Core
