#pragma once

#include "Aquila/Platform/Input.h"
#include "Aquila/UI/Rendering/DrawList.h"
#include "Aquila/UI/Style/ComputedStyle.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "Aquila/UI/Core/DragState.h"

namespace Aquila::UI::Text {
class FontAtlas;
}

namespace Aquila::UI::Core {

class Canvas;

class View {
  public:
	View();
	AQUILA_NONCOPYABLE(View);
	AQUILA_NONMOVEABLE(View);

	virtual View *AddChild(Unique<View> child);

	template <typename T, typename... Args> T *AddChild(Args &&...args) {
		return static_cast<T *>(AddChild(CreateUnique<T>(std::forward<Args>(args)...)));
	}

	void RemoveChild(View *child);
	Unique<View> DetachChild(View *child);
	View *ReplaceChild(View *old, Unique<View> newChild);
	[[nodiscard]] View *GetParent() const;
	[[nodiscard]] View *GetFirstDraggableParent() const;
	[[nodiscard]] View *GetFirstParentThatAcceptsDrop() const;
	[[nodiscard]] const std::vector<Unique<View>> &GetChildren() const;
	View *FindById(std::string_view id);

	template <typename T> T *FindById(std::string_view id) {
		View *v = FindById(id);
		return v ? dynamic_cast<T *>(v) : nullptr;
	}

	void SetCanvas(Canvas *canvas);
	[[nodiscard]] Canvas *GetCanvas() const { return m_Canvas; }

	[[nodiscard]] virtual std::string_view GetTypeName() const { return "View"; }
	[[nodiscard]] const std::string &GetId() const { return m_Id; }
	[[nodiscard]] const std::vector<std::string> &GetClasses() const { return m_Classes; }
	[[nodiscard]] const StyleProperties &GetStyle() const { return m_Style; }
	[[nodiscard]] const ComputedStyle &GetComputedStyle() const { return m_ComputedStyle; }
	[[nodiscard]] const ComputedStyle &GetDisplayStyle() const { return m_DisplayStyle; }
	[[nodiscard]] const Rect &GetLayoutRect() const { return m_LayoutRect; }
	[[nodiscard]] vec2 GetAbsolutePosition() const { return m_AbsolutePosition; }

	void AddClass(std::string cls);
	void RemoveClass(std::string_view cls);

	void SetId(std::string id) { m_Id = std::move(id); }
	void SetStyle(StyleProperties props) { m_Style = std::move(props); }

	void MergeStyle(const StyleProperties &overlay);
	void SetComputedStyle(ComputedStyle style);
	void SetLayoutRect(Rect rect) {
		if (m_LayoutRect == rect) {
			return;
		}
		m_LayoutRect = rect;
		MarkSubtreeBoundsDirty();
	}
	void SetAbsolutePosition(vec2 pos) {
		if (m_AbsolutePosition == pos) {
			return;
		}
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

	Signal<void(vec2)> onContextMenu;
	bool IsAnimationFinished() const { return m_IsAnimationFinished; }

	[[nodiscard]] bool IsHovered() const { return m_IsHovered; }
	[[nodiscard]] bool IsPressed() const { return m_IsPressed; }
	[[nodiscard]] bool IsFocused() const { return m_IsFocused; }
	[[nodiscard]] bool IsSkippingHitTest() const { return m_ShouldSkipHitTest; }
	[[nodiscard]] bool IsAcceptingPayload() const { return m_IsAcceptingPayload; }
	[[nodiscard]] bool IsDraggable() const { return m_IsDraggable; }
	[[nodiscard]] uint32 GetClayId() const { return m_ClayId; }

	[[nodiscard]] uint32 GetStableId() const { return m_StableId; }

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
	virtual void OnDragStart(DragState &dState);
	virtual void OnDrop(DragState &dState);
	virtual void OnDragEnter(DragState &dState);
	virtual void OnDragLeave(DragState &dState);

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

	void UpdateAnimation(float deltaTime);
	virtual ~View() = default;

  protected:
	bool m_IsHovered = false;
	bool m_IsPressed = false;
	bool m_IsFocused = false;
	bool m_ShouldSkipHitTest = false;
	bool m_IsAcceptingPayload = false;
	bool m_IsDraggable = false;

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
	uint32 m_ClayId = 0;

	Option<FloatingConfig> m_Floating;

	bool m_IsAnimationFinished = false;
	bool m_DrawDirty = true;
	int32 m_EffectiveZ = 0;
	Rect m_SubtreeBounds{};
	bool m_SubtreeBoundsDirty = true;
	Text::FontAtlas *m_ResolvedFont = nullptr;
	Canvas *m_Canvas = nullptr;
	uint32 m_StableId = 0;
};
} // namespace Aquila::UI::Core
