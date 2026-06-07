#pragma once

#include "Aquila/Platform/Input.h"
#include "Aquila/UI/Rendering/DrawList.h"
#include "Aquila/UI/Style/ComputedStyle.h"
#include "Aquila/UI/Style/StyleProperties.h"

namespace Aquila::UI::Text {
class FontAtlas;
}

namespace Aquila::UI::Core {

class View {
  public:
	View() = default;
	AQUILA_NONCOPYABLE(View);
	AQUILA_NONMOVEABLE(View);

	View *AddChild(Unique<View> child);
	void RemoveChild(View *child);
	Unique<View> DetachChild(View *child);
	View *ReplaceChild(View *old, Unique<View> newChild);
	[[nodiscard]] View *GetParent() const;
	[[nodiscard]] const std::vector<Unique<View>> &GetChildren() const;
	View *FindById(std::string_view id);

	template <typename T> T *FindById(std::string_view id) {
		View *v = FindById(id);
		return v ? dynamic_cast<T *>(v) : nullptr;
	}
	void PropagateCallbacks(View *node);

	[[nodiscard]] virtual std::string_view GetTypeName() const { return "View"; }
	[[nodiscard]] const std::string &GetId() const { return m_Id; }
	[[nodiscard]] const std::vector<std::string> &GetClasses() const { return m_Classes; }
	[[nodiscard]] const StyleProperties &GetStyle() const { return m_Style; }
	[[nodiscard]] const ComputedStyle &GetComputedStyle() const { return m_ComputedStyle; }
	[[nodiscard]] const ComputedStyle &GetDisplayStyle() const { return m_DisplayStyle; }
	[[nodiscard]] const Rect &GetLayoutRect() const { return m_LayoutRect; }
	[[nodiscard]] vec2 GetAbsolutePosition() const { return m_AbsolutePosition; }

	void AddClass(std::string cls) {
		m_Classes.push_back(std::move(cls));
		if (m_OnDirty) {
			m_OnDirty(this);
		}
	}
	void RemoveClass(std::string_view cls) {
		auto it = std::ranges::find(m_Classes, cls);
		if (it != m_Classes.end()) {
			m_Classes.erase(it);
			if (m_OnDirty) {
				m_OnDirty(this);
			}
		}
	}

	void SetId(std::string id) { m_Id = std::move(id); }
	void SetStyle(StyleProperties props) { m_Style = std::move(props); }

	void MergeStyle(const StyleProperties &overlay);
	void SetComputedStyle(ComputedStyle style);
	void SetLayoutRect(Rect rect) {
		if (m_LayoutRect == rect) return;
		m_LayoutRect = rect;
		MarkSubtreeBoundsDirty();
	}
	void SetAbsolutePosition(vec2 pos) {
		if (m_AbsolutePosition == pos) return;
		m_AbsolutePosition = pos;
		MarkSubtreeBoundsDirty();
	}
	void SetClayId(uint32 id) { m_ClayId = id; }

	void SetInputLeaf(bool v) { m_IsInputLeaf = v; }
	[[nodiscard]] bool IsInputLeaf() const { return m_IsInputLeaf; }
	[[nodiscard]] bool IsVisible() const { return m_Visible; }

	void SetEnabled(bool enabled);
	[[nodiscard]] bool IsEnabled() const { return m_Enabled; }

	[[nodiscard]] Text::FontAtlas *GetResolvedFont() const { return m_ResolvedFont; }

	void RequestFocus();
	void SetPassThroughScroll(bool v) { m_PassThroughScroll = v; }
	[[nodiscard]] bool GetPassThroughScroll() const { return m_PassThroughScroll; }

	[[nodiscard]] Rect GetAbsoluteRect() const { return { m_AbsolutePosition, m_LayoutRect.size }; }

	void MarkSubtreeBoundsDirty() {
		m_SubtreeBoundsDirty = true;
		if (m_Parent) {
			m_Parent->MarkSubtreeBoundsDirty();
		}
	}
	const Rect &GetSubtreeBounds();

