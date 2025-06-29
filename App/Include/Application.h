#ifndef APPLICATION_H
#define APPLICATION_H

#include "EditorLayer.h"

namespace Editor {
    class Application {
        Unique<EditorLayer> editorContext{};

    public:
        Application() {
            editorContext = std::make_unique<EditorLayer>();
        }

        ~Application() {
            // Clean up resources
            editorContext->OnEnd();
        }

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        void Run() {
            while (!editorContext->GetWindow().ShouldClose()) {
                editorContext->OnUpdate();
            }
        }
    };
}

#endif //APPLICATION_H
