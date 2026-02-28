#include "Aquila/Graphics/Core/CommandBuffer.h"

namespace Aquila::Graphics {

CommandBuffer::CommandBuffer(Device &device, const VkCommandPool commandPool, const CommandBufferType type,
							 const std::string &name)
	: m_Type(type), m_Name(name), m_Device(device) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	AQUILA_VULKAN_CHECK(vkAllocateCommandBuffers(m_Device.GetDevice(), &allocInfo, &m_CommandBuffer));

	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64>(m_CommandBuffer),
								(name).c_str());
}

CommandBuffer::~CommandBuffer() {
	// Note(A) : freed automatically when comand pool destroyed
}

void CommandBuffer::Begin(VkCommandBufferUsageFlags flags) {
	if (m_IsRecording) {
		throw std::runtime_error("Command buffer " + m_Name + " is in usage already");
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = flags;
	AQUILA_VULKAN_CHECK(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));
	m_IsRecording = true;
}

void CommandBuffer::End() {
	if (!m_IsRecording) {
		throw std::runtime_error("Command buffer " + m_Name + " is not recording!");
	}

	AQUILA_VULKAN_CHECK(vkEndCommandBuffer(m_CommandBuffer));

	m_IsRecording = false;
}

void CommandBuffer::Reset() {
	vkResetCommandBuffer(m_CommandBuffer, 0);
	m_IsRecording = false;
}

VkCommandBuffer CommandBuffer::GetHandle() const {
	return m_CommandBuffer;
}
CommandBufferType CommandBuffer::GetType() const {
	return m_Type;
}
const std::string &CommandBuffer::GetName() const {
	return m_Name;
}
bool CommandBuffer::IsRecording() const {
	return m_IsRecording;
}

} // namespace Aquila::Graphics
