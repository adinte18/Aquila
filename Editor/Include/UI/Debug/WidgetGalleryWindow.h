#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"

#include <string>

namespace Aquila::Graphics {
class QuadBatcher;
}
namespace Aquila::GFX {
class GfxCommandList;
class GfxContext;
}
namespace Aquila::Application::Events {
class Event;
}
namespace Aquila::UI::Core {
class Canvas;
class View;
class Tooltip;
class TextureCache;
} // namespace Aquila::UI::Core

namespace Editor {

class WidgetGalleryWindow {
  public:
	WidgetGalleryWindow();
	~WidgetGalleryWindow();

	void Build(Aquila::GFX::GfxContext &ctx, Aquila::UI::Core::TextureCache *textureCache, uint32 width, uint32 height,
			   const std::string &stylePath);

	void Update(f32 deltaTime);
	void Render(Aquila::Graphics::QuadBatcher &batcher, Aquila::GFX::GfxCommandList &cmd);
	void OnEvent(Aquila::Application::Events::Event &event);

  private:
	Aquila::UI::Core::View *AddGroup(Aquila::UI::Core::View *host, const std::string &heading);

	Unique<Aquila::UI::Core::Canvas> m_Canvas;
	Aquila::UI::Core::Tooltip *m_Tooltip = nullptr;
};

} // namespace Editor
