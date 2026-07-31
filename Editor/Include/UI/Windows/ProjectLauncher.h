#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Core/ProjectManager.h"

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
class TextInput;
} // namespace Aquila::UI::Core

namespace Editor {

class ProjectLauncher {
  public:
	ProjectLauncher();
	~ProjectLauncher();

	void build(ProjectManager *project_manager, Uint32 width, Uint32 height, const std::string &style_path);

	void update(F32 delta_time);
	void render(Aquila::Graphics::QuadBatcher &batcher, Aquila::GFX::GfxCommandList &cmd);
	void on_event(Aquila::Application::Events::Event &event);

	Delegate<void(const ProjectInfo &)> on_project_ready;

  private:
	void refresh_list();
	void open_project(const ProjectInfo &project);
	void create_project();

	Unique<Aquila::UI::Core::Canvas> m_canvas;
	ProjectManager *m_project_manager = nullptr;
	Aquila::UI::Core::View *m_list_host = nullptr;
	Aquila::UI::Core::TextInput *m_name_input = nullptr;
};

} // namespace Editor
