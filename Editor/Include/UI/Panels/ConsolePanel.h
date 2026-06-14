#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/ScrollView.h"
#include "Aquila/UI/Widgets/Label.h"
#include "UI/Panels/IEditorPanel.h"

namespace Editor {

class ConsolePanel : public IEditorPanel {
  public:
	ConsolePanel() = default;
	void Build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlayRoot) override;

	Aquila::UI::Core::ScrollView *m_ScrollView = nullptr;
};

} // namespace Editor
