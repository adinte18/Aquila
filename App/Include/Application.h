#ifndef APPLICATION_H
#define APPLICATION_H

#include "EditorLayer.h"

namespace Editor {
    class Application {
        std::unique_ptr<EditorLayer> editorContext{};
    public:
        Application() {
            editorContext = std::make_unique<EditorLayer>();
        }

        ~Application() {
            editorContext->OnEnd(); // Clean up resources
        }

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;


        void Run() {
            while (!editorContext->GetWindow().ShouldClose()) {
                editorContext->OnUpdate(); // Updates the application state and render
            }
        }
};
}

#endif //APPLICATION_H
