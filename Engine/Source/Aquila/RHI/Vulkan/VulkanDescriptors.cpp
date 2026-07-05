#include "Aquila/RHI/Vulkan/VulkanDescriptors.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

VulkanDescriptorSetLayout::Builder &VulkanDescriptorSetLayout::Builder::add_binding(Uint32 binding,
																				   VkDescriptorType descriptor_type,
																				   VkShaderStageFlags stage_flags,
																				   Uint32 count) {
	AQUILA_ASSERT(!m_bindings.contains(binding), "Binding already in use");
	VkDescriptorSetLayoutBinding layout_binding{};
	layout_binding.binding = binding;
	layout_binding.descriptorType = descriptor_type;
	layout_binding.descriptorCount = count;
	layout_binding.stageFlags = stage_flags;
	m_bindings[binding] = layout_binding;
	return *this;
}

Unique<VulkanDescriptorSetLayout> VulkanDescriptorSetLayout::Builder::build() const {
	return create_unique<VulkanDescriptorSetLayout>(m_device, m_bindings);
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice &device,
													 std::unordered_map<Uint32, VkDescriptorSetLayoutBinding> bindings)
	: m_device(device), m_bindings(std::move(bindings)) {
	std::vector<VkDescriptorSetLayoutBinding> flat_bindings;
	flat_bindings.reserve(m_bindings.size());
	for (auto &val : m_bindings | std::views::values) {
		flat_bindings.push_back(val);
	}

	VkDescriptorSetLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.bindingCount = static_cast<Uint32>(flat_bindings.size());
	info.pBindings = flat_bindings.data();
	AQUILA_VULKAN_CHECK(vkCreateDescriptorSetLayout(m_device.get_device(), &info, nullptr, &m_descriptor_set_layout));
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
	vkDestroyDescriptorSetLayout(m_device.get_device(), m_descriptor_set_layout, nullptr);
}

VulkanDescriptorPool::Builder &VulkanDescriptorPool::Builder::add_pool_size(VkDescriptorType type, Uint32 count) {
	m_pool_sizes.push_back({ type, count });
	return *this;
}

VulkanDescriptorPool::Builder &VulkanDescriptorPool::Builder::set_pool_flags(VkDescriptorPoolCreateFlags flags) {
	m_pool_flags = flags;
	return *this;
}

VulkanDescriptorPool::Builder &VulkanDescriptorPool::Builder::set_max_sets(Uint32 count) {
	m_max_sets = count;
	return *this;
}

Unique<VulkanDescriptorPool> VulkanDescriptorPool::Builder::build() const {
	return create_unique<VulkanDescriptorPool>(m_device, m_max_sets, m_pool_flags, m_pool_sizes);
}

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice &device, Uint32 max_sets, VkDescriptorPoolCreateFlags pool_flags,
										   const std::vector<VkDescriptorPoolSize> &pool_sizes)
	: m_device(device) {
	VkDescriptorPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.poolSizeCount = static_cast<Uint32>(pool_sizes.size());
	info.pPoolSizes = pool_sizes.data();
	info.maxSets = max_sets;
	info.flags = pool_flags;
	AQUILA_VULKAN_CHECK(vkCreateDescriptorPool(m_device.get_device(), &info, nullptr, &m_descriptor_pool));
}

VulkanDescriptorPool::~VulkanDescriptorPool() {
	vkDestroyDescriptorPool(m_device.get_device(), m_descriptor_pool, nullptr);
}

bool VulkanDescriptorPool::allocate_descriptor(VkDescriptorSetLayout layout, VkDescriptorSet &set) const {
	std::lock_guard<std::mutex> lock(m_mutex);
	VkDescriptorSetAllocateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.descriptorPool = m_descriptor_pool;
	info.pSetLayouts = &layout;
	info.descriptorSetCount = 1;
	return vkAllocateDescriptorSets(m_device.get_device(), &info, &set) == VK_SUCCESS;
}

