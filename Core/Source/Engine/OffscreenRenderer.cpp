#include "Engine/OffscreenRenderer.h"

namespace Engine {
    OffscreenRenderer::OffscreenRenderer(Device& device) : m_Device(device), m_RenderpassManager(device, {800,600}) {
        CreateCommandBuffers();
        Initialize(800, 600);
    }
    OffscreenRenderer::~OffscreenRenderer() {
        FreeCommandBuffers();
    }

    void OffscreenRenderer::CreateCommandBuffers() {
        m_CommandBuffers.resize(3); // 3 frames in flight - triple buffering

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_Device.vk_GetCommandPool();
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

        if (vkAllocateCommandBuffers(m_Device.vk_GetDevice(), &allocInfo, m_CommandBuffers.data()) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    VkCommandBuffer OffscreenRenderer::GetCurrentCommandBuffer() {
        return m_CommandBuffers[m_CurrentFrameID];
    }

    void OffscreenRenderer::FreeCommandBuffers() {
        vkFreeCommandBuffers(
            m_Device.vk_GetDevice(),
            m_Device.vk_GetCommandPool(),
            static_cast<uint32_t>(m_CommandBuffers.size()),
            m_CommandBuffers.data());
        m_CommandBuffers.clear();
    }

    void OffscreenRenderer::Initialize(uint32_t width, uint32_t height) {
        m_Extent = { width, height };

        m_SharedDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(m_Device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        if ((m_HdriPass = HDRiToCubemapPass::Initialize(m_Device, {1024, 1024},m_SharedDescriptorSetLayout)))
            std::cout << "HDRi conversion pass initialized" << std::endl;

        if ((m_PreethamSkyPass = PreethamSkyPass::Initialize(m_Device, {1024, 1024}, m_SharedDescriptorSetLayout)))
            std::cout << "Preetham procedural sky pass initialized" << std::endl;

        if ((m_IrradianceSamplingPass = IrradianceSamplingPass::Initialize(m_Device, {64, 64},  m_SharedDescriptorSetLayout)))
            std::cout << "Irradiance sampling pass initialized" << std::endl;

        if ((m_HDRPrefilterPass = HDRPrefilterPass::Initialize(m_Device, {128, 128}, m_SharedDescriptorSetLayout)))
            std::cout << "HDR Prefilter pass initialized" << std::endl;

        if ((m_BRDFLutPass = LUTPass::Initialize(m_Device, {512, 512}, m_SharedDescriptorSetLayout)))
            std::cout << "2D BRDF LUT pass initialized" << std::endl;

        if ((m_GeometryPass = GeometryPass::Initialize(m_Device, m_Extent, m_SharedDescriptorSetLayout)))
            std::cout << "Geometry pass initialized" << std::endl;

        if ((m_ShadowPass = ShadowPass::Initialize(m_Device, {8192, 8192}, m_SharedDescriptorSetLayout)))
            std::cout << "Shadow pass initialized" << std::endl;

        if ((m_CompositePass = CompositePass::Initialize(m_Device, m_Extent, m_SharedDescriptorSetLayout)))
            std::cout << "Composite pass initialized" << std::endl;
    }

    void OffscreenRenderer::Resize(VkExtent2D newExtent) {
    
        // x11/wayland linux fix - throws  out of memory 
        if (newExtent.width == 0 || newExtent.height == 0) {
            return;
        }

        vkDeviceWaitIdle(m_Device.vk_GetDevice());
        m_Extent = newExtent;
        m_Resized = true;
    }

    void OffscreenRenderer::InvalidatePasses(){
        m_GeometryPass->Invalidate(m_Extent);
        m_CompositePass->Invalidate(m_Extent);
        m_Resized = false;
    }

    // TODO : this is so ugly, this needs to be reworked, or maybe even deleted, its not really used 
    void OffscreenRenderer::TransitionImages(VkCommandBuffer commandBuffer, RenderPassType src, RenderPassType dst) {
        struct TransitionInfo {
            VkImage image;
            VkImageLayout oldLayout;
            VkImageLayout newLayout;
            VkImageAspectFlags aspectMask;
            VkPipelineStageFlags srcStage;
            VkPipelineStageFlags dstStage;
            VkAccessFlags srcAccessMask;
            VkAccessFlags dstAccessMask;
            uint32_t mipLevels;
            uint32_t layerCount;
        };

        std::vector<TransitionInfo> transitions;

        if (src == RenderPassType::ENV_TO_CUBEMAP && dst == RenderPassType::IBL) {
            transitions.push_back({
                GetPassObject<HDRiToCubemapPass>()->GetFinalImage(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                1,
                1
            });
        }


        if (src == RenderPassType::GEOMETRY && dst == RenderPassType::FINAL) {
            transitions.push_back({
                GetPassObject<GeometryPass>()->GetFinalImage(),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                1,
                1
            });
        }

        if (src == RenderPassType::POST_PROCESSING && dst == RenderPassType::FINAL) {
            transitions.push_back({
                m_RenderpassManager.GetImage(src),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                1,
                1
            });
        }

        // Apply all transitions
        std::vector<VkImageMemoryBarrier> barriers;
        for (const auto& transition : transitions) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = transition.oldLayout;
            barrier.newLayout = transition.newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = transition.image;
            barrier.subresourceRange.aspectMask = transition.aspectMask;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = transition.mipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = transition.layerCount;
            barrier.srcAccessMask = transition.srcAccessMask;
            barrier.dstAccessMask = transition.dstAccessMask;

            barriers.push_back(barrier);
        }

        if (!barriers.empty()) {
            vkCmdPipelineBarrier(
                commandBuffer,
                transitions.front().srcStage,
                transitions.front().dstStage,
                0,
                0, nullptr,
                0, nullptr,
                static_cast<uint32_t>(barriers.size()), barriers.data()
            );
        }
    }

    void OffscreenRenderer::EndRenderPass(VkCommandBuffer commandBuffer) {
        vkCmdEndRenderPass(commandBuffer);
    }

    void OffscreenRenderer::PrepareSceneData(Ref<AquilaScene>& scene){

    }

    void OffscreenRenderer::Render(VkCommandBuffer cmd, Ref<AquilaScene>& scene){
        auto commandBuffer = BeginFrame();

        SendDescriptorsToGPU();



        EndFrame();
    }

    void OffscreenRenderer::SendDescriptorsToGPU(){

        // may (or may not) update every frame
        auto shadowInfo = GetImageInfoForPass<Engine::ShadowPass>(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

        if (m_HDRUpdated){
            auto irradianceInfo = GetImageInfoForPass<Engine::IrradianceSamplingPass>(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            auto prefilterInfo = GetImageInfoForPass<Engine::HDRPrefilterPass>(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            auto brdfInfo = GetImageInfoForPass<Engine::LUTPass>(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            auto cubeMapInfo = GetImageInfoForPass<Engine::HDRiToCubemapPass>(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    VkCommandBuffer OffscreenRenderer::BeginFrame() {
        AQUILA_CORE_ASSERT(!m_FrameStarted && "Frame already started!");
        m_FrameStarted = true;

        auto commandBuffer = GetCurrentCommandBuffer();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        return commandBuffer;
    }

    void OffscreenRenderer::EndFrame() {
        AQUILA_CORE_ASSERT(m_FrameStarted && "Can't end a frame that wasn't started!");

        auto commandBuffer = GetCurrentCommandBuffer();

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        // Create a semaphore for synchronization (to signal GPU that it's done with the frame)
        VkSemaphore signalSemaphore;
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(m_Device.vk_GetDevice(), &semaphoreCreateInfo, nullptr, &signalSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphore!");
        }

        // Create a fence for waiting (create it unsignaled)
        VkFence fence;
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = 0;  // Don't use the SIGNALED_BIT, so the fence starts unsignaled

        if (vkCreateFence(m_Device.vk_GetDevice(), &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fence!");
        }

        // Set up the submission info
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;  // Use the created semaphore

        // Submit the command buffer for execution (offscreen rendering)
        if (vkQueueSubmit(m_Device.vk_GetGraphicsQueue(), 1, &submitInfo, fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit command buffer!");
        }

        // Wait for the fence to signal that the GPU has completed the command buffer execution
        vkWaitForFences(m_Device.vk_GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);

        // Clean up the fence and semaphore after use
        vkDestroyFence(m_Device.vk_GetDevice(), fence, nullptr);
        vkDestroySemaphore(m_Device.vk_GetDevice(), signalSemaphore, nullptr);

        m_FrameStarted = false;
        m_CurrentFrameID = (m_CurrentFrameID + 1) % 3;
    }
}