	void SetContextView(Delegate<void(vec2)> cb) { m_OnContextMenu = std::move(cb); }
	bool IsAnimationFinished() const { return m_IsAnimationFinished; }

	[[nodiscard]] bool IsHovered() const { return m_IsHovered; }
	[[nodiscard]] bool IsPressed() const { return m_IsPressed; }
	[[nodiscard]] bool IsFocused() const { return m_IsFocused; }
	[[nodiscard]] uint32 GetClayId() const { return m_ClayId; }

	virtual vec2 GetIntrinsicSize() const { return { -1.f, -1.f }; }

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

	virtual void OnStyleResolved();

	virtual void ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx = nullptr);

	virtual void ApplyXmlTextContent(std::string_view text) { (void)text; }

	virtual void SetFont(Text::FontAtlas * /*font*/) {}

	void QueueRedraw();

	void SetFloating(FloatingConfig cfg) { m_Floating = cfg; }
	void ClearFloating() { m_Floating.reset(); }
	bool HasFloating() const { return m_Floating.has_value(); }
	const FloatingConfig &GetFloating() const { return *m_Floating; }

	virtual View *HitTestAbsolute(vec2 canvasPos);

	void SetDrawDirty() { m_DrawDirty = true; }
	void ClearDrawDirty() { m_DrawDirty = false; }
	[[nodiscard]] bool IsDrawDirty() const { return m_DrawDirty; }

	void SetEffectiveZ(int32 z) { m_EffectiveZ = z; }
	[[nodiscard]] int32 GetEffectiveZ() const { return m_EffectiveZ; }

	void InvalidateLayout();

	void SetDirtyCallback(Delegate<void(View *)> cb) { m_OnDirty = std::move(cb); }
	void SetAnimationCallback(Delegate<void(View *)> cb) { m_OnAnimationStarted = std::move(cb); }
	void SetDrawDirtyCallback(Delegate<void(View *)> cb) { m_OnDrawDirty = std::move(cb); }
	void SetLayoutDirtyCallback(Delegate<void(View *)> cb) { m_OnLayoutDirty = std::move(cb); }
	void SetFocusRequestCallback(Delegate<void(View *)> cb) { m_OnFocusRequest = std::move(cb); }
	void SetRemoveCallback(Delegate<void(View *)> cb) { m_OnRemoved = std::move(cb); }

	void UpdateAnimation(float deltaTime);
	virtual ~View() = default;

  protected:
	bool m_IsHovered = false;
	bool m_IsPressed = false;
	bool m_IsFocused = false;

  private:
	void NotifyRemoved(View *node);

	View *m_Parent = nullptr;
	std::vector<Unique<View>> m_Children;

	ComputedStyle m_DisplayStyle;
	ComputedStyle m_AnimationFrom;
	float m_TransitionTimer = 0.f;
	bool m_DisplayStyleInitialized = false;

	std::string m_Id;
	std::vector<std::string> m_Classes;
	StyleProperties m_Style;
	ComputedStyle m_ComputedStyle;
	Rect m_LayoutRect;
	vec2 m_AbsolutePosition;
	bool m_Visible = true;
	bool m_Enabled = true;
	bool m_IsInputLeaf = false;
	bool m_PassThroughScroll = false;
	Delegate<void(vec2)> m_OnContextMenu;
	uint32 m_ClayId = 0;

	Option<FloatingConfig> m_Floating;

	bool m_IsAnimationFinished = false;
	bool m_DrawDirty = true;
	int32 m_EffectiveZ = 0;
	Rect m_SubtreeBounds{};
	bool m_SubtreeBoundsDirty = true;
	Text::FontAtlas *m_ResolvedFont = nullptr;
	Delegate<void(View *)> m_OnDirty;
	Delegate<void(View *)> m_OnAnimationStarted;
	Delegate<void(View *)> m_OnDrawDirty;
	Delegate<void(View *)> m_OnLayoutDirty;
	Delegate<void(View *)> m_OnFocusRequest;
	Delegate<void(View *)> m_OnRemoved;
};
} // namespace Aquila::UI::Core
