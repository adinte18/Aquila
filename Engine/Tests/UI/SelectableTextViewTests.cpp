#include <doctest.h>

#include "Aquila/UI/Widgets/SelectableTextView.h"

using namespace Aquila::UI::Core;

TEST_SUITE("SelectableTextView") {
	TEST_CASE("select all then read back the whole text") {
		SelectableTextView view;
		view.add_line("alpha");
		view.add_line("beta");
		view.add_line("gamma");

		view.select_all();
		CHECK(view.get_selected_text() == "alpha\nbeta\ngamma");
	}

	TEST_CASE("nothing is selected until the user selects") {
		SelectableTextView view;
		view.add_line("alpha");
		CHECK(view.get_selected_text().empty());
	}

	TEST_CASE("clearing the selection empties it") {
		SelectableTextView view;
		view.add_line("alpha");
		view.select_all();
		view.clear_selection();
		CHECK(view.get_selected_text().empty());
	}

	TEST_CASE("clear removes lines and selection") {
		SelectableTextView view;
		view.add_line("alpha");
		view.select_all();
		view.clear();

		CHECK(view.get_line_count() == 0);
		CHECK(view.get_selected_text().empty());
	}

	TEST_CASE("removing lines from the front keeps the selection on the same text") {
		SelectableTextView view;
		view.add_line("alpha");
		view.add_line("beta");
		view.add_line("gamma");
		view.select_all();

		view.remove_front(1);
		CHECK(view.get_line_count() == 2);
		CHECK(view.get_selected_text() == "beta\ngamma");
	}

	TEST_CASE("removing more lines than exist is harmless") {
		SelectableTextView view;
		view.add_line("alpha");
		view.remove_front(10);
		CHECK(view.get_line_count() == 0);
	}
}
