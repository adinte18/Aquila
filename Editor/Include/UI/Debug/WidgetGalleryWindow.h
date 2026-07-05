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

	void build(Aquila::GFX::GfxContext &ctx, Aquila::UI::Core::TextureCache *texture_cache, Uint32 width, Uint32 height,
			   const std::string &style_path);

	void update(F32 delta_time);
	void render(Aquila::Graphics::QuadBatcher &batcher, Aquila::GFX::GfxCommandList &cmd);
	void on_event(Aquila::Application::Events::Event &event);

  private:
	Aquila::UI::Core::View *add_group(Aquila::UI::Core::View *host, const std::string &heading);

	Unique<Aquila::UI::Core::Canvas> m_canvas;
	Aquila::UI::Core::Tooltip *m_tooltip = nullptr;
};

} // namespace Editor
