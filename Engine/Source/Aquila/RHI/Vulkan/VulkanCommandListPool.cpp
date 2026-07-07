// VulkanCommandListPool.cpp
#include "Aquila/RHI/Vulkan/VulkanCommandListPool.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

VulkanCommandListPool::VulkanCommandListPool(VulkanDevice &device, Uint32 frames_in_flight)
	: m_device(device), m_frames_in_flight(frames_in_flight) {
	auto qf = device.find_physical_qf();

	VkCommandPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	info.queueFamilyIndex = qf.m_graphics_family.value();
	AQUILA_VULKAN_CHECK(vkCreateCommandPool(device.get_device(), &info, nullptr, &m_graphics_pool));

	info.queueFamilyIndex = qf.m_compute_family.value();
	AQUILA_VULKAN_CHECK(vkCreateCommandPool(device.get_device(), &info, nullptr, &m_compute_pool));

	info.queueFamilyIndex = qf.m_transfer_family.value();
	AQUILA_VULKAN_CHECK(vkCreateCommandPool(device.get_device(), &info, nullptr, &m_transfer_pool));
}

VulkanCommandListPool::~VulkanCommandListPool() {
	// Clear command lists before destroying pools — VulkanCommandList destructor
	// doesn't free the buffer, so we explicitly free here via Reset()
	reset();

	VkDevice dev = m_device.get_device();
	if (m_graphics_pool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(dev, m_graphics_pool, nullptr);
	}
	if (m_compute_pool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(dev, m_compute_pool, nullptr);
	}
	if (m_transfer_pool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(dev, m_transfer_pool, nullptr);
	}
}

VkCommandPool VulkanCommandListPool::get_vk_pool(CommandListType type) const {
	switch (type) {
	case CommandListType::Compute:
		return m_compute_pool;
	case CommandListType::Transfer:
		return m_transfer_pool;
	default:
		return m_graphics_pool;
	}
}

IRHICommandList *VulkanCommandListPool::allocate(CommandListType type, const std::string &name) {
	// VulkanCommandList allocates its own VkCommandBuffer in its constructor
	auto cmd = std::make_unique<VulkanCommandList>(m_device, get_vk_pool(type), type, name);
	auto *ptr = cmd.get();
	m_allocated.push_back(std::move(cmd));
	return ptr;
}

void VulkanCommandListPool::free(IRHICommandList *cmd) {
	auto it = std::find_if(m_allocated.begin(), m_allocated.end(),
						   [cmd](const Unique<VulkanCommandList> &c) { return c.get() == cmd; });
	if (it == m_allocated.end()) {
		return;
	}

	auto &vk_cmd = static_cast<VulkanCommandList &>(**it);
	VkCommandBuffer handle = vk_cmd.get_handle();
	VkCommandPool pool = vk_cmd.get_pool(); // correct pool stored on the command list itself

	vkFreeCommandBuffers(m_device.get_device(), pool, 1, &handle);
	m_allocated.erase(it);
}

void VulkanCommandListPool::reset() {
	// Free all individual command buffers before resetting pools
	for (auto &cmd : m_allocated) {
		VkCommandBuffer handle = cmd->get_handle();
		VkCommandPool pool = cmd->get_pool();
		vkFreeCommandBuffers(m_device.get_device(), pool, 1, &handle);
	}
	m_allocated.clear();

	if (m_graphics_pool != VK_NULL_HANDLE) {
		vkResetCommandPool(m_device.get_device(), m_graphics_pool, 0);
	}
	if (m_compute_pool != VK_NULL_HANDLE) {
		vkResetCommandPool(m_device.get_device(), m_compute_pool, 0);
	}
	if (m_transfer_pool != VK_NULL_HANDLE) {
		vkResetCommandPool(m_device.get_device(), m_transfer_pool, 0);
	}
}

} // namespace Aquila::RHI
