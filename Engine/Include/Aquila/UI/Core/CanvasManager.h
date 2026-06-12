#pragma once

#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/Foundation/Singleton.h"

namespace Aquila::UI::Core {

enum class UILayer : uint8 {
	WorldSpace = 0,
	ScreenCamera = 1,
	ScreenOverlay = 2,
	Editor = 3,
	Count // keep track of how many layers we have
};

class CanvasManager : public Foundation::Singleton<CanvasManager> {
  public:
	Canvas &GetLayer(UILayer layer);

	void OnEvent(Application::Events::Event &e);
	void Update(float deltaTime);
	void Compute();

	void RenderLayers(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd, UILayer from, UILayer to);
	void Resize(uint32 width, uint32 height);

	bool IsAnyLayerDirty(UILayer from, UILayer to) const;
	void ClearLayerDirtyFlags(UILayer from, UILayer to);

  private:
	friend class Foundation::Singleton<CanvasManager>;
	CanvasManager(uint32 width, uint32 height);

	std::array<Unique<Canvas>, static_cast<size_t>(UILayer::Count)> m_Layers;
};

} // namespace Aquila::UI::Core
