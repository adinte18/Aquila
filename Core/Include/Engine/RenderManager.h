//
// Created by alexa on 06/04/2025.
//

#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

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

    Unique<OnscreenRenderer>& GetOnScreenRenderer();
    Unique<OffscreenRenderer>& GetOffscreenRenderer();

    VkRenderPass GetImGuiRenderPass() const;

    void UpdateRenderPasses();

private:
    // current renderers
    Unique<OnscreenRenderer> m_OnScreenRenderer;
    Unique<OffscreenRenderer> m_OffScreenRenderer;

    // references to the device and window
    Device& m_Device;
    Window& m_Window;
};

}

#endif //RENDERMANAGER_H


