#pragma once

#include "Aquila/UI/Widgets/Control.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Popup.h"
#include <string>
#include <vector>

namespace Aquila::UI::Core {

class Dropdown : public Control {
  public:
	Dropdown();

	[[nodiscard]] std::string_view get_type_name() const override { return "Dropdown"; }

	void add_option(std::string value, std::string display = "");
	void clear_options();

	void set_value(const std::string &value);
	[[nodiscard]] const std::string &get_value() const { return m_value; }

	void set_placeholder(std::string text);
	Signal<void(const std::string &)> on_changed;

  private:
	struct Option {
		std::string value;
		std::string display;
		[[nodiscard]] const std::string &label() const { return display.empty() ? value : display; }
	};

	void rebuild();
	void select(const std::string &value);
	void update_header_text();

	Button *m_header = nullptr;
	Popup *m_popup = nullptr;
	std::vector<Option> m_options;
	std::string m_value;
	std::string m_placeholder = "Select…";
	std::vector<View *> m_option_buttons;
};

} // namespace Aquila::UI::Core
