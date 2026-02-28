#include "Aquila/Graphics/Core/CommandBufferPool.h"

namespace Aquila::Graphics {
CommandBufferPool::CommandBufferPool(Device &device, const uint32 framesInFlight)
	: m_Device(device), m_GraphicsPool(m_Device.GetGraphicsCommandPool()),
	  m_ComputePool(m_Device.GetComputeCommandPool()), m_TransferPool(m_Device.GetTransferCommandPool()),
	  m_FramesInFlight(framesInFlight) {
	m_FrameCommandBuffers.resize(framesInFlight);
}

CommandBufferPool::~CommandBufferPool() {}

Unique<CommandBuffer> CommandBufferPool::CreateCommandBuffer(CommandBufferType type, const std::string &name) {
	VkCommandPool pool = nullptr;
	switch (type) {
	case CommandBufferType::COMPUTE:
		pool = m_ComputePool;
		break;
	case CommandBufferType::TRANSFER:
		pool = m_TransferPool;
		break;
	default:
		pool = m_GraphicsPool;
		break;
	}
	return CreateUnique<CommandBuffer>(m_Device, pool, type, name);
}
std::vector<Unique<CommandBuffer>>
CommandBufferPool::CreateCommandBuffers(const CommandBufferType type, const uint32 count, const std::string &baseName) {
	std::vector<Unique<CommandBuffer>> commandBuffers;
	commandBuffers.reserve(count);

	for (uint32 i = 0; i < count; ++i) {
		std::string name = baseName.empty() ? "" : baseName + "_" + std::to_string(i);
		commandBuffers.push_back(CreateCommandBuffer(type, name));
	}

	return commandBuffers;
}

Unique<CommandBuffer> CommandBufferPool::GetFrameCommandBuffer(CommandBufferType type, uint32 frameIndex,
															   const std::string &name) {
	if (frameIndex >= m_FramesInFlight) {
		throw std::runtime_error("Invalid frame index: " + std::to_string(frameIndex));
	}

	const std::string fullName = name + "_Frame" + std::to_string(frameIndex);
	return CreateCommandBuffer(type, fullName);
}

} // namespace Aquila::Graphics
