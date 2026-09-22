#include "Core/EditorModule.h"

#include "Aquila/Application/ApplicationNew.h"

int main() {
	ApplicationSpec spec;
	spec.name = std::string("Aquila Studio | ") + AQUILA_VERSION_STRING;
	spec.width = 1920;
	spec.height = 1080;
	spec.start_hidden = true;

	Aquila::Application::Application app(spec);
	app.add_module<Editor::EditorModule>();
	app.run();
	return EXIT_SUCCESS;
}
