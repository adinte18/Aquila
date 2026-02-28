#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Core/Defines.h"
#include "Aquila/Graphics/Pipeline/DescriptorAllocator.h"
#include <mutex>
#include <string>

namespace Aquila::Graphics {
Device::Device(Core::Window &window) : m_Window{ window } {
	CreateInstance();
	SetupDebugMessenger();
	m_Window.CreateWindowSurface(m_VulkanInstance, &m_Surface);
	PickPhysicalDevice();
	CreateLogicalDevice();

	CreateGraphicsCommandPool();
	CreateComputeCommandPool();
	CreateTransferCommandPool();

	if (AQUILA_UNLIKELY(m_Device != nullptr)) {
		m_DeletionManager = CreateUnique<Utils::DeletionManager>(*this);
	} else {
		AQUILA_LOG_CRITICAL("Device is null, cant initialize deletion queue");
		abort(); // FIXME : maybe lets not abort :P
	}

	LogDeviceInfo();
}

Device::~Device() {
	m_DeletionManager.reset();

	RenderingPipeline::DescriptorAllocator::Cleanup(); // release global pool

	for (auto &[id, threadPool] : m_ThreadPools) {
		if (threadPool.pool != VK_NULL_HANDLE) {
			vkDestroyCommandPool(m_Device, threadPool.pool, nullptr);
		}
	}

	vkDestroyCommandPool(m_Device, m_GraphicsCommandPool, nullptr);
	vkDestroyCommandPool(m_Device, m_ComputeCommandPool, nullptr);
	vkDestroyCommandPool(m_Device, m_TransferCommandPool, nullptr);

	vkDestroyDevice(m_Device, nullptr);

	if (enableValidationLayers) {
		DestroyDebugMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);
	vkDestroyInstance(m_VulkanInstance, nullptr);

	AQUILA_LOG_DEBUG("Device destroyed!");
}

void Device::CreateGraphicsCommandPool() {
	const VkQueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.m_GraphicsFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	AQUILA_VULKAN_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_GraphicsCommandPool));
}

void Device::CreateComputeCommandPool() {
	const VkQueueFamilyIndices queueFamiliesIndices = FindQueueFamilies(m_PhysicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamiliesIndices.m_ComputeFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	AQUILA_VULKAN_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_ComputeCommandPool));
}

void Device::CreateTransferCommandPool() {
	const VkQueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = indices.m_TransferFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	AQUILA_VULKAN_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_TransferCommandPool));
}

