#include "Aquila/Application/ApplicationNew.h"

using namespace Aquila;

int main(int argc, char **argv) {
	ApplicationSpec spec;
	spec.Name = "Aquila";
	spec.Width = 1920;
	spec.Height = 1080;

	Application::Application app{ spec };
	app.OnStart();
	app.Run();
	return EXIT_SUCCESS;
}
