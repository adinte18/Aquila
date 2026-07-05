#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"

#include <string>

namespace Aquila::Graphics {
class QuadBatcher;
}
namespace Aquila::GFX {
class GfxCommandList;
}
namespace Aquila::Application::Events {
class Event;
}
namespace Aquila::UI::Core {
class Canvas;
class View;
class DockSpace;
} // namespace Aquila::UI::Core

namespace Editor {

class FloatingPanelWindow {
  public:
	FloatingPanelWindow();
	~FloatingPanelWindow();

	void build(Unique<Aquila::UI::Core::View> panel_subtree, const std::string &title, Uint32 width, Uint32 height,
			   const std::string &style_path);

	void update(F32 delta_time);
	void render(Aquila::Graphics::QuadBatcher &batcher, Aquila::GFX::GfxCommandList &cmd);
	void on_event(Aquila::Application::Events::Event &event);

	[[nodiscard]] bool has_content() const;
	Unique<Aquila::UI::Core::View> detach_content();
	[[nodiscard]] const std::string &get_title() const { return m_title; }
	[[nodiscard]] Aquila::UI::Core::DockSpace *get_dock_space() const { return m_dock_space; }

  private:
	Unique<Aquila::UI::Core::Canvas> m_canvas;
	Aquila::UI::Core::View *m_body = nullptr;
	Aquila::UI::Core::DockSpace *m_dock_space = nullptr;
	std::string m_title;
};

} // namespace Editor
