#include "Aquila/RHI/Vulkan/VulkanDescriptorSet.h"
#include "Aquila/RHI/Vulkan/VulkanBuffer.h"
#include "Aquila/RHI/Vulkan/VulkanDescriptors.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"
#include "Aquila/RHI/Vulkan/VulkanTexture.h"

namespace Aquila::RHI {

VulkanDescriptorSet::VulkanDescriptorSet(VulkanDevice &device, VkDescriptorSet set, VulkanDescriptorSetLayout &layout,
										 VulkanDescriptorPool &pool)
	: m_Device(device), m_Set(set), m_Layout(layout), m_Pool(pool) {}

VulkanDescriptorSet::~VulkanDescriptorSet() {
	if (m_Set != VK_NULL_HANDLE) {
		m_Pool.FreeDescriptor(m_Set);
	}
}

void VulkanDescriptorSet::SetBuffer(uint32 binding, IRHIBuffer &buffer, uint64 offset, uint64 range) {
	auto &vkBuf = static_cast<VulkanBuffer &>(buffer);

	m_BufferInfos.push_back({
		.buffer = vkBuf.GetBuffer(),
		.offset = offset,
		.range = range == 0 ? VK_WHOLE_SIZE : range,
	});

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_Set;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;

	const auto &bindings = m_Layout.GetBindings();
	auto it = bindings.find(binding);
	write.descriptorType = (it != bindings.end()) ? it->second.descriptorType : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.pBufferInfo = nullptr; // fixed up in Flush()

	m_PendingWrites.push_back(write);
}

void VulkanDescriptorSet::SetTexture(uint32 binding, IRHITexture &texture) {
	auto &vkTex = static_cast<VulkanTexture &>(texture);

	VkSampler sampler = m_Device.GetOrCreateSampler(vkTex.GetDesc().sampler);
	m_ImageInfos.push_back({
		.sampler = sampler,
		.imageView = vkTex.GetImageView(),
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	});

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_Set;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.pImageInfo = nullptr; // fixed up in Flush()

	m_PendingWrites.push_back(write);
}

void VulkanDescriptorSet::Flush() {
	if (m_PendingWrites.empty()) {
		return;
	}

	uint32 bufferIdx = 0;
	uint32 imageIdx = 0;

	for (auto &write : m_PendingWrites) {
		if (write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
			write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
			write.pBufferInfo = &m_BufferInfos[bufferIdx++];
		} else {
			write.pImageInfo = &m_ImageInfos[imageIdx++];
		}
	}

	vkUpdateDescriptorSets(m_Device.GetDevice(), static_cast<uint32>(m_PendingWrites.size()), m_PendingWrites.data(), 0,
						   nullptr);

	m_PendingWrites.clear();
	m_BufferInfos.clear();
	m_ImageInfos.clear();
}

} // namespace Aquila::RHI
