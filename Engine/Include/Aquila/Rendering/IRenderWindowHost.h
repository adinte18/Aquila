#ifndef AQUILA_RENDERING_I_RENDER_WINDOW_HOST_H
#define AQUILA_RENDERING_I_RENDER_WINDOW_HOST_H

#include "Aquila/Foundation/Math/MathTypes.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

#include <string>

namespace Aquila::Graphics {
class QuadBatcher;
}

namespace Aquila::GFX {
class GfxCommandList;
}

namespace Aquila::Platform::Events {
class Event;
}

namespace Aquila::Rendering {

using RenderWindowId = const void *;

struct RenderWindowCallbacks {
	Delegate<void(F32)> on_update;
	Delegate<void(Graphics::QuadBatcher &, GFX::GfxCommandList &)> on_render;
	Delegate<void(Platform::Events::Event &)> on_event;
	Delegate<void()> on_close;
};

class IRenderWindowHost {
  public:
	virtual ~IRenderWindowHost() = default;

	virtual RenderWindowId create_window(Uint32 width, Uint32 height, const std::string &title,
										 RenderWindowCallbacks callbacks) = 0;
	virtual void request_close(RenderWindowId window) = 0;

	[[nodiscard]] virtual RenderWindowId get_main_window() const = 0;
	[[nodiscard]] virtual Vec2 get_window_position(RenderWindowId window) const = 0;
	[[nodiscard]] virtual Vec2 get_window_size(RenderWindowId window) const = 0;
	virtual void set_window_position(RenderWindowId window, Vec2 position) = 0;
};

}

#endif
