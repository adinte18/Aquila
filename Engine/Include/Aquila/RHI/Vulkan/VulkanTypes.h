#ifndef AQUILA_VULKAN_TYPES_H
#define AQUILA_VULKAN_TYPES_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::RHI {

struct VkSwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> m_Formats;
	std::vector<VkPresentModeKHR> m_PresentModes;
};

struct BufferAllocation {
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo info = {};
	void *mappedPtr = nullptr;

	[[nodiscard]] bool IsValid() const { return buffer != VK_NULL_HANDLE; }
	[[nodiscard]] bool IsMapped() const { return mappedPtr != nullptr; }
};

struct ImageAllocation {
	VkImage image = VK_NULL_HANDLE;
	VmaAllocation allocation = nullptr;
	VmaAllocationInfo info = {};
	void *mappedPtr = nullptr;
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkExtent3D extent = {};
	uint32_t mipLevels = 1;
	uint32_t arrayLayers = 1;
};

struct VkQueueFamilyIndices {
	std::optional<uint32> m_GraphicsFamily;
	std::optional<uint32> m_PresentFamily;
	std::optional<uint32> m_ComputeFamily;
	std::optional<uint32> m_TransferFamily;

	[[nodiscard]] bool IsComplete() const {
		return m_GraphicsFamily.has_value() && m_PresentFamily.has_value() && m_ComputeFamily.has_value() &&
			   m_TransferFamily.has_value();
	}
};

} // namespace Aquila::RHI
#endif
