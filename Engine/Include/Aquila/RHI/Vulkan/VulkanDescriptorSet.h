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

	void SetBuffer(uint32 binding, IRHIBuffer &buffer, uint64 offset = 0, uint64 range = 0) override;
	void SetTexture(uint32 binding, IRHITexture &texture) override;
	void Flush() override;

	[[nodiscard]] VkDescriptorSet GetDescriptorSet() const { return m_Set; }

  private:
	VulkanDevice &m_Device;
	VkDescriptorSet m_Set;
	VulkanDescriptorSetLayout &m_Layout;
	VulkanDescriptorPool &m_Pool;

	std::vector<VkWriteDescriptorSet> m_PendingWrites;
	std::vector<VkDescriptorBufferInfo> m_BufferInfos;
	std::vector<VkDescriptorImageInfo> m_ImageInfos;
};

} // namespace Aquila::RHI
