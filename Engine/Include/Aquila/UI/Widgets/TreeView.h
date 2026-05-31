#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Button.h"
#include <string>
#include <vector>

namespace Aquila::UI::Core {

class TreeNode;

class TreeView : public View {
  public:
	TreeView();

	[[nodiscard]] std::string_view GetTypeName() const override { return "TreeView"; }

	TreeNode *AddNode(std::string label);

	void RemoveNode(TreeNode *node);

	void SetOnSelected(Delegate<void(TreeNode *)> callback);
	void SetOnRightClicked(Delegate<void(TreeNode *, vec2)> callback);

	void SelectNode(TreeNode *node);
	[[nodiscard]] TreeNode *GetSelected() const { return m_Selected; }

  private:
	friend class TreeNode;
	void NotifySelected(TreeNode *node);
	void NotifyRightClicked(TreeNode *node, vec2 pos);

	View *m_Content = nullptr;
	TreeNode *m_Selected = nullptr;
	Delegate<void(TreeNode *)> m_OnSelected;
	Delegate<void(TreeNode *, vec2)> m_OnRightClicked;
};

class TreeNode : public View {
  public:
	TreeNode(std::string label, TreeView &owner, int depth);

	[[nodiscard]] std::string_view GetTypeName() const override { return "TreeNode"; }

	TreeNode *AddChild(std::string label);
	void SetLabel(std::string label);
	void SetExpanded(bool expanded);
	[[nodiscard]] bool IsExpanded() const { return m_Expanded; }
	[[nodiscard]] const std::string &GetLabel() const { return m_Label; }
	[[nodiscard]] int GetDepth() const { return m_Depth; }

	void SetSelected(bool selected);

  private:
	void ApplyState();
	void OnHeaderClicked();
	void OnHeaderRightClicked(vec2 pos);

	std::string m_Label;
	TreeView &m_Owner;
	int m_Depth = 0;
	bool m_Expanded = true;

	Button *m_Header = nullptr;
	View *m_Children = nullptr;
};

} // namespace Aquila::UI::Core
