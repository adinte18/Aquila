#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Button.h"

namespace Aquila::UI::Core {

class Collapsible : public View {
  public:
	explicit Collapsible(std::string title = "");

	[[nodiscard]] std::string_view get_type_name() const override { return "Collapsible"; }

	void set_title(std::string title);
	void apply_xml_text_content(std::string_view text) override { set_title(std::string(text)); }
	void apply_xml_attribute(std::string_view name, std::string_view value, void *loader_ctx = nullptr) override;
	void set_expanded(bool expanded);
	[[nodiscard]] bool is_expanded() const { return m_expanded; }
	Signal<void(bool)> on_toggled;

	View *add_content(Unique<View> child);

	template <typename T, typename... Args> T *add_content(Args &&...args) {
		return static_cast<T *>(add_content(std::make_unique<T>(std::forward<Args>(args)...)));
	}

  private:
	void apply_state();

	bool m_expanded = true;
	Button *m_header = nullptr;
	View *m_content = nullptr;
};

} // namespace Aquila::UI::Core
