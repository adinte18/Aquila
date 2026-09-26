#include <doctest.h>

#include "Aquila/UI/Widgets/IconLabel.h"

using namespace Aquila::UI;
using namespace Aquila::UI::Core;

namespace {

Display label_display(const IconLabel &icon_label) {
	const View &label = *icon_label.get_children()[1];
	return label.get_style().display.value_or(Display::Flex);
}

} // namespace

TEST_SUITE("IconLabel") {
	TEST_CASE("an empty label takes no space so an icon can be centered") {
		IconLabel icon_label;
		CHECK(label_display(icon_label) == Display::None);
	}

	TEST_CASE("the label is shown once it has text") {
		IconLabel icon_label;
		icon_label.set_text("Move");
		CHECK(label_display(icon_label) == Display::Flex);

		icon_label.set_text("");
		CHECK(label_display(icon_label) == Display::None);
	}
}
