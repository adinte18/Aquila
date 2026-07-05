#pragma once
#include "GraphicsPCH.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHIDescriptors.h"

namespace Aquila::RHI {

class VulkanDevice;
class VulkanDescriptorSetLayout;
class VulkanDescriptorPool;

class VulkanDescriptorSet final : public IRHIDescriptorSet {
  public:
	VulkanDescriptorSet(VulkanDevice &device, VkDescriptorSet set, VulkanDescriptorSetLayout &layout,
						VulkanDescriptorPool &pool);
	~VulkanDescriptorSet() override;
	AQUILA_NONCOPYABLE(VulkanDescriptorSet);

	void set_buffer(Uint32 binding, IRHIBuffer &buffer, Uint64 offset = 0, Uint64 range = 0) override;
	void set_texture(Uint32 binding, IRHITexture &texture) override;
	void flush() override;

	[[nodiscard]] VkDescriptorSet get_descriptor_set() const { return m_set; }

  private:
	VulkanDevice &m_device;
	VkDescriptorSet m_set;
	VulkanDescriptorSetLayout &m_layout;
	VulkanDescriptorPool &m_pool;

	std::vector<VkWriteDescriptorSet> m_pending_writes;
	std::vector<VkDescriptorBufferInfo> m_buffer_infos;
	std::vector<VkDescriptorImageInfo> m_image_infos;
};

} // namespace Aquila::RHI
