#ifndef CORE_H
#define CORE_H

#include "Defines.h"
#include "Engine/EditorCamera.h"
#include "Engine/Window.h"

#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Renderer.h"

#include "Engine/Events/EventRegistry.h"
#include "Engine/Events/EventBus.h"

#include "Platform/Filesystem/VirtualFileSystem.h"
#include "Platform/Filesystem/NativeFileSystem.h"

#include "Scene/SceneManager.h"

#include "Utilities/Singleton.h"
namespace Engine {
    class Controller : public Utility::Singleton<Controller> {
        friend class Utility::Singleton<Controller>;

    
        public:

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

        AQUILA_NONMOVEABLE(Controller);
        AQUILA_NONCOPYABLE(Controller);

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

        VFS::VirtualFileSystem* m_VFS;
    };

} // namespace Engine

#endif // CORE_H
