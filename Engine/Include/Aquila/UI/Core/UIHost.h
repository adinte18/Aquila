#ifndef AQUILA_UI_CORE_UI_HOST_H
#define AQUILA_UI_CORE_UI_HOST_H

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Platform/Cursor.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/CanvasManager.h"

#include <functional>
#include <string>

namespace Aquila::Platform::Events {
class Event;
}

namespace Aquila::Rendering {
class Renderer2D;
}

namespace Aquila::UI::Core {

struct UIHostDesc {
	Aquila::Rendering::Renderer2D &renderer_2d;
	Uint32 width = 0;
	Uint32 height = 0;
	std::function<std::string()> clipboard_get{};
	std::function<void(const std::string &)> clipboard_set{};
};

class UIHost {
  public:
	explicit UIHost(const UIHostDesc &desc);
	~UIHost();

	AQUILA_NONCOPYABLE(UIHost);
	AQUILA_NONMOVEABLE(UIHost);

	void update(F32 delta_time);
	void on_event(Platform::Events::Event &event);

	[[nodiscard]] Canvas &get_canvas(UILayer layer) { return CanvasManager::get()->get_layer(layer); }
	[[nodiscard]] Platform::CursorType get_active_cursor() const { return CanvasManager::get()->get_active_cursor(); }
};

}

#endif
