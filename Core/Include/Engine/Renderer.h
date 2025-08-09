#ifndef AQUILA_RENDERER_H
#define AQUILA_RENDERER_H

#include "RenderingSystems/SceneRenderingSystem_new.h"
#include "Renderpasses/GeometryPass.h"
#include "Renderpasses/HDRiToCubemapPass.h"
#include "Renderpasses/IrradianceSamplingPass.h"
#include "Renderpasses/HDRPrefilterPass.h"
#include "Renderpasses/LUTPass.h"
#include "Renderpasses/ShadowPass.h"
#include "Renderpasses/PreethamSkyPass.h"

#include "Engine/Swapchain.h"


namespace Engine {
    template<class>
    inline constexpr bool always_false = false;

    class Renderer {

    public:
        Renderer(Device& device, Window& window);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        void Initialize(uint32_t width, uint32_t height);

        VkCommandBuffer BeginFrame();
        void EndFrame();

        void BeginSwapchainRenderPass(VkCommandBuffer commandBuffer);

        template <typename T>
        auto GetImageInfoForPass(VkImageLayout layout) {
            return GetPassObject<T>()->GetImageInfo(layout);
        }

        template <typename T>
        [[nodiscard]] Ref<T> GetRenderingSystem() {
            if constexpr (std::is_same_v<T, SceneRenderSystem>) {
                return m_SceneRendering;
            } else {
                AQUILA_STATIC_ASSERT(always_false<T>, "Unsupported rendering system type.");
                return nullptr;
            }
        }

        template <typename T>
        [[nodiscard]] Ref<T> GetPassObject() const {
            if constexpr (std::is_same_v<T, GeometryPass>) {
                return m_GeometryPass;
            } else {
                AQUILA_STATIC_ASSERT(always_false<T>, "Unsupported render pass type.");
                return nullptr;
            }
        }
        template <typename T>
        [[nodiscard]] Ref<DescriptorSetLayout>& GetPassLayout() const {
            if constexpr (std::is_same_v<T, GeometryPass>) {
                return m_GeometryPass->GetDescriptorSetLayout();
            } 
            else {
                AQUILA_STATIC_ASSERT(always_false<T>, "Unsupported render pass type.");
            }
        }


        template<typename T>
        void BeginOffscreenRenderPass(VkCommandBuffer cmd, int cubemapFace = -1, uint32_t mipExtent = UINT32_MAX) {
            auto pass = GetPassObject<T>();

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = pass->GetRenderPass();

            if constexpr (std::is_same_v<T, HDRiToCubemapPass> || std::is_same_v<T, IrradianceSamplingPass> || std::is_same_v<T, HDRPrefilterPass> || std::is_same_v<T, PreethamSkyPass>) {
                renderPassInfo.framebuffer = pass->GetFramebuffer(cubemapFace);
            } else {
                renderPassInfo.framebuffer = pass->GetFramebuffer()->GetHandle();
            }

            renderPassInfo.renderArea.offset = {0, 0};
            if (mipExtent != UINT32_MAX) {
                renderPassInfo.renderArea.extent = {mipExtent, mipExtent};
            } else {
                renderPassInfo.renderArea.extent = pass->GetExtent();
            }

            const auto& clearValues = pass->GetClearValues();
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Viewport and scissor
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{};
            scissor.offset = {0, 0};

            if (mipExtent != UINT32_MAX) {
                viewport.width  = static_cast<float>(mipExtent);
                viewport.height = static_cast<float>(mipExtent);
                scissor.extent = {mipExtent, mipExtent};
            } else {
                viewport.width  = static_cast<float>(pass->GetExtent().width);
                viewport.height = static_cast<float>(pass->GetExtent().height);
                scissor.extent = pass->GetExtent();
            }

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);
        }
        
        void EndRenderPass(VkCommandBuffer commandBuffer);

        void Resize(VkExtent2D newExtent);
        bool Resized() const { return m_Resized; }

        void InvalidatePasses();
        void InvalidateSwapchain();

        [[nodiscard]] VkExtent2D GetExtent() const { return m_Extent; }
        [[nodiscard]] float GetAspectRatio() const { return static_cast<float>(m_Extent.width) / static_cast<float>(m_Extent.height); }   
        [[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer() const { return m_CommandBuffers[m_CurrentFrameID]; }
        [[nodiscard]] VkRenderPass GetImGuiRenderPass() const { return m_Swapchain->GetRenderpass(); }

    private:
        Device& m_Device;
        Window& m_Window;

        Unique<Swapchain> m_Swapchain; // editor will use it to render ImGui to the swapchain
        Ref<DescriptorSetLayout> m_SharedDescriptorSetLayout{};
        VkExtent2D m_Extent{};

        // Passes
        Ref<GeometryPass> m_GeometryPass;

        // Systems
        Ref<SceneRenderSystem> m_SceneRendering;

        bool m_Resized{false};

        // Frame management
        uint32_t m_CurrentImageID{0};
        bool m_FrameStarted{false};
        int m_CurrentFrameID{0};
        std::vector<VkCommandBuffer> m_CommandBuffers;


        void CreateCommandBuffers();
        void FreeCommandBuffers();
    };
}

#endif // AQUILA_RENDERER_H