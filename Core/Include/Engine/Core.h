#ifndef CORE_H
#define CORE_H

#include "Engine/Window.h"
#include "Engine/Device.h"


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
        std::chrono::steady_clock::time_point m_StartTime;
        std::chrono::steady_clock::time_point m_LastFrameTime;
        float m_DeltaTime = 0.0f;
    };

    class Core {
        public:

        Core() {
            OnStart();
        }

        ~Core() {
            OnEnd();
        }


        void OnStart();
        void OnUpdate();
        void OnEnd();

        static Device& GetDevice();
        static Window& GetWindow();

        private: 
        
        // Timing
        Ref<Clock> m_Clock;
        float m_DeltaTime;

        // Init device and window
        Unique<Window> m_Window;
        Unique<Device> m_Device;
    };
}

#endif