void Device::LogDeviceInfo() const {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

	AQUILA_LOG_INFO("Graphics Device Information");
	AQUILA_LOG_INFO("Device Name: " + std::string(properties.deviceName));

	std::string deviceType;
	switch (properties.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		deviceType = "Discrete GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		deviceType = "Integrated GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		deviceType = "Virtual GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		deviceType = "CPU";
		break;
	default:
		deviceType = "Unknown";
	}

	AQUILA_LOG_INFO("Device Type: " + deviceType);

	const uint32 driverVersion = properties.driverVersion;
	const uint32 apiVersion = properties.apiVersion;

	AQUILA_LOG_INFO("Driver Version: " + std::to_string(VK_VERSION_MAJOR(driverVersion)) + "." +
					std::to_string(VK_VERSION_MINOR(driverVersion)) + "." +
					std::to_string(VK_VERSION_PATCH(driverVersion)));

	AQUILA_LOG_INFO("Vulkan Version: " + std::to_string(VK_VERSION_MAJOR(apiVersion)) + "." +
					std::to_string(VK_VERSION_MINOR(apiVersion)) + "." + std::to_string(VK_VERSION_PATCH(apiVersion)));

	// Log memory info
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

	uint64 totalMemory = 0;
	for (uint32 i = 0; i < memProperties.memoryHeapCount; i++) {
		if ((memProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0U) {
			totalMemory += memProperties.memoryHeaps[i].size;
		}
	}
	AQUILA_LOG_INFO("Device Memory: {}", std::to_string(totalMemory / (1024 * 1024)) + " MB");

	// Log queue families
	VkQueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
	AQUILA_LOG_INFO("Graphics Queue Family: " + std::to_string(indices.m_GraphicsFamily.value()));
	AQUILA_LOG_INFO("Present Queue Family: " + std::to_string(indices.m_PresentFamily.value()));
	AQUILA_LOG_INFO("Compute Queue Family: " + std::to_string(indices.m_ComputeFamily.value()));
	AQUILA_LOG_INFO("Transfer Queue Family: " + std::to_string(indices.m_TransferFamily.value()));

	AQUILA_LOG_INFO("============================================");
}

void Device::CreateInstance() {
	if (enableValidationLayers && !CheckValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	/*
   * This data is technically optional,
   * but it may provide some useful information to the driver in order to
   * optimize our specific application (e.g. because it uses a well-known
   * graphics engine with certain special behavior).
   */
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Aquila Studio";
	appInfo.applicationVersion =
		VK_MAKE_API_VERSION(0, AQUILA_VERSION_MAJOR, AQUILA_VERSION_MINOR, AQUILA_VERSION_PATCH);
	appInfo.pEngineName = "Aquila";
	appInfo.engineVersion = VK_MAKE_API_VERSION(0, AQUILA_VERSION_MAJOR, AQUILA_VERSION_MINOR, AQUILA_VERSION_PATCH);
	appInfo.apiVersion = VK_API_VERSION_1_4;

	/*
   * This struct is not optional and tells the Vulkan driver which global
   * extensions and validation layers we want to use. Global here means that
   * they apply to the entire program and not a specific device. We can also use
   * this struct to pass information about our own application to the driver.
   */
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	/*
   * The extensions are a way to add functionality to the Vulkan core.
   * For example, there are extensions that add support for specific window
   * systems like X11 or Win32. There are also extensions that provide more
   * advanced functionality like ray tracing. In order to use any extension, we
   * need to enable it at instance creation time. We can get a list of supported
   * extensions using glfw.
   */

	const auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (&debugCreateInfo);
	} else {
		/*
     * The extensions are a way to add functionality to the Vulkan core.
     * For example, there are extensions that add support for specific window
     * systems like X11 or Win32. There are also extensions that provide more
     * advanced functionality like ray tracing. In order to use any extension,
     * we need to enable it at instance creation time. We can get a list of
     * supported extensions using glfw.
     */

		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	AQUILA_VULKAN_CHECK(vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance));
}

#pragma region Validation Layer and Extension Support
bool Device::CheckValidationLayerSupport() const {
	uint32 layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char *layerName : validationLayers) {
		bool layerFound = false;

		for (const auto &layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

std::vector<const char *> Device::GetRequiredExtensions() const {
	uint32 glfwExtensionCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}
#pragma endregion

#pragma region Logical/Physical Device
void Device::CreateLogicalDevice() {
	VkQueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32> uniqueQueueFamilies = { indices.m_GraphicsFamily.value(), indices.m_PresentFamily.value(),
											 indices.m_ComputeFamily.value(), indices.m_TransferFamily.value() };

	// Request 2 queues from graphics family (for async transfers)
	std::vector<f32> queuePriorities = { 1.0F, 0.5F }; // Main queue, transfer queue

	for (uint32 queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;

		// Request 2 queues if this is the graphics family
		if (queueFamily == indices.m_GraphicsFamily.value()) {
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());
			uint32 availableQueues = queueFamilies[queueFamily].queueCount;
			queueCreateInfo.queueCount = std::min(2U, availableQueues);
			queueCreateInfo.pQueuePriorities = queuePriorities.data();
		} else {
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = queuePriorities.data();
		}

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
	dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.wideLines = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.independentBlend = VK_TRUE;

	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.pNext = &dynamicRenderingFeatures;
	deviceFeatures2.features = deviceFeatures; // what an original way to call them deviceFeature *2* :D

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pNext = &deviceFeatures2;
	createInfo.pEnabledFeatures = nullptr; // needs to be nullptr

	createInfo.enabledExtensionCount = static_cast<uint32>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	bool computeSharesGraphics = indices.m_ComputeFamily.value() == indices.m_GraphicsFamily.value();

	vkGetDeviceQueue(m_Device, indices.m_GraphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, indices.m_PresentFamily.value(), 0, &m_PresentQueue);
	vkGetDeviceQueue(m_Device, indices.m_TransferFamily.value(), 0, &m_TransferQueue);

	if (computeSharesGraphics) {
		// Use queue index 1 if available, otherwise fall back to 0
		uint32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

		uint32 availableQueues = queueFamilies[indices.m_GraphicsFamily.value()].queueCount;
		uint32 computeQueueIndex = (availableQueues > 1) ? 1 : 0;
		vkGetDeviceQueue(m_Device, indices.m_ComputeFamily.value(), computeQueueIndex, &m_ComputeQueue);
	} else {
		// Dedicated compute family, always index 0
		vkGetDeviceQueue(m_Device, indices.m_ComputeFamily.value(), 0, &m_ComputeQueue);
	}
}

void Device::PickPhysicalDevice() {
	uint32 deviceCount = 0;
	vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		AQUILA_LOG_CRITICAL("Failed to find GPUs with Vulkan support!");
		abort();
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, devices.data());

	for (const auto &device : devices) {
		if (IsSuitable(device)) {
			m_PhysicalDevice = device;
			break;
		}
	}

	if (m_PhysicalDevice == VK_NULL_HANDLE) {
		AQUILA_LOG_CRITICAL("Failed to find GPUs with Vulkan support!");
		abort();
	}
}

bool Device::IsSuitable(const VkPhysicalDevice vkPhysicalDevice) {
	VkQueueFamilyIndices indices = FindQueueFamilies(vkPhysicalDevice);

	const bool extensionSupported = CheckDeviceExtensionSupport(vkPhysicalDevice);

	bool swapChainAdequate = false;
	if (extensionSupported) {
		const VkSwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(vkPhysicalDevice);
		swapChainAdequate = !swapChainSupport.m_Formats.empty() && !swapChainSupport.m_PresentModes.empty();
	}

	return indices.IsComplete() && extensionSupported && swapChainAdequate;
}

void Device::SubmitToComputeQueue(const VkSubmitInfo *submitInfo, VkFence fence) {
	std::lock_guard<std::mutex> lock(m_ComputeQueueMutex);
	AQUILA_VULKAN_CHECK(vkQueueSubmit(m_ComputeQueue, 1, submitInfo, fence));
}

void Device::SubmitToGraphicsQueue(const VkSubmitInfo *submitInfo, VkFence fence) {
	std::lock_guard<std::mutex> lock(m_GraphicsQueueMutex);
	AQUILA_VULKAN_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, submitInfo, fence));
}

