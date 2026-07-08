#include "Core/EditorApplication.h"

int main() {
	ApplicationSpec spec;
	spec.name = std::string("Aquila Studio | ") + AQUILA_VERSION_STRING;
	spec.width = 1920;
	spec.height = 1080;

	Editor::EditorApplication editor(spec);
	editor.run();
	return EXIT_SUCCESS;
}
