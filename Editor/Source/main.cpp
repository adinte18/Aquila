#include "Core/EditorApplication.h"

int main() {
	ApplicationSpec spec;
	spec.Name = "Aquila Studio";
	spec.Width = 1920;
	spec.Height = 1080;

	Editor::EditorApplication editor(spec);
	editor.Run();
	return EXIT_SUCCESS;
}
