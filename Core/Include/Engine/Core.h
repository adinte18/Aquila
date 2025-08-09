#ifndef CORE_H
#define CORE_H

#include "Engine/RenderManager.h"
#include "Engine/Window.h"
#include "Engine/Device.h"

#include "Engine/Renderer.h"

#include "Engine/Events/EventRegistry.h"
#include "Engine/Events/EventBus.h"

#include "Scene/SceneManager.h"

namespace Engine {

    class Clock {
    public:
        Clock() {
            Reset();
        }

        void Reset() {
            m_StartTime = std::chrono::steady_clock::now();
            m_LastFrameTime = m_StartTime;
        }

        void Tick() {
            auto currentTime = std::chrono::steady_clock::now();
            std::chrono::duration<float> frameDuration = currentTime - m_LastFrameTime;
            m_DeltaTime = frameDuration.count();
            m_LastFrameTime = currentTime;
        }

        float GetDeltaTime() const {
            return m_DeltaTime;
        }

        float GetElapsedTime() const {
            std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - m_StartTime;
            return elapsed.count();
        }

    private:
        std::chrono::steady_clock::time_point   m_StartTime;
        std::chrono::steady_clock::time_point   m_LastFrameTime;
        float                                   m_DeltaTime = 0.0f;
    };

    class Core {
    public:
        static Core& Get() {
            static Core instance;
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

        Clock& GetClock() const {
            return *m_Clock;
        }

        RenderManager& GetRenderManager() const {
            return *m_RenderManager;
        }

        Renderer& GetRenderer() const {
            return *m_Renderer;
        }

        SceneManager& GetSceneManager() const {
            return *m_SceneManager;
        }

    private:
        Core(); // defined in cpp
        ~Core(); // defined in cpp

        Core(const Core&)                   = delete;
        Core& operator=(const Core&)        = delete;

        // Timing
        Unique<Clock>                       m_Clock;
        float                               m_DeltaTime = 0.0f;

        // Systems
        Unique<Window>             m_Window;
        Unique<Device>             m_Device;
        Unique<RenderManager>      m_RenderManager;
        Unique<Renderer>           m_Renderer;
        Unique<SceneManager>       m_SceneManager;
        Unique<EventRegistry>      m_EventRegistry;
    };

} // namespace Engine

#endif // CORE_H
