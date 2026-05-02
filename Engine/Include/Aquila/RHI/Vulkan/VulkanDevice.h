#ifndef AQUILA_VULKAN_DEVICE_H
#define AQUILA_VULKAN_DEVICE_H

#include "GraphicsPCH.h"
#include "GLFW/glfw3.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

#include "Aquila/RHI/Backend/IRHIDevice.h"
#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/RHI/Vulkan/VulkanTypes.h"
#include "Aquila/RHI/Vulkan/VulkanCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanFormatUtils.h"

namespace Aquila::RHI {

class DeletionQueue;
class VulkanDescriptorPool;
class VulkanBuffer;
class VulkanTexture;
class VulkanSwapchain;

class VulkanDevice final : public IRHIDevice {
  public:
#ifdef ENABLE_VALIDATION_LAYERS
	constexpr static bool enableValidationLayers = true;
#else
	constexpr static bool enableValidationLayers = false;
#endif

	explicit VulkanDevice(GLFWwindow &nativeWindow);
	~VulkanDevice() override;

	AQUILA_NONCOPYABLE(VulkanDevice);
	AQUILA_NONMOVEABLE(VulkanDevice);

	[[nodiscard]] Unique<IRHIBuffer> CreateBuffer(const BufferDesc &desc) override;
	[[nodiscard]] Unique<IRHITexture> CreateTexture(const TextureDesc &desc) override;
	[[nodiscard]] Unique<IRHICommandList> CreateCommandList(CommandListType type,
															const std::string &name = "") override;
	[[nodiscard]] Unique<IRHISwapchain> CreateSwapchain(const SwapchainDesc &desc) override;
	[[nodiscard]] Unique<IRHIPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc &desc) override;
	[[nodiscard]] Unique<IRHIPipeline> CreateComputePipeline(const ComputePipelineDesc &desc) override;
	[[nodiscard]] Unique<IRHIRenderPass> CreateRenderPass(const RHI::RenderPassDesc &desc) override;

	[[nodiscard]] Unique<IRHIDescriptorSetLayout>
	CreateDescriptorSetLayout(const DescriptorSetLayoutDesc &desc) override;
	[[nodiscard]] Unique<IRHIDescriptorSet> AllocateDescriptorSet(IRHIDescriptorSetLayout &layout) override;
	void CopyBuffer(IRHICommandList &cmd, IRHIBuffer &src, IRHIBuffer &dst, uint64 size, uint64 srcOffset = 0,
					uint64 dstOffset = 0) override;
	void Submit(IRHICommandList &cmd) override;
	void SubmitAndWait(IRHICommandList &cmd) override;
	void SubmitFrame(IRHICommandList &cmd, IRHISwapchain *swapchain, uint32 imageIndex) override;
	void PresentFrame(IRHISwapchain &swapchain, uint32 imageIndex,
					  vec4 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f }) override;
	void WaitIdle() override { vkDeviceWaitIdle(m_Device); }
	void ProcessPendingDeletions() override;

	void SubmitToGraphicsQueue(const VkSubmitInfo *submitInfo, VkFence fence);
	void SubmitToComputeQueue(const VkSubmitInfo *submitInfo, VkFence fence);
	void SubmitToTransferQueue(const VkSubmitInfo *submitInfo, VkFence fence);
	void WaitGraphicsQueueIdle();
	void WaitTransferQueueIdle();
	void Wait() const { vkDeviceWaitIdle(m_Device); }

	VkCommandPool GetOrCreateThreadLocalGraphicsPool();

	template <typename Func> void ExecuteGraphicsCommands(Func &&func) {
		VkCommandPool pool = GetOrCreateThreadLocalGraphicsPool();
		ExecuteSingleTimeCommands(pool, m_GraphicsQueue, m_GraphicsQueueMutex, std::forward<Func>(func));
	}

	template <typename Func> void ExecuteTransferCommands(Func &&func) {
		ExecuteSingleTimeCommands(m_TransferCommandPool, m_TransferQueue, m_TransferQueueMutex,
								  std::forward<Func>(func));
	}

	VkFence CreateFence(bool signaled = false);
	void WaitForFence(VkFence fence);
	void DestroyFence(VkFence fence);

	VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
								 VkFormatFeatureFlags features);

	void SetObjectDebugName(VkObjectType objectType, uint64 handle, const char *name) const;

	[[nodiscard]] PFN_vkCmdBeginDebugUtilsLabelEXT GetDebugBeginLabel() const { return m_vkCmdBeginDebugUtilsLabelEXT; }
	[[nodiscard]] PFN_vkCmdEndDebugUtilsLabelEXT GetDebugEndLabel() const { return m_vkCmdEndDebugUtilsLabelEXT; }

	void CreateGraphicsCommandPool();
	void CreateComputeCommandPool();
	void CreateTransferCommandPool();

	template <MemoryDomain Domain>
	BufferAllocation CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const char *debugName = nullptr) {
		AQUILA_ASSERT(size > 0, "Buffer size must be > 0");
		AQUILA_ASSERT(m_Allocator != VK_NULL_HANDLE, "VMA allocator is null");

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		if constexpr (Domain == MemoryDomain::GPU_ONLY) {
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			allocInfo.flags = 0;
		} else if constexpr (Domain == MemoryDomain::CPU_TO_GPU) {
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		} else if constexpr (Domain == MemoryDomain::GPU_TO_CPU) {
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		} else if constexpr (Domain == MemoryDomain::CPU_ONLY) {
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}

		BufferAllocation bufferAllocation{};
		VmaAllocationInfo vmaInfo{};
		AQUILA_VULKAN_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &bufferAllocation.buffer,
											&bufferAllocation.allocation, &vmaInfo));
		bufferAllocation.info = vmaInfo;
		bufferAllocation.mappedPtr = vmaInfo.pMappedData;

