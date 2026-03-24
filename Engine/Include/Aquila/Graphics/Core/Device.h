#ifndef AQUILA_DEVICE_H
#define AQUILA_DEVICE_H

#include "Aquila/Core/AquilaCore.h"
#include "Aquila/Core/Window.h"
#include "Aquila/Graphics/Core/DeletionManager.h"

namespace Aquila::Graphics {
struct VkSwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> m_Formats;
	std::vector<VkPresentModeKHR> m_PresentModes;
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

class Device {
  private:
	struct ThreadLocalPool {
		VkCommandPool pool = VK_NULL_HANDLE;
	};

	VkInstance m_VulkanInstance{};
	VkDebugUtilsMessengerEXT m_DebugMessenger{};
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE; // Graphics card
	VkCommandPool m_GraphicsCommandPool{};
	VkCommandPool m_ComputeCommandPool{};
	VkCommandPool m_TransferCommandPool{};

	VkDevice m_Device; // Logical device
	VkSurfaceKHR m_Surface{};

	VkQueue m_ComputeQueue{};
	VkQueue m_GraphicsQueue{};
	VkQueue m_PresentQueue{};
	VkQueue m_TransferQueue{};

	std::mutex m_GraphicsQueueMutex;
	std::mutex m_PresentQueueMutex;
	std::mutex m_ComputeQueueMutex;
	std::mutex m_TransferQueueMutex;

	std::mutex m_TransferCommandPoolMutex;
	std::mutex m_GraphicsCommandPoolMutex;

	std::unordered_map<std::thread::id, ThreadLocalPool> m_ThreadPools;
	std::mutex m_ThreadPoolMapMutex;

	Unique<Utils::DeletionManager> m_DeletionManager;

	Core::Window &m_Window;

	const std::vector<const char *> validationLayers = { "VK_LAYER_KHRONOS_validation" };

	const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
														 VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
														 VK_KHR_MAINTENANCE1_EXTENSION_NAME };

	void CreateInstance();
	void SetupDebugMessenger();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	bool CheckDeviceExtensionSupport(VkPhysicalDevice vkPhysicalDevice) const;

	VkSwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice vkPhysicalDevice) const;

	[[nodiscard]] bool CheckValidationLayerSupport() const;

	static VkResult CreateDebugMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
											const VkAllocationCallbacks *pAllocator,
											VkDebugUtilsMessengerEXT *pDebugMessenger);

	static void DestroyDebugMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
										 const VkAllocationCallbacks *pAllocator);

	static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

	[[nodiscard]] std::vector<const char *> GetRequiredExtensions() const;

	static const char *GetObjectTypeName(VkObjectType objectType) {
		switch (objectType) {
		case VK_OBJECT_TYPE_BUFFER:
			return "Buffer";
		case VK_OBJECT_TYPE_IMAGE:
			return "Image";
		case VK_OBJECT_TYPE_IMAGE_VIEW:
			return "ImageView";
		case VK_OBJECT_TYPE_SAMPLER:
			return "Sampler";
		case VK_OBJECT_TYPE_DESCRIPTOR_SET:
			return "DescriptorSet";
		case VK_OBJECT_TYPE_FRAMEBUFFER:
			return "Framebuffer";
		case VK_OBJECT_TYPE_COMMAND_BUFFER:
			return "CommandBuffer";
		case VK_OBJECT_TYPE_PIPELINE:
			return "Pipeline";
		case VK_OBJECT_TYPE_DEVICE:
			return "Device";

		default:
			return "Unknown";
		}
	}

	/*
   * The first parameter specifies the severity of the message, which is one of
   the following flags:

      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Informational message like
   the creation of a resource VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
   Message about behavior that is not necessarily an error, but very likely a
   bug in your application VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
   Message about behavior that is invalid and may cause crashes
   */
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
														VkDebugUtilsMessageTypeFlagsEXT messageType,
														const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
														void *pUserData) {
		const auto *severityColor = "";
		const auto *severityText = "";
		const auto *resetColor = "\033[0m";

		switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			severityColor = "\033[31m";
			severityText = "ERROR";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			severityColor = "\033[33m";
			severityText = "WARNING";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			severityColor = "\033[36m";
			severityText = "INFO";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			severityColor = "\033[37m";
			severityText = "VERBOSE";
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
			severityColor = "\033[37m";
			severityText = "FLAG BITS MAX ENUM";
			break;
		default:
			break;
		}

		std::string typeStr;
		if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) != 0U) {
			typeStr += "GENERAL";
		}
		if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0U) {
			typeStr += "VALIDATION";
		}
		if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0U) {
			typeStr += "PERFORMANCE";
		}

		std::cerr << severityColor << "[VULKAN " << severityText << "]" << resetColor;
		if (!typeStr.empty()) {
			std::cerr << " [" << typeStr << "]";
		}
		std::cerr << '\n';

		if (pCallbackData->pMessageIdName != nullptr) {
			std::cerr << "  ID: " << pCallbackData->pMessageIdName << " (" << pCallbackData->messageIdNumber << ")"
					  << '\n';
		}

		std::cerr << "  Message: " << pCallbackData->pMessage << '\n';

		if (pCallbackData->objectCount > 0) {
			std::cerr << "  Objects involved:" << '\n';
			for (uint32 i = 0; i < pCallbackData->objectCount; i++) {
				const auto &obj = pCallbackData->pObjects[i];
				std::cerr << "    [" << i << "] " << GetObjectTypeName(obj.objectType);
				std::cerr << " (0x" << std::hex << obj.objectHandle << std::dec << ")";
				if (obj.pObjectName != nullptr) {
					std::cerr << " \"" << obj.pObjectName << "\"";
				}
				std::cerr << '\n';
			}
		}

		std::cerr << '\n';

		return VK_FALSE;
	}

	VkQueueFamilyIndices FindQueueFamilies(VkPhysicalDevice vkPhysicalDevice) const;
	void LogDeviceInfo() const;

	bool IsSuitable(VkPhysicalDevice vkPhysicalDevice);

  public:
	void SubmitToGraphicsQueue(const VkSubmitInfo *submitInfo, VkFence fence);
	void SubmitToComputeQueue(const VkSubmitInfo *submitInfo, VkFence fence);
	void SubmitToTransferQueue(const VkSubmitInfo *submitInfo, VkFence fence);
	void WaitGraphicsQueueIdle();
	void WaitTransferQueueIdle();

	VkPhysicalDeviceProperties m_Properties{};

