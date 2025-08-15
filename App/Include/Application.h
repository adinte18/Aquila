// #ifndef APPLICATION_H
// #define APPLICATION_H

// #include "Engine/Controller.h"

// namespace Editor {
//     class Application {
//         Unique<EditorLayer> editorContext{};

//     public:
//         Application() {
//             Engine::Controller::Get(); // Initialize the core components (window, device, render manager, etc.)
//             editorContext = std::make_unique<EditorLayer>();
//         }

//         ~Application() {
//             editorContext->OnEnd();
//         }

//         Application(const Application&) = delete;
//         Application& operator=(const Application&) = delete;

//         void Run() {
//             while (!Engine::Controller::Get().GetWindow().ShouldClose()) {
//                 editorContext->OnUpdate();
//             }
//         }
//     };
// }

// #endif //APPLICATION_H
