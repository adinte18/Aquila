#ifndef APPLICATION_H
#define APPLICATION_H

#include "EditorLayer.h"
#include "Engine/Core.h"

namespace Editor {
    class Application {
        Unique<EditorLayer> editorContext{};

    public:
        Application() {
            Engine::Core::Get(); // Initialize the core components (window, device, render manager, etc.)
            editorContext = std::make_unique<EditorLayer>();
        }

        ~Application() {
            // Clean up resources
            editorContext->OnEnd();
        }

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        void Run() {
            while (!Engine::Core::Get().GetWindow().ShouldClose()) {
                editorContext->OnUpdate();
            }
        }
    };
}

#endif //APPLICATION_H