#ifdef ENABLE_VALIDATION_LAYERS
	constexpr static bool enableValidationLayers = true;
#else
	constexpr static bool enableValidationLayers = false;
#endif

	explicit Device(Core::Window &window);
	~Device();

	AQUILA_NONCOPYABLE(Device);
	AQUILA_NONMOVEABLE(Device);

	VkCommandPool GetOrCreateThreadLocalGraphicsPool();

	[[nodiscard]] Utils::DeletionManager &GetDeletionManager() const {
		AQUILA_ASSERT(m_DeletionManager != nullptr, "DeletionQueue not initialized");
		return *m_DeletionManager;
	}
	[[nodiscard]] VkCommandPool GetGraphicsCommandPool() const { return m_GraphicsCommandPool; };
	[[nodiscard]] VkCommandPool GetComputeCommandPool() const { return m_ComputeCommandPool; };
	[[nodiscard]] VkCommandPool GetTransferCommandPool() const { return m_TransferCommandPool; };

	[[nodiscard]] VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; };
	[[nodiscard]] VkQueue GetPresentQueue() const { return m_PresentQueue; };
	[[nodiscard]] VkQueue GetComputeQueue() const { return m_ComputeQueue; };
	[[nodiscard]] VkQueue GetTransferQueue() const { return m_TransferQueue; };

	[[nodiscard]] VkInstance GetInstance() const { return m_VulkanInstance; }
	[[nodiscard]] VkDevice &GetDevice() { return m_Device; };
	[[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
	[[nodiscard]] VkSurfaceKHR GetSurface() const { return m_Surface; };
	[[nodiscard]] VkQueueFamilyIndices FindPhysicalQF() const { return FindQueueFamilies(m_PhysicalDevice); }
	[[nodiscard]] VkSwapChainSupportDetails GetSwapChainSupport() const {
		return QuerySwapChainSupport(m_PhysicalDevice);
	}

	void CreateGraphicsCommandPool();
	void CreateComputeCommandPool();
	void CreateTransferCommandPool();

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
					  VkDeviceMemory &bufferMemory);
	void CreateImageWithInfo(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags properties, VkImage &image,
							 VkDeviceMemory &imageMemory);
	VkFence CreateFence(bool signaled = false);
	void WaitForFence(VkFence fence);
	void DestroyFence(VkFence fence);

	uint32 FindMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties);
	VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
								 VkFormatFeatureFlags features);

	AQUILA_DEPRECATED("Use ExecuteGraphicsCommands or ExecuteTransferCommands")
	VkCommandBuffer BeginSingleTimeCommands();
	AQUILA_DEPRECATED("Use ExecuteGraphicsCommands or ExecuteTransferCommands")
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	template <typename Func>
	void ExecuteSingleTimeCommands(VkCommandPool pool, VkQueue queue, std::mutex &queueMutex, Func &&func) {
		VkCommandBuffer cmd = nullptr;

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = pool;
		allocInfo.commandBufferCount = 1;
		vkAllocateCommandBuffers(m_Device, &allocInfo, &cmd);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmd, &beginInfo);

		std::forward<Func>(func)(cmd);

		vkEndCommandBuffer(cmd);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd;

		{
			std::lock_guard<std::mutex> lock(queueMutex);
			vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(queue);
		}

		vkFreeCommandBuffers(m_Device, pool, 1, &cmd);
	}

	template <typename Func> void ExecuteGraphicsCommands(Func &&func) {
		VkCommandPool pool = GetOrCreateThreadLocalGraphicsPool();
		ExecuteSingleTimeCommands(pool, m_GraphicsQueue, m_GraphicsQueueMutex, std::forward<Func>(func));
	}

	template <typename Func> void ExecuteTransferCommands(Func &&func) {
		ExecuteSingleTimeCommands(m_TransferCommandPool, m_TransferQueue, m_TransferQueueMutex,
								  m_TransferCommandPoolMutex, std::forward<Func>(func));
	}
	void Wait() const { vkDeviceWaitIdle(m_Device); }

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void SetObjectDebugName(VkObjectType objectType, uint64_t handle, const char *name) const;
};
} // namespace Aquila::Graphics

#endif
