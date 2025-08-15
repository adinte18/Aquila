#ifndef CORE_H
#define CORE_H

#include "Engine/EditorCamera.h"
#include "Engine/Window.h"

#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Renderer.h"

#include "Engine/Events/EventRegistry.h"
#include "Engine/Events/EventBus.h"

#include "Scene/SceneManager.h"

namespace Engine {
    class Controller {
    public:
        static Controller& Get() {
            static Controller instance;
            return instance;
        }

        void OnStart();
        void OnUpdate();
        void OnEnd();

        void InvalidatePasses();
        void HandleSceneChange();
        
        float GetDeltaTime();

        Device& GetDevice() const {
            return *m_Device;
        }

        Window& GetWindow() const {
            return *m_Window;
        }

        Renderer& GetRenderer() const {
            return *m_Renderer;
        }

        SceneManager& GetSceneManager() const {
            return *m_SceneManager;
        }

        Timer::Stopwatch& GetStopwatch() {
            return *m_Stopwatch;
        }

        EditorCamera& GetCamera() {
            return *m_EditorCamera;
        }

    private:
        Controller(); // defined in cpp
        ~Controller(); // defined in cpp

        Controller(const Controller&)                   = delete;
        Controller& operator=(const Controller&)        = delete;

        // Timing
        Unique<Timer::Stopwatch>            m_Stopwatch;
        float                               m_DeltaTime = 0.0f;

        // Camera
        Unique<EditorCamera>       m_EditorCamera;

        // Systems
        Unique<Window>             m_Window;
        Unique<Device>             m_Device;
        Unique<Renderer>           m_Renderer;
        Unique<SceneManager>       m_SceneManager;
        Unique<EventRegistry>      m_EventRegistry;
    };

} // namespace Engine

#endif // CORE_H
