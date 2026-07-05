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
	VulkanCommandListPool(VulkanDevice &device, Uint32 frames_in_flight);
	~VulkanCommandListPool() override;
	AQUILA_NONCOPYABLE(VulkanCommandListPool);
	AQUILA_NONMOVEABLE(VulkanCommandListPool);

	// IRHICommandListPool
	IRHICommandList *allocate(CommandListType type, const std::string &name = "") override;
	void free(IRHICommandList *cmd) override;
	void reset() override;
	[[nodiscard]] Uint32 get_frames_in_flight() const override { return m_frames_in_flight; }

  private:
	[[nodiscard]] VkCommandPool get_vk_pool(CommandListType type) const;

	VulkanDevice &m_device;
	VkCommandPool m_graphics_pool = VK_NULL_HANDLE;
	VkCommandPool m_compute_pool = VK_NULL_HANDLE;
	VkCommandPool m_transfer_pool = VK_NULL_HANDLE;
	Uint32 m_frames_in_flight;

	std::vector<Unique<VulkanCommandList>> m_allocated;
};

} // namespace Aquila::RHI
#endif