#ifdef AQUILA_DEBUG
		if (debugName && enableValidationLayers) {
			SetObjectDebugName(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(bufferAllocation.buffer), debugName);
		}
#endif
		return bufferAllocation;
	}

	template <MemoryDomain Domain>
	ImageAllocation CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
								uint32_t mipLevels = 1, uint32_t arrayLayers = 1,
								VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
								const char *debugName = nullptr) {
		AQUILA_ASSERT(m_Allocator != VK_NULL_HANDLE, "VMA allocator is null");
		AQUILA_ASSERT(width > 0 && height > 0, "extent must be > 0");

		constexpr bool isCPUAccessible = (Domain != MemoryDomain::GPU_ONLY);
		constexpr VkImageTiling tiling = isCPUAccessible ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;

		if constexpr (isCPUAccessible) {
			AQUILA_ASSERT(mipLevels == 1, "Linear tiling does not support mipmaps.");
			AQUILA_ASSERT(samples == VK_SAMPLE_COUNT_1_BIT, "Linear tiling does not support MSAA.");
			AQUILA_ASSERT(arrayLayers == 1, "Linear tiling array layer support is not guaranteed.");
		}

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = format;
		imageInfo.extent = { .width = width, .height = height, .depth = 1 };
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = arrayLayers;
		imageInfo.samples = samples;
		imageInfo.tiling = tiling;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = isCPUAccessible ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocInfo{};
		if constexpr (Domain == MemoryDomain::GPU_ONLY) {
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			allocInfo.flags = 0;
			allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		} else if constexpr (Domain == MemoryDomain::CPU_TO_GPU) {
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		} else if constexpr (Domain == MemoryDomain::GPU_TO_CPU) {
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		} else if constexpr (Domain == MemoryDomain::CPU_ONLY) {
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}

		ImageAllocation imageAllocation{};
		VmaAllocationInfo vmaInfo{};
		AQUILA_VULKAN_CHECK(vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &imageAllocation.image,
										   &imageAllocation.allocation, &vmaInfo));
		imageAllocation.info = vmaInfo;
		imageAllocation.mappedPtr = vmaInfo.pMappedData;
		imageAllocation.format = format;
		imageAllocation.extent = { .width = width, .height = height, .depth = 1 };
		imageAllocation.mipLevels = mipLevels;
		imageAllocation.arrayLayers = arrayLayers;

		if constexpr (isCPUAccessible) {
			VkImageSubresource sub{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
			VkSubresourceLayout layout{};
			vkGetImageSubresourceLayout(m_Device, imageAllocation.image, &sub, &layout);
			imageAllocation.info.size = layout.size;
		}

#ifdef AQUILA_DEBUG
		if (debugName && enableValidationLayers) {
			SetObjectDebugName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(imageAllocation.image), debugName);
		}