void Device::SubmitToTransferQueue(const VkSubmitInfo *submitInfo, VkFence fence) {
	std::lock_guard<std::mutex> lock(m_TransferQueueMutex);
	AQUILA_VULKAN_CHECK(vkQueueSubmit(m_TransferQueue, 1, submitInfo, fence));
}

void Device::WaitGraphicsQueueIdle() {
	std::lock_guard<std::mutex> lock(m_GraphicsQueueMutex);
	AQUILA_VULKAN_CHECK(vkQueueWaitIdle(m_GraphicsQueue));
}

void Device::WaitTransferQueueIdle() {
	std::lock_guard<std::mutex> lock(m_TransferQueueMutex);
	AQUILA_VULKAN_CHECK(vkQueueWaitIdle(m_TransferQueue));
}

bool Device::CheckDeviceExtensionSupport(const VkPhysicalDevice vkPhysicalDevice) const {
	uint32 extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto &extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

VkQueueFamilyIndices Device::FindQueueFamilies(const VkPhysicalDevice vkPhysicalDevice) const {
	VkQueueFamilyIndices indices;

	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, queueFamilies.data());

	int index = 0;
	for (const auto &queueFamily : queueFamilies) {
		if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
			indices.m_GraphicsFamily = index;
		}

		if (((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0U) &&
			((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0U) &&
			((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0U)) {
			indices.m_TransferFamily = index;
		}

		if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0U) {
			bool isDedicated = (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0U;
			VkBool32 presentSupport = 0U;
			vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, index, m_Surface, &presentSupport);
			bool isPresent = presentSupport != 0U;

			// Never share compute with present, dedicated or not
			if (!isPresent) {
				if (isDedicated || !indices.m_ComputeFamily.has_value()) {
					indices.m_ComputeFamily = index;
				}
			}
		}

		VkBool32 presentSupport = 0U;
		vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, index, m_Surface, &presentSupport);

		if (presentSupport != 0U) {
			indices.m_PresentFamily = index;
		}

		if (indices.IsComplete()) {
			break;
		}

		index++;
	}

	if (!indices.m_ComputeFamily.has_value() && indices.m_GraphicsFamily.has_value()) {
		indices.m_ComputeFamily = indices.m_GraphicsFamily;
	}
	if (!indices.m_TransferFamily.has_value()) {
		indices.m_TransferFamily = indices.m_ComputeFamily;
	}

	return indices;
}

