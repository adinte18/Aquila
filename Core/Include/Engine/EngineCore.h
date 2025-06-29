#ifndef AQUILA_APP_H
#define AQUILA_APP_H

#include "Engine/Device.h"
#include "Engine/Window.h"
#include "Engine/DescriptorAllocator.h"
#include "Engine/RenderManager.h"

namespace Engine {
    class EngineCore {
        public:
        static EngineCore& Get() {
            static EngineCore instance;
            return instance;
        }

        void OnStart();
        void OnUpdate();
        void OnEnd();

        // no copy constructors
        EngineCore(const EngineCore&) = delete;
        EngineCore& operator=(const EngineCore&) = delete;

        EngineCore(const EngineCore&&) noexcept = delete;
        EngineCore&& operator=(const EngineCore&&) noexcept = delete;

        private:
        EngineCore() = default;
        ~EngineCore() = default;

        Unique<Device> m_Device;
        Unique<Window> m_Window;
        Unique<RenderManager> m_RenderManager;
    };
};

#endif