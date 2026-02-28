#include "Core/EditorApplication.h"
#include "Core/EditorConfig.h"

int main(int argc, char **argv) {
	AQUILA_LOG_INFO("===========================================");
	AQUILA_LOG_INFO("    Aquila Editor - Starting");
	AQUILA_LOG_INFO("===========================================");

	try {
		Aquila::Core::ApplicationConfig config;
		config.windowWidth = 1920;
		config.windowHeight = 1080;
		config.windowTitle = Editor::Config::EDITOR_TITLE;

		config.assetPath = ASSET_PATH;

		Editor::Application editor(config);

		editor.Run();

		AQUILA_LOG_INFO("Editor exited normally");

	} catch (const std::exception &e) {
		AQUILA_LOG_ERROR("Fatal error: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		AQUILA_LOG_ERROR("Unknown fatal error occurred");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
