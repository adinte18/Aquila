#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class ScrollView : public View {
  public:
	ScrollView();

	[[nodiscard]] std::string_view GetTypeName() const override { return "ScrollView"; }

	View *AddContent(Unique<View> child);

  private:
	View *m_Inner = nullptr;
};

} // namespace Aquila::UI::Core
