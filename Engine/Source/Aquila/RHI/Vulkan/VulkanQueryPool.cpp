#include "Aquila/RHI/Vulkan/VulkanQueryPool.h"

namespace Aquila::RHI {

VulkanQueryPool::VulkanQueryPool(VkDevice device, Uint32 count, F64 nanoseconds_per_tick)
	: m_device(device), m_count(count), m_nanoseconds_per_tick(nanoseconds_per_tick) {
	VkQueryPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	info.queryType = VK_QUERY_TYPE_TIMESTAMP;
	info.queryCount = count;
	if (vkCreateQueryPool(m_device, &info, nullptr, &m_pool) != VK_SUCCESS) {
		m_pool = VK_NULL_HANDLE;
		m_count = 0;
	}
}

VulkanQueryPool::~VulkanQueryPool() {
	if (m_pool != VK_NULL_HANDLE) {
		vkDestroyQueryPool(m_device, m_pool, nullptr);
	}
}

bool VulkanQueryPool::read_timestamps(Uint32 first, std::span<Uint64> out) {
	if (m_pool == VK_NULL_HANDLE || out.empty() || first + out.size() > m_count) {
		return false;
	}
	const VkResult result = vkGetQueryPoolResults(m_device, m_pool, first, static_cast<Uint32>(out.size()),
												  out.size_bytes(), out.data(), sizeof(Uint64), VK_QUERY_RESULT_64_BIT);
	return result == VK_SUCCESS;
}

}
