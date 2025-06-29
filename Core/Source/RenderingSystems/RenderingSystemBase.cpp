#include "RenderingSystems/RenderingSystemBase.h"

namespace Engine {

RenderingSystemBase::RenderingSystemBase(Device& device)
    : device(device)
{}

RenderingSystemBase::~RenderingSystemBase() {
    if (m_PipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device.vk_GetDevice(), m_PipelineLayout, nullptr);
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

void RenderingSystemBase::SendDataToGPU() {
    DescriptorWriter writer{*m_Layout, *DescriptorAllocator::GetSharedPool()};

    for (auto& [binding, bufInfo] : m_UniformBuffers) {
        writer.writeBuffer(binding, &bufInfo);
    }
    for (auto& [binding, imgInfo] : m_Textures) {
        writer.writeImage(binding, &imgInfo);
    }

    writer.overwrite(m_DescriptorSet);
}

void RenderingSystemBase::AllocateDescriptorSet() {
    DescriptorAllocator::Allocate(m_Layout->GetDescriptorSetLayout(), m_DescriptorSet);
}

} // namespace Engine
