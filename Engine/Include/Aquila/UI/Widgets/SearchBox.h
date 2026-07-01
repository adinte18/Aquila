#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/TextInput.h"
#include "Aquila/UI/Widgets/Button.h"

namespace Aquila::UI::Core {

class SearchBox : public View {
  public:
	SearchBox();

	[[nodiscard]] std::string_view GetTypeName() const override { return "SearchBox"; }

	void SetPlaceholder(std::string placeholder);
	Signal<void(const std::string &)> onChanged;
	[[nodiscard]] const std::string &GetText() const;
	void Clear();

  private:
	void UpdateClearButton();

	TextInput *m_Input = nullptr;
	Button *m_ClearButton = nullptr;
};

} // namespace Aquila::UI::Core
