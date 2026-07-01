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
	Signal<void(bool)> onToggled;

	View *AddContent(Unique<View> child);

	template <typename T, typename... Args> T *AddContent(Args &&...args) {
		return static_cast<T *>(AddContent(CreateUnique<T>(std::forward<Args>(args)...)));
	}

  private:
	void ApplyState();

	bool m_Expanded = true;
	Button *m_Header = nullptr;
	View *m_Content = nullptr;
};

} // namespace Aquila::UI::Core
