#ifndef AQUILA_VULKAN_DESCRIPTORS_H
#define AQUILA_VULKAN_DESCRIPTORS_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHIDescriptors.h"

namespace Aquila::RHI {

class VulkanDevice;

class VulkanDescriptorSetLayout final : public IRHIDescriptorSetLayout {
  public:
	class Builder {
	  public:
		Builder(VulkanDevice &device) : m_Device(device) {}
		Builder &AddBinding(uint32 binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags,
							uint32 count = 1);
		Unique<VulkanDescriptorSetLayout> Build() const;

	  private:
		VulkanDevice &m_Device;
		std::unordered_map<uint32, VkDescriptorSetLayoutBinding> m_Bindings{};
	};

	VulkanDescriptorSetLayout(VulkanDevice &device, std::unordered_map<uint32, VkDescriptorSetLayoutBinding> bindings);
	~VulkanDescriptorSetLayout() override;
	AQUILA_NONCOPYABLE(VulkanDescriptorSetLayout);

	[[nodiscard]] uint32 GetBindingCount() const override { return static_cast<uint32>(m_Bindings.size()); }

	[[nodiscard]] VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
	[[nodiscard]] const std::unordered_map<uint32, VkDescriptorSetLayoutBinding> &GetBindings() const {
		return m_Bindings;
	}

  private:
	VulkanDevice &m_Device;
	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	std::unordered_map<uint32, VkDescriptorSetLayoutBinding> m_Bindings;
	friend class VulkanDescriptorWriter;
};

class VulkanDescriptorPool {
  public:
	class Builder {
	  public:
		Builder(VulkanDevice &device) : m_Device(device) {}
		Builder &AddPoolSize(VkDescriptorType descriptorType, uint32 count);
		Builder &SetPoolFlags(VkDescriptorPoolCreateFlags flags);
		Builder &SetMaxSets(uint32 count);
		[[nodiscard]] Unique<VulkanDescriptorPool> Build() const;

	  private:
		VulkanDevice &m_Device;
		std::vector<VkDescriptorPoolSize> m_PoolSizes{};
		uint32 m_MaxSets = 1000;
		VkDescriptorPoolCreateFlags m_PoolFlags = 0;
	};

	VulkanDescriptorPool(VulkanDevice &device, uint32 maxSets, VkDescriptorPoolCreateFlags poolFlags,
						 const std::vector<VkDescriptorPoolSize> &poolSizes);
	~VulkanDescriptorPool();
	AQUILA_NONCOPYABLE(VulkanDescriptorPool);

	bool AllocateDescriptor(VkDescriptorSetLayout layout, VkDescriptorSet &set) const;
	void FreeDescriptor(VkDescriptorSet set) const;
	void FreeDescriptors(const std::vector<VkDescriptorSet> &sets) const;
	void ResetPool() const;

	[[nodiscard]] VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }

  private:
	VulkanDevice &m_Device;
	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	mutable std::mutex m_Mutex;
	friend class VulkanDescriptorWriter;
};

class VulkanDescriptorWriter {
  public:
	VulkanDescriptorWriter(VulkanDescriptorSetLayout &layout, VulkanDescriptorPool &pool);

	VulkanDescriptorWriter &WriteBuffer(uint32 binding, const VkDescriptorBufferInfo *info);
	VulkanDescriptorWriter &WriteImage(uint32 binding, const VkDescriptorImageInfo *info);
	VulkanDescriptorWriter &WriteImageArray(uint32 binding, const std::vector<VkDescriptorImageInfo> &infos);

	bool Build(VkDescriptorSet &set);
	void Overwrite(const VkDescriptorSet &set);

  private:
	void FixupAndSubmit(VkDescriptorSet set);

	VulkanDescriptorSetLayout &m_SetLayout;
	VulkanDescriptorPool &m_Pool;
	std::vector<VkWriteDescriptorSet> m_Writes;
	std::vector<VkDescriptorBufferInfo> m_BufferInfos;
	std::vector<VkDescriptorImageInfo> m_ImageInfos;
};

} // namespace Aquila::RHI
#endif
