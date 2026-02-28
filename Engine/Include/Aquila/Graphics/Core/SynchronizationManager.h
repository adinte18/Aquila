#ifndef AQUILA_SEMAPHORE_MANAGER_H
#define AQUILA_SEMAPHORE_MANAGER_H

#include "Aquila/Graphics/Core/Device.h"

namespace Aquila::Graphics {

class SynchronizationManager {
  public:
	SynchronizationManager(Device &device, uint32 framesInFlight);
	~SynchronizationManager();

	VkSemaphore GetSemaphore(const std::string &name, uint32 frameIndex);
	void CreateSemaphore(const std::string &name);

	void CreateFence(const std::string &name, bool startSignaled = true);
	VkFence GetFence(const std::string &name, uint32 frameIndex);
	bool IsFenceSignaled(const std::string &name, uint32 frameIndex);
	void WaitForFence(const std::string &name, uint32 frameIndex);
	void ResetFence(const std::string &name, uint32 frameIndex);

  private:
	struct SemaphoreSet {
		std::vector<VkSemaphore> semaphores;
	};

	struct FenceSet {
		std::vector<VkFence> fences;
	};

	Device &m_Device;
	uint32 m_FramesInFlight;

	std::unordered_map<std::string, SemaphoreSet> m_Semaphores;
	std::unordered_map<std::string, FenceSet> m_Fences;
};
} // namespace Aquila::Graphics

#endif // SEMAPHORE_MANAGER_H
