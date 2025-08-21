#ifndef AQUILA_RENDERER_H
#define AQUILA_RENDERER_H

#include "RenderingSystems/SceneRenderingSystem_new.h"

#include "Renderpasses/GeometryPass.h"

#include "Engine/Renderer/Swapchain.h"
#include "Engine/SemaphoreManager.h"
#include "Engine/Renderer/CommandBufferPool.h"

namespace Engine {
    template<class>
    inline constexpr bool always_false = false;


    enum class RenderType {
        PRESENT,
        OFFSCREEN
    };

    struct RenderPassConfig {
        RenderType type;
        Ref<GeometryPass> renderpass = nullptr;
        uint32_t mipExtent = UINT32_MAX;
        bool autoSetViewportScissor = true;
    };

    class Renderer {
    public:
        Renderer(Device& device, Window& window);
        ~Renderer();

        VkCommandBuffer BeginFrame();
        void EndFrame();
        void RenderScene();
        
        void BeginRenderPass(VkCommandBuffer cmd, const RenderPassConfig& config);
        void EndRenderPass(VkCommandBuffer cmd);
        
        void Resize(VkExtent2D newExtent);
        void InvalidatePasses();
        
        template<typename T>
        Ref<T> GetRenderingSystem() const {
            if constexpr (std::is_same_v<T, SceneRenderSystem>) {
                return m_SceneRendering;
            }
            return nullptr;
        }

        template<typename T>
        Ref<T> GetPassObject() const {
            if constexpr (std::is_same_v<T, GeometryPass>) {
                return m_GeometryPass;
            }
            return nullptr;
        }

        VkRenderPass GetImGuiRenderPass() { return m_Swapchain->GetRenderpass(); }

    private:
        void Initialize(uint32_t width, uint32_t height);
        void InvalidateSwapchain();
        void SetupCommandBuffers();
        void SetupSynchronization();

        Device& m_Device;
        Window& m_Window;
        
        Unique<CommandBufferPool> m_CommandPool;
        Unique<SemaphoreManager> m_SemaphoreManager;
        
        std::vector<Unique<CommandBuffer>> m_PresentCommandBuffers;
        std::vector<Unique<CommandBuffer>> m_OffscreenCommandBuffers;
        
        Unique<Swapchain> m_Swapchain;
        Ref<DescriptorSetLayout> m_SharedDescriptorSetLayout;
        Ref<GeometryPass> m_GeometryPass;
        Ref<SceneRenderSystem> m_SceneRendering;
        
        VkExtent2D m_Extent{};
        uint32_t m_CurrentFrameID = 0;
        uint32_t m_CurrentImageID = 0;
        bool m_FrameStarted = false;
        bool m_RenderPassActive = false;
        bool m_Resized = false;
        RenderType m_CurrentRenderType = RenderType::PRESENT;
    };
}

#endif // AQUILA_RENDERER_H