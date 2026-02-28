#ifndef VK_DESCRIPTOR_H
#define VK_DESCRIPTOR_H

#include "Aquila/Graphics/Core/Device.h"

namespace Aquila::Graphics::RenderingPipeline {
class DescriptorSetLayout {
  public:
	class Builder {
	  public:
		Builder(Device &device) : m_Device{ device } {}

		Builder &AddBinding(uint32 binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags,
							uint32 count = 1);
		Unique<DescriptorSetLayout> Build() const;

	  private:
		Device &m_Device;
		std::unordered_map<uint32, VkDescriptorSetLayoutBinding> m_Bindings{};
	};

	DescriptorSetLayout(Device &device, std::unordered_map<uint32, VkDescriptorSetLayoutBinding> bindings);
	~DescriptorSetLayout();
	DescriptorSetLayout(const DescriptorSetLayout &) = delete;
	DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;
	const std::unordered_map<uint32, VkDescriptorSetLayoutBinding> &GetBindings() const { return m_Bindings; }

	[[nodiscard]] VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

  private:
	Device &m_Device;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	std::unordered_map<uint32, VkDescriptorSetLayoutBinding> m_Bindings;

	friend class DescriptorWriter;
};

class DescriptorPool {
  public:
	class Builder {
	  public:
		Builder(Device &device) : m_Device{ device } {}

		Builder &AddPoolSize(VkDescriptorType descriptorType, uint32 count);
		Builder &SetPoolFlags(VkDescriptorPoolCreateFlags flags);
		Builder &SetMaxSets(uint32 count);
		Unique<DescriptorPool> Build() const;

	  private:
		Device &m_Device;
		std::vector<VkDescriptorPoolSize> m_PoolSizes{};
		uint32 m_MaxSets = 1000;
		VkDescriptorPoolCreateFlags m_PoolFlags = 0;
	};

	DescriptorPool(Device &device, uint32 maxSets, VkDescriptorPoolCreateFlags poolFlags,
				   const std::vector<VkDescriptorPoolSize> &poolSizes);
	~DescriptorPool();
	DescriptorPool(const DescriptorPool &) = delete;
	DescriptorPool &operator=(const DescriptorPool &) = delete;

	bool AllocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;

	void FreeDescriptors(const std::vector<VkDescriptorSet> &descriptors) const;
	void FreeDescriptor(const VkDescriptorSet &descriptor) const;

	VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }

	void ResetPool() const;

  private:
	Device &m_Device;
	VkDescriptorPool m_DescriptorPool;
	mutable std::mutex m_Mutex;

	friend class DescriptorWriter;
};

class DescriptorWriter {
  public:
	DescriptorWriter(DescriptorSetLayout &setLayout, DescriptorPool &pool);

	DescriptorWriter &WriteBuffer(uint32 binding, const VkDescriptorBufferInfo *bufferInfo);
	DescriptorWriter &WriteImage(uint32 binding, const VkDescriptorImageInfo *imageInfo);
	DescriptorWriter &WriteImageArray(uint32 binding, const std::vector<VkDescriptorImageInfo> &imageInfos);

	/**
	 * Build a new descriptor set
	 */
	bool Build(VkDescriptorSet &set);

	/**
	 * Update an existing descriptor set
	 */
	void Overwrite(const VkDescriptorSet &set);

  private:
	DescriptorSetLayout &m_SetLayout;
	DescriptorPool &m_Pool;

	std::vector<VkWriteDescriptorSet> m_Writes;
	std::vector<VkDescriptorBufferInfo> m_BufferInfos;
	std::vector<VkDescriptorImageInfo> m_ImageInfos;

	std::vector<uint32> m_BufferBindingIndices; // parallel to m_Writes
	std::vector<bool> m_WriteIsBuffer;			// parallel to m_Writes, true=buffer, false=image

	void FixupPointers();
};

} // namespace Aquila::Graphics::RenderingPipeline

#endif
