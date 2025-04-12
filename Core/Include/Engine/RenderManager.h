//
// Created by alexa on 06/04/2025.
//

#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include <memory>
#include "Engine/OffscreenRenderer.h"
#include "Engine/OnscreenRenderer.h"

namespace Engine {

class RenderManager {
public:
    RenderManager(Device& device, Window& window);
    ~RenderManager() = default;

    RenderManager(const RenderManager&) = delete;
    RenderManager& operator=(const RenderManager&) = delete;
    RenderManager(RenderManager&&) = delete;

    void Initialize();

    std::unique_ptr<OnscreenRenderer>& GetOnScreenRenderer();
    std::unique_ptr<OffscreenRenderer>& GetOffscreenRenderer();

    void UpdateRenderPasses();

private:
    // current renderers
    std::unique_ptr<OnscreenRenderer> m_OnScreenRenderer;
    std::unique_ptr<OffscreenRenderer> m_OffScreenRenderer;

    // references to the device and window
    Device& m_Device;
    Window& m_Window;
};

}

#endif //RENDERMANAGER_H


