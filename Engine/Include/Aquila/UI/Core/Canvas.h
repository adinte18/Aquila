#pragma once

#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/UI/Core/DrawCompositor.h"
#include "Aquila/UI/Core/InputRouter.h"
#include "Aquila/UI/Core/LayoutEngine.h"
#include "Aquila/UI/Core/StyleEngine.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StyleSheet.h"

namespace Aquila::UI::Core {

using namespace Aquila::UI::Rendering;

class Canvas {
	friend class InputRouter; // calls MarkDirty()/RequestLayout() on input

  public:
	Canvas(uint32 width, uint32 height);

	void OnEvent(Application::Events::Event &event);
	void Update(float deltaTime);
	void Compute();
	void SubmitToQuadBatcher(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd);
	void Resize(uint32 width, uint32 height);

	StyleSheet &GetStyleSheet();
	View *GetRoot();
	View *HitTest(vec2 pos);
	void ScrollIntoView(View *target);
	uint32 GetWidth() const { return m_Width; }
	uint32 GetHeight() const { return m_Height; }
	void NotifyStyleDirty(View *view);
	void NotifyAnimationStarted(View *view);
	void NotifyDrawDirty(View *view);
	void NotifyLayoutDirty(View *view);
	void NotifyFocusRequest(View *view);
	void NotifyViewRemoved(View *view);
	void ReloadStyles();
	void MarkSubtreeDirty(View *node);

	void RegisterPopup(View *popup, Delegate<void()> onDismiss);
	void UnregisterPopup(View *popup);

	void RegisterTick(View *view);
	void UnregisterTick(View *view);

	bool IsDrawListDirty() const { return m_DrawListDirty; }
	void ClearDrawListDirty() { m_DrawListDirty = false; }

  private:
	void MarkNodeDrawDirty(View *node);
	void DismissPopupsOutside(View *hit);

	struct OpenPopup {
		View *root;
		Delegate<void()> onDismiss;
	};
	std::vector<OpenPopup> m_OpenPopups;
	std::vector<View *> m_Ticking;
	View *m_ScrollTarget = nullptr;

	Unique<View> m_Root;
	StyleEngine m_StyleEngine;
	LayoutEngine m_LayoutEngine;
	DrawCompositor m_DrawCompositor;
	InputRouter m_InputRouter;
	uint32 m_Width, m_Height;

	float m_DeltaTime = 0.f;

	std::vector<View *> m_ActiveAnims;

	bool m_LayoutDirty = true;
	bool m_Dirty = true;
	bool m_DrawListDirty = true; // true when draw list was rebuilt this frame

	void MarkDirty();
	void RequestLayout(); // mark layout dirty + request a frame (used by input/scroll)
	void StylePass();
	void AnimationPass(f32 dt);
};
} // namespace Aquila::UI::Core
