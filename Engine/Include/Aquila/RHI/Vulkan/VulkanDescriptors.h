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
		Builder(VulkanDevice &device) : m_device(device) {}
		Builder &add_binding(Uint32 binding, VkDescriptorType descriptor_type, VkShaderStageFlags stage_flags,
							Uint32 count = 1);
		Unique<VulkanDescriptorSetLayout> build() const;

	  private:
		VulkanDevice &m_device;
		std::unordered_map<Uint32, VkDescriptorSetLayoutBinding> m_bindings{};
	};

	VulkanDescriptorSetLayout(VulkanDevice &device, std::unordered_map<Uint32, VkDescriptorSetLayoutBinding> bindings);
	~VulkanDescriptorSetLayout() override;
	AQUILA_NONCOPYABLE(VulkanDescriptorSetLayout);

	[[nodiscard]] Uint32 get_binding_count() const override { return static_cast<Uint32>(m_bindings.size()); }

	[[nodiscard]] VkDescriptorSetLayout get_descriptor_set_layout() const { return m_descriptor_set_layout; }
	[[nodiscard]] const std::unordered_map<Uint32, VkDescriptorSetLayoutBinding> &get_bindings() const {
		return m_bindings;
	}

  private:
	VulkanDevice &m_device;
	VkDescriptorSetLayout m_descriptor_set_layout = VK_NULL_HANDLE;
	std::unordered_map<Uint32, VkDescriptorSetLayoutBinding> m_bindings;
	friend class VulkanDescriptorWriter;
};

class VulkanDescriptorPool {
  public:
	class Builder {
	  public:
		Builder(VulkanDevice &device) : m_device(device) {}
		Builder &add_pool_size(VkDescriptorType descriptor_type, Uint32 count);
		Builder &set_pool_flags(VkDescriptorPoolCreateFlags flags);
		Builder &set_max_sets(Uint32 count);
		[[nodiscard]] Unique<VulkanDescriptorPool> build() const;

	  private:
		VulkanDevice &m_device;
		std::vector<VkDescriptorPoolSize> m_pool_sizes{};
		Uint32 m_max_sets = 1000;
		VkDescriptorPoolCreateFlags m_pool_flags = 0;
	};

	VulkanDescriptorPool(VulkanDevice &device, Uint32 max_sets, VkDescriptorPoolCreateFlags pool_flags,
						 const std::vector<VkDescriptorPoolSize> &pool_sizes);
	~VulkanDescriptorPool();
	AQUILA_NONCOPYABLE(VulkanDescriptorPool);

	bool allocate_descriptor(VkDescriptorSetLayout layout, VkDescriptorSet &set) const;
	void free_descriptor(VkDescriptorSet set) const;
	void free_descriptors(const std::vector<VkDescriptorSet> &sets) const;
	void reset_pool() const;

	[[nodiscard]] VkDescriptorPool get_descriptor_pool() const { return m_descriptor_pool; }

  private:
	VulkanDevice &m_device;
	VkDescriptorPool m_descriptor_pool = VK_NULL_HANDLE;
	mutable std::mutex m_mutex;
	friend class VulkanDescriptorWriter;
};

class VulkanDescriptorWriter {
  public:
	VulkanDescriptorWriter(VulkanDescriptorSetLayout &layout, VulkanDescriptorPool &pool);

	VulkanDescriptorWriter &write_buffer(Uint32 binding, const VkDescriptorBufferInfo *info);
	VulkanDescriptorWriter &write_image(Uint32 binding, const VkDescriptorImageInfo *info);
	VulkanDescriptorWriter &write_image_array(Uint32 binding, const std::vector<VkDescriptorImageInfo> &infos);

	bool build(VkDescriptorSet &set);
	void overwrite(const VkDescriptorSet &set);

  private:
	void fixup_and_submit(VkDescriptorSet set);

	VulkanDescriptorSetLayout &m_set_layout;
	VulkanDescriptorPool &m_pool;
	std::vector<VkWriteDescriptorSet> m_writes;
	std::vector<VkDescriptorBufferInfo> m_buffer_infos;
	std::vector<VkDescriptorImageInfo> m_image_infos;
};

} // namespace Aquila::RHI
#endif
