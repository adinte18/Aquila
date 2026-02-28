#pragma once

#include "Aquila/Graphics/Pipeline/Descriptor.h"
#include <mutex>

namespace Aquila::Graphics::RenderingPipeline {

class DescriptorAllocator {
  public:
	/**
	 * Initialize global descriptor pool (call once at startup)
	 */
	static void Init(Device &device);

	/**
	 * Thread-safe allocation of descriptor sets
	 */
	static bool Allocate(const VkDescriptorSetLayout layout, VkDescriptorSet &set);

	/**
	 * Get shared pool reference (use with caution - prefer Allocate/Release)
	 */
	static Ref<DescriptorPool> GetSharedPool();

	/**
	 * Get mutex for manual locking (used by DescriptorWriter)
	 */
	static std::mutex &GetPoolMutex();

	/**
	 * Cleanup global pool (call at shutdown)
	 */
	static void Cleanup();

	/**
	 * Thread-safe release of descriptor sets
	 */
	static void Release(const std::vector<VkDescriptorSet> &sets);
	static void Release(VkDescriptorSet set);

  private:
	static Ref<DescriptorPool> s_GlobalPool;
	static std::mutex s_PoolMutex;
};

} // namespace Aquila::Graphics::RenderingPipeline