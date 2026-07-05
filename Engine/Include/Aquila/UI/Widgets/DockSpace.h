#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/DockDragContext.h"
#include "Aquila/UI/Widgets/DockTypes.h"

namespace Aquila::UI::Core {

class DockNode;
class DockPanel;
class DockSplitter;

class DockSpace : public View {
  public:
	DockSpace();

	[[nodiscard]] std::string_view GetTypeName() const override { return "DockSpace"; }
	[[nodiscard]] DockNode *GetRootNode() const { return m_Root; }

	// Compiles a declared <DockNode>/<DockPanel> child tree into the runtime dock structure.
	void OnXmlLoaded() override;

	[[nodiscard]] bool HasAnyPanels() const;
	[[nodiscard]] DockNode *FirstLeafWithTabs() const;
	void SetTearOffCallback(Delegate<void(Unique<View>, std::string, vec2)> cb) { m_OnTearOff = std::move(cb); }

	void SetEmptiedCallback(Delegate<void()> cb) { m_OnEmptied = std::move(cb); }

	void SetExternalDragObserver(Delegate<void(vec2)> onOutside, Delegate<void()> onInside) {
		m_OnExternalDragMove = std::move(onOutside);
		m_OnExternalDragClear = std::move(onInside);
	}
	bool TryDockExternal(Unique<View> &panelView, const std::string &title, vec2 localPos);
	void PreviewExternalDrag(vec2 localPos);
	void ClearExternalDrag();
	void BeginExternalDrag(DockPanel *panel, const std::string &title);

  private:
	void CompileDeclaration(DockNode *realNode, DockNode *declNode);
	void RealizeSlot(DockNode *realLeaf, View *slot);
	void RealizePanel(DockNode *realLeaf, DockPanel *declPanel);

	void ExecuteDrop(DockNode *target, DropZone zone, vec2 releasePos);
	void CollapseNode(DockNode *node);
	void HoistSingleChild(DockNode *container, DockNode *only);
	void UpdatePreview(DockNode *target, DropZone zone);

	[[nodiscard]] bool IsOutsideCanvas(vec2 pos) const;

	DockDragContext m_DragCtx;
	DockNode *m_Root = nullptr;
	DockNode *m_DropTarget = nullptr;
	DropZone m_CurrentZone = DropZone::None;
	View *m_DropPreview = nullptr;
	Delegate<void(Unique<View>, std::string, vec2)> m_OnTearOff;
	Delegate<void()> m_OnEmptied;
	Delegate<void(vec2)> m_OnExternalDragMove;
	Delegate<void()> m_OnExternalDragClear;
	bool m_DragLeftCanvas = false;
};

} // namespace Aquila::UI::Core