void VulkanDescriptorPool::free_descriptor(VkDescriptorSet set) const {
	std::lock_guard<std::mutex> lock(m_mutex);
	vkFreeDescriptorSets(m_device.get_device(), m_descriptor_pool, 1, &set);
}

void VulkanDescriptorPool::free_descriptors(const std::vector<VkDescriptorSet> &sets) const {
	std::lock_guard<std::mutex> lock(m_mutex);
	vkFreeDescriptorSets(m_device.get_device(), m_descriptor_pool, static_cast<Uint32>(sets.size()), sets.data());
}

void VulkanDescriptorPool::reset_pool() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	vkResetDescriptorPool(m_device.get_device(), m_descriptor_pool, 0);
}

VulkanDescriptorWriter::VulkanDescriptorWriter(VulkanDescriptorSetLayout &layout, VulkanDescriptorPool &pool)
	: m_set_layout(layout), m_pool(pool) {}

VulkanDescriptorWriter &VulkanDescriptorWriter::write_buffer(Uint32 binding, const VkDescriptorBufferInfo *info) {
	AQUILA_ASSERT(m_set_layout.m_bindings.count(binding) == 1, "Layout does not contain binding");
	AQUILA_ASSERT(info && info->buffer != VK_NULL_HANDLE, "Invalid buffer info");
	AQUILA_ASSERT(m_set_layout.m_bindings[binding].descriptorCount == 1, "Use WriteBufferArray for arrays");

	m_buffer_infos.push_back(*info);

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = m_set_layout.m_bindings[binding].descriptorType;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.pBufferInfo = nullptr;
	m_writes.push_back(write);
	return *this;
}

VulkanDescriptorWriter &VulkanDescriptorWriter::write_image(Uint32 binding, const VkDescriptorImageInfo *info) {
	AQUILA_ASSERT(m_set_layout.m_bindings.count(binding) == 1, "Layout does not contain binding");
	AQUILA_ASSERT(m_set_layout.m_bindings[binding].descriptorCount == 1, "Use WriteImageArray for arrays");

	m_image_infos.push_back(*info);

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = m_set_layout.m_bindings[binding].descriptorType;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.pImageInfo = nullptr;
	m_writes.push_back(write);
	return *this;
}

VulkanDescriptorWriter &VulkanDescriptorWriter::write_image_array(Uint32 binding,
																const std::vector<VkDescriptorImageInfo> &infos) {
	AQUILA_ASSERT(m_set_layout.m_bindings.count(binding) == 1, "Layout does not contain binding");

	m_image_infos.insert(m_image_infos.end(), infos.begin(), infos.end());

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = m_set_layout.m_bindings[binding].descriptorType;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorCount = static_cast<Uint32>(infos.size());
	write.pImageInfo = nullptr;
	m_writes.push_back(write);
	return *this;
}

void VulkanDescriptorWriter::fixup_and_submit(VkDescriptorSet set) {
	size_t buf_idx = 0;
	size_t img_idx = 0;
	for (auto &write : m_writes) {
		write.dstSet = set;
		auto &binding = m_set_layout.m_bindings[write.dstBinding];
		if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
			binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
			binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
			binding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
			write.pBufferInfo = &m_buffer_infos[buf_idx];
			buf_idx += write.descriptorCount;
		} else {
			write.pImageInfo = &m_image_infos[img_idx];
			img_idx += write.descriptorCount;
		}
	}
	vkUpdateDescriptorSets(m_pool.m_device.get_device(), static_cast<Uint32>(m_writes.size()), m_writes.data(), 0,
						   nullptr);
}

bool VulkanDescriptorWriter::build(VkDescriptorSet &set) {
	if (!m_pool.allocate_descriptor(m_set_layout.get_descriptor_set_layout(), set)) {
		AQUILA_LOG_ERROR("Failed to allocate descriptor set");
		return false;
	}
	fixup_and_submit(set);
	return true;
}

void VulkanDescriptorWriter::overwrite(const VkDescriptorSet &set) {
	fixup_and_submit(set);
}

} // namespace Aquila::RHI
