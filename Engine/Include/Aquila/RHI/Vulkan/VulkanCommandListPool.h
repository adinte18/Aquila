#ifndef AQUILA_VULKAN_COMMAND_LIST_POOL_H
#define AQUILA_VULKAN_COMMAND_LIST_POOL_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHICommandListPool.h"
#include "Aquila/RHI/Vulkan/VulkanCommandList.h"

namespace Aquila::RHI {
class VulkanDevice;

class VulkanCommandListPool final : public IRHICommandListPool {
  public:
	VulkanCommandListPool(VulkanDevice &device, uint32 framesInFlight);
	~VulkanCommandListPool() override;
	AQUILA_NONCOPYABLE(VulkanCommandListPool);
	AQUILA_NONMOVEABLE(VulkanCommandListPool);

	// IRHICommandListPool
	IRHICommandList *Allocate(CommandListType type, const std::string &name = "") override;
	void Free(IRHICommandList *cmd) override;
	void Reset() override;
	[[nodiscard]] uint32 GetFramesInFlight() const override { return m_FramesInFlight; }

  private:
	[[nodiscard]] VkCommandPool GetVkPool(CommandListType type) const;

	VulkanDevice &m_Device;
	VkCommandPool m_GraphicsPool = VK_NULL_HANDLE;
	VkCommandPool m_ComputePool = VK_NULL_HANDLE;
	VkCommandPool m_TransferPool = VK_NULL_HANDLE;
	uint32 m_FramesInFlight;

	std::vector<Unique<VulkanCommandList>> m_Allocated;
};

} // namespace Aquila::RHI
#endif
