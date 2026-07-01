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

	void Build(Unique<Aquila::UI::Core::View> panelSubtree, const std::string &title, uint32 width, uint32 height,
			   const std::string &stylePath);

	void Update(f32 deltaTime);
	void Render(Aquila::Graphics::QuadBatcher &batcher, Aquila::GFX::GfxCommandList &cmd);
	void OnEvent(Aquila::Application::Events::Event &event);

	[[nodiscard]] bool HasContent() const;
	Unique<Aquila::UI::Core::View> DetachContent();
	[[nodiscard]] const std::string &GetTitle() const { return m_Title; }
	[[nodiscard]] Aquila::UI::Core::DockSpace *GetDockSpace() const { return m_DockSpace; }

  private:
	Unique<Aquila::UI::Core::Canvas> m_Canvas;
	Aquila::UI::Core::View *m_Body = nullptr;
	Aquila::UI::Core::DockSpace *m_DockSpace = nullptr;
	std::string m_Title;
};

} // namespace Editor
