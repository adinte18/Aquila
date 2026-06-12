#pragma once

#include "Aquila/Foundation/Cache/ComputedCache.h"
#include "Aquila/Foundation/Invalidation/DirtySet.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StyleSheet.h"
#include <array>
#include <unordered_map>

namespace Aquila::UI::Core {

using namespace Aquila::UI::Rendering;

class Canvas {
  public:
	Canvas(uint32 width, uint32 height);

	void OnEvent(Application::Events::Event &event);
	void Update(float deltaTime);
	void Compute();
	void SubmitToQuadBatcher(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd);
	void Resize(uint32 width, uint32 height);

	StyleSheet &GetStyleSheet();
	View *GetRoot();
	uint32 GetWidth() const { return m_Width; }
	uint32 GetHeight() const { return m_Height; }
	void NotifyStyleDirty(View *view);
	void NotifyAnimationStarted(View *view);
	void ReloadStyles();
	void MarkSubtreeDirty(View *node);

	bool IsDrawListDirty() const { return m_DrawListDirty; }
	void ClearDrawListDirty() { m_DrawListDirty = false; }

  private:
	View *HitTest(vec2 pos);
	void ClayLayoutPass(View *node);
	bool ClayUpdateRects(View *node, vec2 parentAbsPos = {});
	void MarkNodeDrawDirty(View *node);
	void _CullCanvasItem(View *node, int32 parentEffectiveZ, const Rect *clipRect);
	void _CollectCanvasLayer(View *node);
	void _CollectCanvasLayerSubtree(View *node);

	Unique<View> m_Root;
	StyleSheet m_StyleSheet;
	DrawList m_DrawList;
	View *m_HoveredView = nullptr;
	View *m_FocusedView = nullptr;
	uint32 m_Width, m_Height;

	void *m_ClayCtx = nullptr;
	std::vector<uint8_t> m_ClayMemory;

	vec2 m_MousePos = {};
	bool m_MouseDown = false;
	vec2 m_ScrollDelta = {};
	float m_DeltaTime = 0.f;

	Foundation::DirtySet<View *> m_DirtyViews;
	Foundation::ComputedCache<View *, ComputedStyle> m_StyleCache;
	std::array<std::vector<DrawCmd>, SharedConstants::Z_RANGE> m_ZBuckets;
	std::vector<View *> m_CanvasItems;
	std::vector<View *> m_CanvasLayers;
	std::unordered_map<View *, std::vector<DrawCmd>> m_PerNodeCmds;
	std::vector<View *> m_ActiveAnims;

	bool m_LayoutDirty = true;
	bool m_Dirty = true;
	bool m_DrawListDirty = true; // true when draw list was rebuilt this frame

	void MarkDirty();
	void SetFocus(View *view); // handles OnFocusLost/OnFocusGained bookkeeping
	void StylePass();
	void AnimationPass(f32 dt);
};
} // namespace Aquila::UI::Core
