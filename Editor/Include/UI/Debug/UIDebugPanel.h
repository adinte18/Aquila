#pragma once

#include <string>
#include <unordered_map>

namespace Aquila::UI::Core {
class View;
class Canvas;
class TreeView;
class TreeNode;
} // namespace Aquila::UI::Core

namespace Editor {

class UIDebugPanel {
  public:
	void Build(Aquila::UI::Core::View *overlayRoot, Aquila::UI::Core::Canvas *target);
	void Toggle();
	void Refresh();

  private:
	Aquila::UI::Core::TreeNode *AddViewNode(Aquila::UI::Core::View *view, Aquila::UI::Core::TreeNode *parentNode);

	Aquila::UI::Core::Canvas *m_Target = nullptr;
	Aquila::UI::Core::View *m_Window = nullptr;
	Aquila::UI::Core::View *m_TreeHost = nullptr;
	Aquila::UI::Core::TreeView *m_Tree = nullptr;
	bool m_Visible = false;

	std::unordered_map<Aquila::UI::Core::TreeNode *, Aquila::UI::Core::View *> m_NodeToView;
};

} // namespace Editor
