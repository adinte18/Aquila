#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class ScrollView : public View {
  public:
	ScrollView();

	[[nodiscard]] std::string_view get_type_name() const override { return "ScrollView"; }

	View *add_content(Unique<View> child);

	template <typename T, typename... Args> T *add_content(Args &&...args) {
		return static_cast<T *>(add_content(create_unique<T>(std::forward<Args>(args)...)));
	}

	void remove_oldest_content();

  private:
	View *m_inner = nullptr;
};

} // namespace Aquila::UI::Core
