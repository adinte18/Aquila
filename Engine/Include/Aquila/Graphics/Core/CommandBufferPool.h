#ifndef COMMAND_BUFFER_POOL_H
#define COMMAND_BUFFER_POOL_H

#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Graphics/Core/CommandBuffer.h"

namespace Aquila::Graphics {
class CommandBufferPool {
  public:
	CommandBufferPool(Device &device, uint32 framesInFlight);
	~CommandBufferPool();

	Unique<CommandBuffer> CreateCommandBuffer(CommandBufferType type, const std::string &name = "");
	std::vector<Unique<CommandBuffer>> CreateCommandBuffers(CommandBufferType type, uint32 count,
															const std::string &baseName = "");

	Unique<CommandBuffer> GetFrameCommandBuffer(CommandBufferType type, uint32 frameIndex,
												const std::string &name = "");

	void ResetPool();
	uint32 GetFramesInFlight() const { return m_FramesInFlight; }

  private:
	Device &m_Device;
	VkCommandPool m_GraphicsPool = VK_NULL_HANDLE;
	VkCommandPool m_ComputePool = VK_NULL_HANDLE;
	VkCommandPool m_TransferPool = VK_NULL_HANDLE;

	uint32 m_FramesInFlight;

	std::vector<std::vector<Unique<CommandBuffer>>> m_FrameCommandBuffers;
};
} // namespace Aquila::Graphics

#endif // COMMAND_BUFFER_POOL_H
