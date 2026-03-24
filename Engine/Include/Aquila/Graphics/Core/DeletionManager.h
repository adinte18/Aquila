#ifndef AQUILA_DELETION_MANAGER_H
#define AQUILA_DELETION_MANAGER_H

#include "Aquila/Core/AquilaCore.h"
#include "Aquila/Core/Defines.h"

namespace Aquila::Graphics {
class Device;
}

namespace Aquila::Foundation {

using namespace Aquila::Graphics;

struct DeferredDeletion {
	using ResourceVariant =
		std::variant<VkPipeline, VkShaderModule, VkPipelineLayout, VkPipelineCache, VkImage, VkImageView, VkBuffer,
					 VkDeviceMemory, VkSampler, VkDescriptorSetLayout, VkRenderPass, VkFramebuffer>;

	ResourceVariant resource;
};

class DeletionManager final {
  public:
	explicit DeletionManager(Device &device);
	~DeletionManager();

	AQUILA_NONCOPYABLE(DeletionManager);
	AQUILA_NONMOVEABLE(DeletionManager);

	// Deferred path — GPU may still be reading this resource
	// Safe to call from destructors, will be destroyed once GPU is done
	void QueueDeletion(const DeferredDeletion::ResourceVariant &resource);

	// Immediate path — caller guarantees GPU is already done with this resource
	// Use for staging buffers, init-time resources, anything synchronous
	void DestroyNow(const DeferredDeletion::ResourceVariant &resource);

	// Called once per frame after IsPreviousFrameComplete()
	void ProcessDeletions();

	// Destroys everything remaining — call on shutdown after WaitIdle()
	void Flush();

	// How many deletions are pending
	[[nodiscard]] size_t PendingCount() const { return m_DeletionQueue.size(); }

	// Whether there are any pending deletions
	[[nodiscard]] bool HasPendingDeletions() const { return !m_DeletionQueue.empty(); }

  private:
	void Dispatch(const DeferredDeletion::ResourceVariant &resource);

	Device &m_Device;
	std::queue<DeferredDeletion> m_DeletionQueue;
};

} // namespace Aquila::Foundation

#endif
