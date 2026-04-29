#include "Aquila/RHI/Vulkan/VulkanDescriptors.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

VulkanDescriptorSetLayout::Builder &VulkanDescriptorSetLayout::Builder::AddBinding(uint32 binding,
																				   VkDescriptorType descriptorType,
																				   VkShaderStageFlags stageFlags,
																				   uint32 count) {
	AQUILA_ASSERT(!m_Bindings.contains(binding), "Binding already in use");
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = descriptorType;
	layoutBinding.descriptorCount = count;
	layoutBinding.stageFlags = stageFlags;
	m_Bindings[binding] = layoutBinding;
	return *this;
}

Unique<VulkanDescriptorSetLayout> VulkanDescriptorSetLayout::Builder::Build() const {
	return CreateUnique<VulkanDescriptorSetLayout>(m_Device, m_Bindings);
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice &device,
													 std::unordered_map<uint32, VkDescriptorSetLayoutBinding> bindings)
	: m_Device(device), m_Bindings(std::move(bindings)) {
	std::vector<VkDescriptorSetLayoutBinding> flatBindings;
	flatBindings.reserve(m_Bindings.size());
	for (auto &val : m_Bindings | std::views::values) {
		flatBindings.push_back(val);
	}

	VkDescriptorSetLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.bindingCount = static_cast<uint32>(flatBindings.size());
	info.pBindings = flatBindings.data();
	AQUILA_VULKAN_CHECK(vkCreateDescriptorSetLayout(m_Device.GetDevice(), &info, nullptr, &m_DescriptorSetLayout));
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
	vkDestroyDescriptorSetLayout(m_Device.GetDevice(), m_DescriptorSetLayout, nullptr);
}

VulkanDescriptorPool::Builder &VulkanDescriptorPool::Builder::AddPoolSize(VkDescriptorType type, uint32 count) {
	m_PoolSizes.push_back({ type, count });
	return *this;
}

VulkanDescriptorPool::Builder &VulkanDescriptorPool::Builder::SetPoolFlags(VkDescriptorPoolCreateFlags flags) {
	m_PoolFlags = flags;
	return *this;
}

VulkanDescriptorPool::Builder &VulkanDescriptorPool::Builder::SetMaxSets(uint32 count) {
	m_MaxSets = count;
	return *this;
}

Unique<VulkanDescriptorPool> VulkanDescriptorPool::Builder::Build() const {
	return CreateUnique<VulkanDescriptorPool>(m_Device, m_MaxSets, m_PoolFlags, m_PoolSizes);
}

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice &device, uint32 maxSets, VkDescriptorPoolCreateFlags poolFlags,
										   const std::vector<VkDescriptorPoolSize> &poolSizes)
	: m_Device(device) {
	VkDescriptorPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.poolSizeCount = static_cast<uint32>(poolSizes.size());
	info.pPoolSizes = poolSizes.data();
	info.maxSets = maxSets;
	info.flags = poolFlags;
	AQUILA_VULKAN_CHECK(vkCreateDescriptorPool(m_Device.GetDevice(), &info, nullptr, &m_DescriptorPool));
}

VulkanDescriptorPool::~VulkanDescriptorPool() {
	vkDestroyDescriptorPool(m_Device.GetDevice(), m_DescriptorPool, nullptr);
}

bool VulkanDescriptorPool::AllocateDescriptor(VkDescriptorSetLayout layout, VkDescriptorSet &set) const {
	std::lock_guard<std::mutex> lock(m_Mutex);
	VkDescriptorSetAllocateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.descriptorPool = m_DescriptorPool;
	info.pSetLayouts = &layout;
	info.descriptorSetCount = 1;
	return vkAllocateDescriptorSets(m_Device.GetDevice(), &info, &set) == VK_SUCCESS;
}

void VulkanDescriptorPool::FreeDescriptor(VkDescriptorSet set) const {
	std::lock_guard<std::mutex> lock(m_Mutex);
	vkFreeDescriptorSets(m_Device.GetDevice(), m_DescriptorPool, 1, &set);
}

