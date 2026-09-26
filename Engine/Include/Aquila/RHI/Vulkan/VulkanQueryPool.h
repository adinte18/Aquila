#ifndef AQUILA_VULKAN_QUERY_POOL_H
#define AQUILA_VULKAN_QUERY_POOL_H

#include "Aquila/RHI/Backend/IRHIQueryPool.h"

#include <vulkan/vulkan.h>

namespace Aquila::RHI {

class VulkanQueryPool final : public IRHIQueryPool {
  public:
	VulkanQueryPool(VkDevice device, Uint32 count, F64 nanoseconds_per_tick);
	~VulkanQueryPool() override;

	VulkanQueryPool(const VulkanQueryPool &) = delete;
	VulkanQueryPool &operator=(const VulkanQueryPool &) = delete;

	[[nodiscard]] Uint32 get_count() const override { return m_count; }
	[[nodiscard]] F64 get_nanoseconds_per_tick() const override { return m_nanoseconds_per_tick; }
	[[nodiscard]] bool read_timestamps(Uint32 first, std::span<Uint64> out) override;

	[[nodiscard]] VkQueryPool get_handle() const { return m_pool; }

  private:
	VkDevice m_device = VK_NULL_HANDLE;
	VkQueryPool m_pool = VK_NULL_HANDLE;
	Uint32 m_count = 0;
	F64 m_nanoseconds_per_tick = 1.0;
};

}

#endif
