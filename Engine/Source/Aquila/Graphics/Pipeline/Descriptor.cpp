#include "Aquila/Graphics/Pipeline/Descriptor.h"

#include "Aquila/Graphics/Pipeline/DescriptorAllocator.h"

namespace Aquila::Graphics::RenderingPipeline {

DescriptorSetLayout::Builder &DescriptorSetLayout::Builder::AddBinding(uint32 binding, VkDescriptorType descriptorType,
																	   VkShaderStageFlags stageFlags, uint32 count) {
	AQUILA_ASSERT(!m_Bindings.contains(binding), "Binding already in use");
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = descriptorType;
	layoutBinding.descriptorCount = count;
	layoutBinding.stageFlags = stageFlags;
	m_Bindings[binding] = layoutBinding;
	return *this;
}

Unique<DescriptorSetLayout> DescriptorSetLayout::Builder::Build() const {
	return CreateUnique<DescriptorSetLayout>(m_Device, m_Bindings);
}

DescriptorSetLayout::DescriptorSetLayout(Device &device,
										 std::unordered_map<uint32, VkDescriptorSetLayoutBinding> bindings)
	: m_Device{ device }, m_Bindings{ std::move(bindings) } {
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
	setLayoutBindings.reserve(m_Bindings.size());
	for (auto val : m_Bindings | std::views::values) {
		setLayoutBindings.push_back(val);
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32>(setLayoutBindings.size());
	descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

	AQUILA_VULKAN_CHECK(
		vkCreateDescriptorSetLayout(m_Device.GetDevice(), &descriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout));
}

DescriptorSetLayout::~DescriptorSetLayout() {
	vkDestroyDescriptorSetLayout(m_Device.GetDevice(), m_DescriptorSetLayout, nullptr);
}

DescriptorPool::Builder &DescriptorPool::Builder::AddPoolSize(VkDescriptorType descriptorType, uint32 count) {
	m_PoolSizes.push_back({ descriptorType, count });
	return *this;
}

DescriptorPool::Builder &DescriptorPool::Builder::SetPoolFlags(VkDescriptorPoolCreateFlags flags) {
	m_PoolFlags = flags;
	return *this;
}

DescriptorPool::Builder &DescriptorPool::Builder::SetMaxSets(uint32 count) {
	m_MaxSets = count;
	return *this;
}

Unique<DescriptorPool> DescriptorPool::Builder::Build() const {
	return CreateUnique<DescriptorPool>(m_Device, m_MaxSets, m_PoolFlags, m_PoolSizes);
}

DescriptorPool::DescriptorPool(Device &device, const uint32 maxSets, const VkDescriptorPoolCreateFlags poolFlags,
							   const std::vector<VkDescriptorPoolSize> &poolSizes)
	: m_Device{ device } {
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32>(poolSizes.size());
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = maxSets;
	descriptorPoolInfo.flags = poolFlags;

	AQUILA_VULKAN_CHECK(vkCreateDescriptorPool(m_Device.GetDevice(), &descriptorPoolInfo, nullptr, &m_DescriptorPool));
}

DescriptorPool::~DescriptorPool() {
	vkDestroyDescriptorPool(m_Device.GetDevice(), m_DescriptorPool, nullptr);
}

bool DescriptorPool::AllocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout,
										VkDescriptorSet &descriptor) const {
	// Lock the pool mutex to prevent concurrent allocations
	std::lock_guard<std::mutex> lock(m_Mutex);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	return vkAllocateDescriptorSets(m_Device.GetDevice(), &allocInfo, &descriptor) == VK_SUCCESS;
}

void DescriptorPool::FreeDescriptors(const std::vector<VkDescriptorSet> &descriptors) const {
	std::lock_guard<std::mutex> lock(m_Mutex);

	vkFreeDescriptorSets(m_Device.GetDevice(), m_DescriptorPool, static_cast<uint32>(descriptors.size()),
						 descriptors.data());
}

void DescriptorPool::FreeDescriptor(const VkDescriptorSet &descriptor) const {
	std::lock_guard<std::mutex> lock(m_Mutex);

	vkFreeDescriptorSets(m_Device.GetDevice(), m_DescriptorPool, 1, &descriptor);
}

void DescriptorPool::ResetPool() const {
	std::lock_guard<std::mutex> lock(m_Mutex);

	vkResetDescriptorPool(m_Device.GetDevice(), m_DescriptorPool, 0);
}
DescriptorWriter::DescriptorWriter(DescriptorSetLayout &setLayout, DescriptorPool &pool)
	: m_SetLayout{ setLayout }, m_Pool{ pool } {}

DescriptorWriter &DescriptorWriter::WriteBuffer(uint32 binding, const VkDescriptorBufferInfo *bufferInfo) {
	AQUILA_ASSERT(m_SetLayout.m_Bindings.count(binding) == 1, "Layout does not contain specified binding");
	AQUILA_ASSERT(bufferInfo != nullptr, "bufferInfo must not be null");
	AQUILA_ASSERT(bufferInfo->buffer != VK_NULL_HANDLE,
				  "WriteBuffer: VkBuffer is VK_NULL_HANDLE — UBO not yet created?");

	auto &bindingDescription = m_SetLayout.m_Bindings[binding];
	AQUILA_ASSERT(bindingDescription.descriptorCount == 1,
				  "Binding single descriptor info, but binding expects multiple");

	m_BufferInfos.push_back(*bufferInfo);

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.pBufferInfo = nullptr;
	m_BufferBindingIndices.push_back(static_cast<uint32>(m_BufferInfos.size() - 1));
	m_WriteIsBuffer.push_back(true);

	m_Writes.push_back(write);
	return *this;
}

DescriptorWriter &DescriptorWriter::WriteImage(uint32 binding, const VkDescriptorImageInfo *imageInfo) {
	AQUILA_ASSERT(m_SetLayout.m_Bindings.count(binding) == 1, "Layout does not contain specified binding");
	auto &bindingDescription = m_SetLayout.m_Bindings[binding];
	AQUILA_ASSERT(bindingDescription.descriptorCount == 1,
				  "Binding single descriptor info, but binding expects multiple");

	m_ImageInfos.push_back(*imageInfo); // copy, not pointer

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.pImageInfo = nullptr;
	m_BufferBindingIndices.push_back(static_cast<uint32>(m_ImageInfos.size() - 1));
	m_WriteIsBuffer.push_back(false);

	m_Writes.push_back(write);
	return *this;
}

DescriptorWriter &DescriptorWriter::WriteImageArray(uint32 binding,
													const std::vector<VkDescriptorImageInfo> &imageInfos) {
	AQUILA_ASSERT(m_SetLayout.m_Bindings.count(binding) == 1, "Layout does not contain specified binding");

	auto &bindingDescription = m_SetLayout.m_Bindings[binding];

	m_ImageInfos.insert(m_ImageInfos.end(), imageInfos.begin(), imageInfos.end());

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.pImageInfo = &m_ImageInfos[m_ImageInfos.size() - imageInfos.size()];
	write.descriptorCount = static_cast<uint32>(imageInfos.size());

	m_Writes.push_back(write);
	return *this;
}

void DescriptorWriter::FixupPointers() {
	// TODO : I dont like this but it does the job for now
	AQUILA_ASSERT(m_Writes.size() == m_WriteIsBuffer.size(), "Write metadata mismatch");
	for (size_t i = 0; i < m_Writes.size(); ++i) {
		auto &write = m_Writes[i];
		uint32 idx = m_BufferBindingIndices[i];
		if (m_WriteIsBuffer[i]) {
			write.pBufferInfo = &m_BufferInfos[idx];
			write.pImageInfo = nullptr;
		} else {
			write.pImageInfo = &m_ImageInfos[idx];
			write.pBufferInfo = nullptr;
		}
	}
}

bool DescriptorWriter::Build(VkDescriptorSet &set) {
	std::lock_guard<std::mutex> lock(DescriptorAllocator::GetPoolMutex());

	bool success = m_Pool.AllocateDescriptor(m_SetLayout.GetDescriptorSetLayout(), set);
	if (!success) {
		AQUILA_LOG_ERROR("Failed to allocate descriptor set");
		return false;
	}

	FixupPointers();

	for (auto &write : m_Writes) {
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(m_Pool.m_Device.GetDevice(), static_cast<uint32>(m_Writes.size()), m_Writes.data(), 0,
						   nullptr);
	return true;
}

void DescriptorWriter::Overwrite(const VkDescriptorSet &set) {
	std::lock_guard<std::mutex> lock(DescriptorAllocator::GetPoolMutex());

	FixupPointers();

	for (auto &write : m_Writes) {
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(m_Pool.m_Device.GetDevice(), static_cast<uint32>(m_Writes.size()), m_Writes.data(), 0,
						   nullptr);
}
} // namespace Aquila::Graphics::RenderingPipeline
