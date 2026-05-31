#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Button.h"

namespace Aquila::UI::Core {

class Collapsible : public View {
  public:
	explicit Collapsible(std::string title = "");

	[[nodiscard]] std::string_view GetTypeName() const override { return "Collapsible"; }

	void SetTitle(std::string title);
	void SetExpanded(bool expanded);
	[[nodiscard]] bool IsExpanded() const { return m_Expanded; }
	void SetOnToggled(Delegate<void(bool)> callback);

	View *AddContent(Unique<View> child);

  private:
	void ApplyState();

	bool m_Expanded = true;
	Button *m_Header = nullptr;
	View *m_Content = nullptr;
	Delegate<void(bool)> m_OnToggled;
};

} // namespace Aquila::UI::Core
