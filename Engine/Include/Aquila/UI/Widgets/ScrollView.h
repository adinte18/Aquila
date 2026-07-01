#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class ScrollView : public View {
  public:
	ScrollView();

	[[nodiscard]] std::string_view GetTypeName() const override { return "ScrollView"; }

	View *AddContent(Unique<View> child);

	template <typename T, typename... Args> T *AddContent(Args &&...args) {
		return static_cast<T *>(AddContent(CreateUnique<T>(std::forward<Args>(args)...)));
	}

	void RemoveOldestContent();

  private:
	View *m_Inner = nullptr;
};

} // namespace Aquila::UI::Core
