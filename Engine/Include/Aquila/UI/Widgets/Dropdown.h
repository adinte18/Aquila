#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Popup.h"
#include <string>
#include <vector>

namespace Aquila::UI::Core {

class Dropdown : public View {
  public:
	Dropdown();

	[[nodiscard]] std::string_view GetTypeName() const override { return "Dropdown"; }

	void AddOption(std::string value, std::string display = "");
	void ClearOptions();

	void SetValue(const std::string &value);
	[[nodiscard]] const std::string &GetValue() const { return m_Value; }

	void SetPlaceholder(std::string text);
	void SetOnChanged(Delegate<void(const std::string &)> callback);

  private:
	struct Option {
		std::string value;
		std::string display;
		[[nodiscard]] const std::string &Label() const { return display.empty() ? value : display; }
	};

	void Rebuild();
	void Select(const std::string &value);
	void UpdateHeaderText();

	Button *m_Header = nullptr;
	Popup *m_Popup = nullptr;
	std::vector<Option> m_Options;
	std::string m_Value;
	std::string m_Placeholder = "Select…";
	std::vector<View *> m_OptionButtons;
	Delegate<void(const std::string &)> m_OnChanged;
};

} // namespace Aquila::UI::Core
