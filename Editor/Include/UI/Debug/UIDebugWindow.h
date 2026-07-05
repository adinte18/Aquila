#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"

#include <string>
#include <unordered_map>

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
class TreeView;
class TreeNode;
} // namespace Aquila::UI::Core

namespace Editor {

class UIDebugWindow {
  public:
	UIDebugWindow();
	~UIDebugWindow();

	void build(Aquila::UI::Core::Canvas *target, Uint32 width, Uint32 height, const std::string &style_path);

	void update(F32 delta_time);
	void render(Aquila::Graphics::QuadBatcher &batcher, Aquila::GFX::GfxCommandList &cmd);
	void on_event(Aquila::Application::Events::Event &event);
	void refresh();
	void select_view(Aquila::UI::Core::View *view);

	Delegate<void()> on_pick_requested;

  private:
	Aquila::UI::Core::TreeNode *add_view_node(Aquila::UI::Core::View *view, Aquila::UI::Core::TreeNode *parent_node);
	void show_details(Aquila::UI::Core::View *view);

	Aquila::UI::Core::Canvas *m_target = nullptr;
	Unique<Aquila::UI::Core::Canvas> m_canvas;
	Aquila::UI::Core::View *m_tree_host = nullptr;
	Aquila::UI::Core::View *m_details_host = nullptr;
	Aquila::UI::Core::TreeView *m_tree = nullptr;

	std::unordered_map<Aquila::UI::Core::TreeNode *, Aquila::UI::Core::View *> m_node_to_view;
};

} // namespace Editor
