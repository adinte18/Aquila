// VulkanCommandListPool.cpp
#include "Aquila/RHI/Vulkan/VulkanCommandListPool.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

VulkanCommandListPool::VulkanCommandListPool(VulkanDevice &device, uint32 framesInFlight)
	: m_Device(device), m_FramesInFlight(framesInFlight) {
	auto qf = device.FindPhysicalQF();

	VkCommandPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	info.queueFamilyIndex = qf.m_GraphicsFamily.value();
	AQUILA_VULKAN_CHECK(vkCreateCommandPool(device.GetDevice(), &info, nullptr, &m_GraphicsPool));

	info.queueFamilyIndex = qf.m_ComputeFamily.value();
	AQUILA_VULKAN_CHECK(vkCreateCommandPool(device.GetDevice(), &info, nullptr, &m_ComputePool));

	info.queueFamilyIndex = qf.m_TransferFamily.value();
	AQUILA_VULKAN_CHECK(vkCreateCommandPool(device.GetDevice(), &info, nullptr, &m_TransferPool));
}

VulkanCommandListPool::~VulkanCommandListPool() {
	// Clear command lists before destroying pools — VulkanCommandList destructor
	// doesn't free the buffer, so we explicitly free here via Reset()
	Reset();

	VkDevice dev = m_Device.GetDevice();
	if (m_GraphicsPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(dev, m_GraphicsPool, nullptr);
	}
	if (m_ComputePool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(dev, m_ComputePool, nullptr);
	}
	if (m_TransferPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(dev, m_TransferPool, nullptr);
	}
}

VkCommandPool VulkanCommandListPool::GetVkPool(CommandListType type) const {
	switch (type) {
	case CommandListType::Compute:
		return m_ComputePool;
	case CommandListType::Transfer:
		return m_TransferPool;
	default:
		return m_GraphicsPool;
	}
}

IRHICommandList *VulkanCommandListPool::Allocate(CommandListType type, const std::string &name) {
	// VulkanCommandList allocates its own VkCommandBuffer in its constructor
	auto cmd = CreateUnique<VulkanCommandList>(m_Device, GetVkPool(type), type, name);
	auto *ptr = cmd.get();
	m_Allocated.push_back(std::move(cmd));
	return ptr;
}

void VulkanCommandListPool::Free(IRHICommandList *cmd) {
	auto it = std::find_if(m_Allocated.begin(), m_Allocated.end(),
						   [cmd](const Unique<VulkanCommandList> &c) { return c.get() == cmd; });
	if (it == m_Allocated.end()) {
		return;
	}

	auto &vkCmd = static_cast<VulkanCommandList &>(**it);
	VkCommandBuffer handle = vkCmd.GetHandle();
	VkCommandPool pool = vkCmd.GetPool(); // correct pool stored on the command list itself

	vkFreeCommandBuffers(m_Device.GetDevice(), pool, 1, &handle);
	m_Allocated.erase(it);
}

void VulkanCommandListPool::Reset() {
	// Free all individual command buffers before resetting pools
	for (auto &cmd : m_Allocated) {
		VkCommandBuffer handle = cmd->GetHandle();
		VkCommandPool pool = cmd->GetPool();
		vkFreeCommandBuffers(m_Device.GetDevice(), pool, 1, &handle);
	}
	m_Allocated.clear();

	if (m_GraphicsPool != VK_NULL_HANDLE) {
		vkResetCommandPool(m_Device.GetDevice(), m_GraphicsPool, 0);
	}
	if (m_ComputePool != VK_NULL_HANDLE) {
		vkResetCommandPool(m_Device.GetDevice(), m_ComputePool, 0);
	}
	if (m_TransferPool != VK_NULL_HANDLE) {
		vkResetCommandPool(m_Device.GetDevice(), m_TransferPool, 0);
	}
}

} // namespace Aquila::RHI
