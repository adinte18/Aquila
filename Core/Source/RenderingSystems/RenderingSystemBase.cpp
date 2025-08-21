#include "RenderingSystems/RenderingSystemBase.h"

#include "Engine/Renderer/DescriptorAllocator.h"
#include "Engine/Renderer/Swapchain.h"
namespace Engine {

    RenderingSystemBase::RenderingSystemBase(Device& device)
        : device(device)
    {}

    RenderingSystemBase::~RenderingSystemBase() {
        if (m_PipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device.GetDevice(), m_PipelineLayout, nullptr);
        }
    }

    void RenderingSystemBase::SetUniformData(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo) {
        if (bufferInfo)
            m_UniformBuffers[binding] = *bufferInfo;
        else
            m_UniformBuffers.erase(binding);
    }

    void RenderingSystemBase::SetTexture(uint32_t binding, const VkDescriptorImageInfo* imageInfo) {
        if (imageInfo)
            m_Textures[binding] = *imageInfo;
        else
            m_Textures.erase(binding);
    }

    void RenderingSystemBase::SendDataToGPU(int frameIndex) {
        DescriptorWriter writer{*m_Layout, *DescriptorAllocator::GetSharedPool()};

        for (auto& [binding, bufInfo] : m_UniformBuffers) {
            writer.writeBuffer(binding, &bufInfo);
        }
        for (auto& [binding, imgInfo] : m_Textures) {
            writer.writeImage(binding, &imgInfo);
        }

        // Only overwrite the descriptor set for the current frame
        writer.overwrite(m_DescriptorSets[frameIndex]);
    }

    void RenderingSystemBase::AllocateDescriptorSet() {
        for (int i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++){
            DescriptorAllocator::Allocate(m_Layout->GetDescriptorSetLayout(), m_DescriptorSets[i]);
        }
    }

} // namespace Engine