#pragma endregion

#pragma region Debug Messenger
void Device::SetupDebugMessenger() {
	if (!enableValidationLayers) {
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);

	AQUILA_VULKAN_CHECK(CreateDebugMessengerEXT(m_VulkanInstance, &createInfo, nullptr, &m_DebugMessenger));
}

VkResult Device::CreateDebugMessengerEXT(const VkInstance instance,
										 const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
										 const VkAllocationCallbacks *pAllocator,
										 VkDebugUtilsMessengerEXT *pDebugMessenger) {
	if (const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
		func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void Device::DestroyDebugMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger,
									  const VkAllocationCallbacks *pAllocator) {
	const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void Device::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}

#pragma endregion

VkSwapChainSupportDetails Device::QuerySwapChainSupport(VkPhysicalDevice vkPhysicalDevice) const {
	VkSwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, m_Surface, &details.m_SurfaceCapabilities);

	uint32 formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, m_Surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.m_Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, m_Surface, &formatCount, details.m_Formats.data());
	}

	uint32 presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, m_Surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.m_PresentModes.resize(presentModeCount);
		AQUILA_VULKAN_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, m_Surface, &presentModeCount,
																	  details.m_PresentModes.data()));
	}

	return details;
}

uint32 Device::FindMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

	for (uint32 i = 0; i < memProperties.memoryTypeCount; i++) {
		if (((typeFilter & (1 << i)) != 0U) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

VkFence Device::CreateFence(bool signaled) {
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	VkFence fence = nullptr;
	AQUILA_VULKAN_CHECK(vkCreateFence(m_Device, &fenceInfo, nullptr, &fence));
	return fence;
}

void Device::WaitForFence(VkFence fence) {
	AQUILA_VULKAN_CHECK(vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX));
}

void Device::DestroyFence(VkFence fence) {
	vkDestroyFence(m_Device, fence, nullptr);
}

void Device::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
						  VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	AQUILA_VULKAN_CHECK(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}

	vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
}

void Device::CreateImageWithInfo(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags properties, VkImage &image,
								 VkDeviceMemory &imageMemory) {
	AQUILA_VULKAN_CHECK(vkCreateImage(m_Device, &imageInfo, nullptr, &image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_Device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
	AQUILA_VULKAN_CHECK(vkAllocateMemory(m_Device, &allocInfo, nullptr, &imageMemory));

	AQUILA_VULKAN_CHECK(vkBindImageMemory(m_Device, image, imageMemory, 0));
}

VkFormat Device::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
									 VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}

VkCommandBuffer Device::BeginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_GraphicsCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer = nullptr;
	vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void Device::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	SubmitToGraphicsQueue(&submitInfo, VK_NULL_HANDLE);
	WaitGraphicsQueueIdle();

	vkFreeCommandBuffers(m_Device, m_GraphicsCommandPool, 1, &commandBuffer);
}

VkCommandPool Device::GetOrCreateThreadLocalGraphicsPool() {
	// 1 pool per thread

	auto queueFamiliesIndices = FindQueueFamilies(m_PhysicalDevice);
	auto id = std::this_thread::get_id();

	{
		std::lock_guard<std::mutex> lock(m_ThreadPoolMapMutex);
		auto it = m_ThreadPools.find(id);
		if (it != m_ThreadPools.end()) {
			return it->second.pool;
		}
	}

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamiliesIndices.m_GraphicsFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkCommandPool pool = nullptr;
	vkCreateCommandPool(m_Device, &poolInfo, nullptr, &pool);

	std::lock_guard<std::mutex> lock(m_ThreadPoolMapMutex);
	m_ThreadPools[id].pool = pool;
	return pool;
}

void Device::CopyBuffer(const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size) {
	ExecuteGraphicsCommands([&](VkCommandBuffer cmd) {
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);
	});
}

void Device::SetObjectDebugName(const VkObjectType objectType, const uint64 handle, const char *name) const {
	if (!enableValidationLayers) {
		return;
	}

	VkDebugUtilsObjectNameInfoEXT nameInfo{};
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType = objectType;
	nameInfo.objectHandle = handle;
	nameInfo.pObjectName = name;

	const auto func = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
		vkGetInstanceProcAddr(m_VulkanInstance, "vkSetDebugUtilsObjectNameEXT"));
	if (func != nullptr) {
		func(m_Device, &nameInfo);
	}
}

} // namespace Aquila::Graphics