#endif
		return imageAllocation;
	}

	VkSampler GetOrCreateSampler(const SamplerDesc &desc);
	void DestroySamplerCache();

	[[nodiscard]] RHI::DeletionQueue &GetDeletionQueue() const;
	[[nodiscard]] VkCommandPool GetGraphicsCommandPool() const { return m_GraphicsCommandPool; }
	[[nodiscard]] VkCommandPool GetComputeCommandPool() const { return m_ComputeCommandPool; }
	[[nodiscard]] VkCommandPool GetTransferCommandPool() const { return m_TransferCommandPool; }
	[[nodiscard]] VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
	[[nodiscard]] VkQueue GetPresentQueue() const { return m_PresentQueue; }
	[[nodiscard]] VkQueue GetComputeQueue() const { return m_ComputeQueue; }
	[[nodiscard]] VkQueue GetTransferQueue() const { return m_TransferQueue; }
	[[nodiscard]] VkInstance GetInstance() const { return m_VulkanInstance; }
	[[nodiscard]] VkDevice &GetDevice() { return m_Device; }
	[[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
	[[nodiscard]] VkSurfaceKHR GetSurface() const { return m_Surface; }
	[[nodiscard]] VmaAllocator GetAllocator() const { return m_Allocator; }
	[[nodiscard]] VkQueueFamilyIndices FindPhysicalQF() const { return FindQueueFamilies(m_PhysicalDevice); }
	[[nodiscard]] VkSwapChainSupportDetails GetSwapChainSupport() const {
		return QuerySwapChainSupport(m_PhysicalDevice);
	}

  private:
	void CreateInstance();
	void SetupDebugMessenger();
	void InitializeVMA();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSurface();

	bool IsSuitable(VkPhysicalDevice vkPhysicalDevice);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice vkPhysicalDevice) const;
	bool CheckValidationLayerSupport() const;
	std::vector<const char *> GetRequiredExtensions() const;
	VkQueueFamilyIndices FindQueueFamilies(VkPhysicalDevice vkPhysicalDevice) const;
	VkSwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice vkPhysicalDevice) const;
	void LogDeviceInfo() const;

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

		// The lambda may take a VkCommandBuffer or a VulkanCommandList (via wrapper).
		// We wrap it temporarily so callers can do either.
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

	static VkResult CreateDebugMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
											const VkAllocationCallbacks *pAllocator,
											VkDebugUtilsMessengerEXT *pDebugMessenger);
	static void DestroyDebugMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
										 const VkAllocationCallbacks *pAllocator);
	static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
														VkDebugUtilsMessageTypeFlagsEXT messageType,
														const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
														void *pUserData);
	static const char *GetObjectTypeName(VkObjectType objectType);

	// Vulkan handles
	VkInstance m_VulkanInstance{};
	VkDebugUtilsMessengerEXT m_DebugMessenger{};
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device{};
	VkSurfaceKHR m_Surface{};
	VmaAllocator m_Allocator{};
	VkPhysicalDeviceProperties m_Properties{};
	PFN_vkSetDebugUtilsObjectNameEXT m_vkSetDebugUtilsObjectNameEXT = nullptr;
	PFN_vkCmdBeginDebugUtilsLabelEXT m_vkCmdBeginDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT m_vkCmdEndDebugUtilsLabelEXT = nullptr;

	// Queues
	VkQueue m_GraphicsQueue{};
	VkQueue m_PresentQueue{};
	VkQueue m_ComputeQueue{};
	VkQueue m_TransferQueue{};

	std::mutex m_GraphicsQueueMutex;
	std::mutex m_PresentQueueMutex;
	std::mutex m_ComputeQueueMutex;
	std::mutex m_TransferQueueMutex;

	// Command pools
	VkCommandPool m_GraphicsCommandPool{};
	VkCommandPool m_ComputeCommandPool{};
	VkCommandPool m_TransferCommandPool{};
	std::mutex m_TransferCommandPoolMutex;
	std::mutex m_GraphicsCommandPoolMutex;

	Unique<RHI::DeletionQueue> m_DeletionQueue;

	std::unordered_map<SamplerDesc, VkSampler, SamplerDescHash> m_SamplerCache;

	Unique<VulkanDescriptorPool> m_GlobalPool;
	void CreateGlobalDescriptorPool();
	void DestroyGlobalDescriptorPool();

	struct ThreadLocalPool {
		VkCommandPool pool = VK_NULL_HANDLE;
	};
	std::unordered_map<std::thread::id, ThreadLocalPool> m_ThreadPools;
	std::mutex m_ThreadPoolMapMutex;

	GLFWwindow &m_WindowHandle;

	const std::vector<const char *> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
														 VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
														 VK_KHR_MAINTENANCE1_EXTENSION_NAME };
};

} // namespace Aquila::RHI
#endif
