#include <doctest.h>

#include "Aquila/UI/Widgets/DragFloat.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/VecField.h"

#include <algorithm>

using namespace Aquila::UI::Core;

namespace {

bool has_class(const View &view, const char *name) {
	const auto &classes = view.get_classes();
	return std::find(classes.begin(), classes.end(), name) != classes.end();
}

} // namespace

TEST_SUITE("PropertyGrid") {
	TEST_CASE("rows put the label beside the widget by default") {
		PropertyGrid grid;
		auto *field = grid.add_row<DragFloat>("Speed");

		const View *row = field->get_parent();
		CHECK(has_class(*row, "property-row"));
		CHECK_FALSE(has_class(*row, "property-row-stacked"));
	}

	TEST_CASE("stacked rows put the label above the widget") {
		PropertyGrid grid;
		auto *field = grid.add_stacked_row<Vec3Field>("Position");

		const View *row = field->get_parent();
		CHECK(has_class(*row, "property-row-stacked"));
		const View &label = *row->get_children().front();
		CHECK(has_class(label, "property-label-stacked"));
	}
}
