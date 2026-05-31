#pragma once

#include "Aquila/UI/Core/View.h"
#include <string>

namespace Aquila::UI::Core {

class DockPanel : public View {
  public:
	explicit DockPanel(std::string title);

	[[nodiscard]] std::string_view GetTypeName() const override { return "DockPanel"; }
	[[nodiscard]] const std::string &GetTitle() const { return m_Title; }
	void SetTitle(std::string title);

  private:
	std::string m_Title;
};

} // namespace Aquila::UI::Core
