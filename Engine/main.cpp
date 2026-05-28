#include "Aquila/Application/ApplicationNew.h"

// Headless engine runner — starts with an empty scene, no editor UI.
// For the full editor experience build and run AquilaEditor instead.

int main() {
	ApplicationSpec spec;
	spec.Name = "Aquila Runtime";
	spec.Width = 1920;
	spec.Height = 1080;

	Aquila::Application::Application app{ spec };
	app.Run();
	return EXIT_SUCCESS;
}
