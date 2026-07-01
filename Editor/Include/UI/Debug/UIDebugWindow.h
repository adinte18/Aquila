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

	void Build(Aquila::UI::Core::Canvas *target, uint32 width, uint32 height, const std::string &stylePath);

	void Update(f32 deltaTime);
	void Render(Aquila::Graphics::QuadBatcher &batcher, Aquila::GFX::GfxCommandList &cmd);
	void OnEvent(Aquila::Application::Events::Event &event);
	void Refresh();

  private:
	Aquila::UI::Core::TreeNode *AddViewNode(Aquila::UI::Core::View *view, Aquila::UI::Core::TreeNode *parentNode);
	void ShowDetails(Aquila::UI::Core::View *view);

	Aquila::UI::Core::Canvas *m_Target = nullptr;
	Unique<Aquila::UI::Core::Canvas> m_Canvas;
	Aquila::UI::Core::View *m_TreeHost = nullptr;
	Aquila::UI::Core::View *m_DetailsHost = nullptr;
	Aquila::UI::Core::TreeView *m_Tree = nullptr;

	std::unordered_map<Aquila::UI::Core::TreeNode *, Aquila::UI::Core::View *> m_NodeToView;
};

} // namespace Editor
