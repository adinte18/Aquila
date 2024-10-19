#ifndef APPLICATION_H
#define APPLICATION_H

#include "EditorLayer.h"

namespace Editor {
    class Application {
        std::unique_ptr<EditorLayer> editorContext{};
    public:
        Application() {
            editorContext = std::make_unique<EditorLayer>();
            editorContext->OnStart(); // Initialize resources
        }

        ~Application() {
            editorContext->OnEnd(); // Clean up resources
        }

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;


        void Run() {
            while (!editorContext->window->shouldClose()) {
                editorContext->OnEvent(); // Handles input and window events
                editorContext->OnUpdate(); // Updates the application state and render
            }
        }
};
}

#endif //APPLICATION_H