void VulkanDescriptorPool::FreeDescriptors(const std::vector<VkDescriptorSet> &sets) const {
	std::lock_guard<std::mutex> lock(m_Mutex);
	vkFreeDescriptorSets(m_Device.GetDevice(), m_DescriptorPool, static_cast<uint32>(sets.size()), sets.data());
}

void VulkanDescriptorPool::ResetPool() const {
	std::lock_guard<std::mutex> lock(m_Mutex);
	vkResetDescriptorPool(m_Device.GetDevice(), m_DescriptorPool, 0);
}

VulkanDescriptorWriter::VulkanDescriptorWriter(VulkanDescriptorSetLayout &layout, VulkanDescriptorPool &pool)
	: m_SetLayout(layout), m_Pool(pool) {}

VulkanDescriptorWriter &VulkanDescriptorWriter::WriteBuffer(uint32 binding, const VkDescriptorBufferInfo *info) {
	AQUILA_ASSERT(m_SetLayout.m_Bindings.count(binding) == 1, "Layout does not contain binding");
	AQUILA_ASSERT(info && info->buffer != VK_NULL_HANDLE, "Invalid buffer info");
	AQUILA_ASSERT(m_SetLayout.m_Bindings[binding].descriptorCount == 1, "Use WriteBufferArray for arrays");

	m_BufferInfos.push_back(*info);

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = m_SetLayout.m_Bindings[binding].descriptorType;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.pBufferInfo = nullptr;
	m_Writes.push_back(write);
	return *this;
}

VulkanDescriptorWriter &VulkanDescriptorWriter::WriteImage(uint32 binding, const VkDescriptorImageInfo *info) {
	AQUILA_ASSERT(m_SetLayout.m_Bindings.count(binding) == 1, "Layout does not contain binding");
	AQUILA_ASSERT(m_SetLayout.m_Bindings[binding].descriptorCount == 1, "Use WriteImageArray for arrays");

	m_ImageInfos.push_back(*info);

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = m_SetLayout.m_Bindings[binding].descriptorType;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.pImageInfo = nullptr;
	m_Writes.push_back(write);
	return *this;
}

VulkanDescriptorWriter &VulkanDescriptorWriter::WriteImageArray(uint32 binding,
																const std::vector<VkDescriptorImageInfo> &infos) {
	AQUILA_ASSERT(m_SetLayout.m_Bindings.count(binding) == 1, "Layout does not contain binding");

	m_ImageInfos.insert(m_ImageInfos.end(), infos.begin(), infos.end());

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = m_SetLayout.m_Bindings[binding].descriptorType;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorCount = static_cast<uint32>(infos.size());
	write.pImageInfo = nullptr;
	m_Writes.push_back(write);
	return *this;
}

void VulkanDescriptorWriter::FixupAndSubmit(VkDescriptorSet set) {
	size_t bufIdx = 0;
	size_t imgIdx = 0;
	for (auto &write : m_Writes) {
		write.dstSet = set;
		auto &binding = m_SetLayout.m_Bindings[write.dstBinding];
		if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
			binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
			binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
			binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
			write.pBufferInfo = &m_BufferInfos[bufIdx];
			bufIdx += write.descriptorCount;
		} else {
			write.pImageInfo = &m_ImageInfos[imgIdx];
			imgIdx += write.descriptorCount;
		}
	}
	vkUpdateDescriptorSets(m_Pool.m_Device.GetDevice(), static_cast<uint32>(m_Writes.size()), m_Writes.data(), 0,
						   nullptr);
}

bool VulkanDescriptorWriter::Build(VkDescriptorSet &set) {
	if (!m_Pool.AllocateDescriptor(m_SetLayout.GetDescriptorSetLayout(), set)) {
		AQUILA_LOG_ERROR("Failed to allocate descriptor set");
		return false;
	}
	FixupAndSubmit(set);
	return true;
}

void VulkanDescriptorWriter::Overwrite(const VkDescriptorSet &set) {
	FixupAndSubmit(set);
}

} // namespace Aquila::RHI
