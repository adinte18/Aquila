#pragma once

#include "Aquila/Foundation/Singleton.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/UI/Core/Canvas.h"

namespace Aquila::UI::Core {

enum class UILayer : uint8 { World = 0, HUD = 1, Screen = 2, Overlay = 3 };

class ViewSystem : public Foundation::Singleton<ViewSystem> {
  public:
	Canvas &GetLayer(UILayer layer);

	void OnEvent(Application::Events::Event &e);
	void Update(float deltaTime);
	void Render(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd);
	void RenderLayers(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd, UILayer from, UILayer to);
	void Resize(uint32 width, uint32 height);

  private:
	friend class Foundation::Singleton<ViewSystem>;

	ViewSystem(uint32 width, uint32 height);

	std::array<Unique<Canvas>, 4> m_Layers;
};

} // namespace Aquila::UI::Core
