#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include "Aquila/Graphics/Core/Device.h"

namespace Aquila::Graphics {
enum class CommandBufferType : uint8 { PRESENT, OFFSCREEN, COMPUTE, TRANSFER };

class CommandBuffer {
  public:
	CommandBuffer(Device &device, VkCommandPool commandPool, CommandBufferType type, const std::string &name);
	~CommandBuffer();

	void Begin(VkCommandBufferUsageFlags flags = 0);
	void Reset();
	void End();

	[[nodiscard]] VkCommandBuffer GetHandle() const;
	[[nodiscard]] CommandBufferType GetType() const;
	[[nodiscard]] const std::string &GetName() const;
	[[nodiscard]] bool IsRecording() const;

  private:
	VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
	CommandBufferType m_Type;
	std::string m_Name;
	bool m_IsRecording = false;
	Device &m_Device;
};
} // namespace Aquila::Graphics

#endif // COMMAND_BUFFER_H
