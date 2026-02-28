#include "Aquila/Graphics/Pipeline/DescriptorAllocator.h"

namespace Aquila::Graphics::RenderingPipeline {

// Static member initialization
Ref<DescriptorPool> DescriptorAllocator::s_GlobalPool = nullptr;
std::mutex DescriptorAllocator::s_PoolMutex;

void DescriptorAllocator::Init(Device &device) {
	std::lock_guard<std::mutex> lock(s_PoolMutex);

	if (s_GlobalPool) {
		AQUILA_LOG_WARNING("DescriptorAllocator already initialized!");
		return;
	}

	s_GlobalPool = DescriptorPool::Builder(device)
					   .SetMaxSets(2048)
					   .SetPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
					   .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2048)
					   .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 256)
					   .Build();

	AQUILA_LOG_INFO("DescriptorAllocator initialized ");
}

bool DescriptorAllocator::Allocate(const VkDescriptorSetLayout layout, VkDescriptorSet &set) {
	std::lock_guard<std::mutex> lock(s_PoolMutex);

	if (!s_GlobalPool) {
		AQUILA_LOG_ERROR("DescriptorAllocator not initialized!");
		return false;
	}

	return s_GlobalPool->AllocateDescriptor(layout, set);
}

Ref<DescriptorPool> DescriptorAllocator::GetSharedPool() {
	std::lock_guard<std::mutex> lock(s_PoolMutex);

	if (!s_GlobalPool) {
		AQUILA_LOG_ERROR("DescriptorAllocator not initialized!");
	}

	return s_GlobalPool;
}

std::mutex &DescriptorAllocator::GetPoolMutex() {
	return s_PoolMutex;
}

void DescriptorAllocator::Cleanup() {
	std::lock_guard<std::mutex> lock(s_PoolMutex);

	if (s_GlobalPool) {
		s_GlobalPool.reset();
		AQUILA_LOG_INFO("DescriptorAllocator cleaned up");
	}
}

void DescriptorAllocator::Release(const std::vector<VkDescriptorSet> &sets) {
	std::lock_guard<std::mutex> lock(s_PoolMutex);

	if (!s_GlobalPool) {
		AQUILA_LOG_ERROR("Cannot release descriptors - allocator not initialized!");
		return;
	}

	s_GlobalPool->FreeDescriptors(sets);
}

void DescriptorAllocator::Release(VkDescriptorSet set) {
	std::lock_guard<std::mutex> lock(s_PoolMutex);

	if (!s_GlobalPool) {
		AQUILA_LOG_ERROR("Cannot release descriptor - allocator not initialized!");
		return;
	}

	s_GlobalPool->FreeDescriptor(set);
}

} // namespace Aquila::Graphics::RenderingPipeline
