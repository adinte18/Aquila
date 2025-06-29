//
// Created by alexa on 06/04/2025.
//

#include "Engine/RenderManager.h"

namespace Engine {

    RenderManager::RenderManager(Device& device, Window& window)
        : m_Device(device), m_Window(window) {
        m_OnScreenRenderer = std::make_unique<OnscreenRenderer>(m_Window, m_Device);
        m_OffScreenRenderer = std::make_unique<OffscreenRenderer>(m_Device);
    }

    Unique<OnscreenRenderer>& RenderManager::GetOnScreenRenderer() {
        return m_OnScreenRenderer;
    }

    Unique<OffscreenRenderer>& RenderManager::GetOffscreenRenderer() {
        return m_OffScreenRenderer;
    }

    VkRenderPass RenderManager::GetImGuiRenderPass() const{
        return m_OnScreenRenderer->vk_GetCurrentRenderPass();
    }
} // Engine