#pragma once

#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Rendering/DrawList.h"

#include <array>
#include <unordered_map>
#include <vector>

namespace Aquila::UI::Core {

using namespace Aquila::UI::Rendering;

class DrawCompositor {
  public:
	void SetCanvasSize(uint32 width, uint32 height);

	void InvalidateAll(View *root);

	bool RebuildDirty(View *root);

	[[nodiscard]] View *HitTest(vec2 pos) const;

	void Submit(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd);

  private:
	void RebuildLists(View *root);
	void Cull(View *node, int32 parentEffectiveZ, const Rect *clipRect);
	void CollectLayer(View *node);
	void CollectLayerSubtree(View *node);
	void EmitFloatingLayer(View *node, const Rect *clipRect);

	std::array<std::vector<DrawCmd>, SharedConstants::Z_RANGE> m_ZBuckets;
	std::unordered_map<View *, std::vector<DrawCmd>> m_PerNodeCmds;
	std::vector<View *> m_CanvasItems;
	std::vector<View *> m_CanvasLayers;
	std::vector<View *> m_FloatRoots;
	DrawList m_DrawList;
	uint32 m_Width = 0;
	uint32 m_Height = 0;
};

} // namespace Aquila::UI::Core
