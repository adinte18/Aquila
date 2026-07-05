#include "Aquila/Application/ApplicationNew.h"

// Headless engine runner — starts with an empty scene, no editor UI.
// For the full editor experience build and run AquilaEditor instead.

int main() {
	ApplicationSpec spec;
	spec.name = "Aquila Runtime";
	spec.width = 1920;
	spec.height = 1080;

	Aquila::Application::Application app{ spec };
	app.run();
	return EXIT_SUCCESS;
}
