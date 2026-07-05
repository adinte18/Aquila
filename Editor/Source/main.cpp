#include "Core/EditorApplication.h"

int main() {
	ApplicationSpec spec;
	spec.name = "Aquila Studio";
	spec.width = 1920;
	spec.height = 1080;

	Editor::EditorApplication editor(spec);
	editor.run();
	return EXIT_SUCCESS;
}
