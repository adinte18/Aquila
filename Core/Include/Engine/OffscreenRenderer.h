//
// Created by alexa on 19/10/2024.
//

#ifndef OFFSCREENRENDERER_H
#define OFFSCREENRENDERER_H

#include "Engine/Device.h"
#include "Engine/RenderpassManager.h"
#include "Scene/Scene.h"

#include "Asserts.h"
#include "Descriptor.h"
#include "Renderpasses/CompositePass.h"
#include "Renderpasses/GeometryPass.h"
#include "Renderpasses/HDRiToCubemapPass.h"
#include "Renderpasses/HDRPrefilterPass.h"
#include "Renderpasses/IrradianceSamplingPass.h"
#include "Renderpasses/LUTPass.h"
#include "Renderpasses/ShadowPass.h"
#include "Renderpasses/PreethamSkyPass.h"


template<class>
inline constexpr bool always_false = false;

namespace Engine {
    class OffscreenRenderer {

    public:
        explicit OffscreenRenderer(Device& device);
        ~OffscreenRenderer();

        OffscreenRenderer(const OffscreenRenderer&) = delete;
        OffscreenRenderer& operator=(const OffscreenRenderer&) = delete;

        void Initialize(uint32_t width, uint32_t height);
        void Resize(VkExtent2D newExtent);
        void InvalidatePasses();

        void TransitionImages(VkCommandBuffer commandBuffer, RenderPassType src, RenderPassType dst);

        VkCommandBuffer BeginFrame();
        void EndFrame();

        void PrepareSceneData(Ref<AquilaScene>& scene);
        void Render(VkCommandBuffer cmd, Ref<AquilaScene>& scene);
        void SendDescriptorsToGPU();

        [[nodiscard]] float GetAspectRatio() const { return static_cast<float>(m_Extent.width) / static_cast<float>(m_Extent.height); }
        [[nodiscard]] DescriptorSetLayout& GetSharedDescriptorSetLayout() const { return *m_SharedDescriptorSetLayout; }
        [[nodiscard]] bool Resized() const {return m_Resized;}

        template <typename T>
        auto GetImageInfoForPass(VkImageLayout layout) {
            return GetPassObject<T>()->GetImageInfo(layout);
        }

        template <typename T>
        [[nodiscard]] Ref<T> GetPassObject() const {
            if constexpr (std::is_same_v<T, HDRiToCubemapPass>) {
                return m_HdriPass;
            } else if constexpr (std::is_same_v<T, GeometryPass>) {
                return m_GeometryPass;
            } else if constexpr (std::is_same_v<T, IrradianceSamplingPass>) {
                return m_IrradianceSamplingPass;
            } else if constexpr (std::is_same_v<T, ShadowPass>) {
                return m_ShadowPass;
            } else if constexpr (std::is_same_v<T, CompositePass>) {
                return m_CompositePass;
            } else if constexpr (std::is_same_v<T, HDRPrefilterPass>) {
                return m_HDRPrefilterPass;
            } else if constexpr (std::is_same_v<T, LUTPass>) {
                return m_BRDFLutPass;
            } else if constexpr (std::is_same_v<T, PreethamSkyPass>) {
                return m_PreethamSkyPass;
            }
            else {
                AQUILA_STATIC_ASSERT(always_false<T>, "Unsupported render pass type.");
                return nullptr;
            }
        }
        template <typename T>
        [[nodiscard]] Ref<DescriptorSetLayout>& GetPassLayout() const {
            if constexpr (std::is_same_v<T, HDRiToCubemapPass>) {
                return m_HdriPass->GetDescriptorSetLayout();
            } else if constexpr (std::is_same_v<T, GeometryPass>) {
                return m_GeometryPass->GetDescriptorSetLayout();
            } else if constexpr (std::is_same_v<T, IrradianceSamplingPass>) {
                return m_IrradianceSamplingPass->GetDescriptorSetLayout();
            } else if constexpr (std::is_same_v<T, ShadowPass>) {
                return m_ShadowPass->GetDescriptorSetLayout();
            } else if constexpr (std::is_same_v<T, CompositePass>) {
                return m_CompositePass->GetDescriptorSetLayout();
            } else if constexpr (std::is_same_v<T, LUTPass>) {
                return m_BRDFLutPass->GetDescriptorSetLayout();
            } else if constexpr (std::is_same_v<T, HDRPrefilterPass>) {
                return m_HDRPrefilterPass->GetDescriptorSetLayout();
            } else if constexpr (std::is_same_v<T, PreethamSkyPass>) {
                return m_PreethamSkyPass->GetDescriptorSetLayout();
            }
            else {
                AQUILA_STATIC_ASSERT(always_false<T>, "Unsupported render pass type.");
            }
        }

        template<typename T>
        void BeginRenderPass(VkCommandBuffer cmd, const Ref<T>& pass, int cubemapFace = -1, uint32_t mipExtent = UINT32_MAX) {
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = pass->GetRenderPass();

            if constexpr (std::is_same_v<T, HDRiToCubemapPass> || std::is_same_v<T, IrradianceSamplingPass> || std::is_same_v<T, HDRPrefilterPass> || std::is_same_v<T, PreethamSkyPass>) {
                renderPassInfo.framebuffer = pass->GetFramebuffer(cubemapFace);
            } else {
                renderPassInfo.framebuffer = pass->GetFramebuffer();
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

        private:
        Device& m_Device;
        VkExtent2D m_Extent{};
        RenderpassManager m_RenderpassManager;
        std::vector<VkCommandBuffer> m_CommandBuffers;

        bool m_Resized{false};

        Ref<HDRiToCubemapPass> m_HdriPass;
        Ref<GeometryPass> m_GeometryPass;
        Ref<IrradianceSamplingPass> m_IrradianceSamplingPass;
        Ref<ShadowPass> m_ShadowPass;
        Ref<CompositePass> m_CompositePass;
        Ref<HDRPrefilterPass> m_HDRPrefilterPass;
        Ref<LUTPass> m_BRDFLutPass;
        Ref<PreethamSkyPass> m_PreethamSkyPass;

        Ref<Engine::DescriptorSetLayout> m_SharedDescriptorSetLayout{};
        bool m_HDRUpdated{false};


        uint32_t m_CurrentImageID{0};
        bool m_FrameStarted{false};
        int m_CurrentFrameID{0};

        void CreateCommandBuffers();
        VkCommandBuffer GetCurrentCommandBuffer();
        void FreeCommandBuffers();

    };
}

#endif //OFFSCREENRENDERER_H
