#ifndef VK_APP_Device_H
#define VK_APP_Device_H

#include "AquilaCore.h"

#include "Engine/Window.h"

namespace Engine {
    struct VkSwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
        std::vector<VkSurfaceFormatKHR> m_Formats;
        std::vector<VkPresentModeKHR> m_PresentModes;
    };

    struct VkQueueFamilyIndices {
        std::optional<uint32_t> m_GraphicsFamily;
        std::optional<uint32_t> m_PresentFamily;

        [[nodiscard]] bool IsComplete() const {
            return m_GraphicsFamily.has_value() && m_PresentFamily.has_value();
        }
    };

    class Device {
    private :
        VkInstance m_VulkanInstance{};
        VkDebugUtilsMessengerEXT m_DebugMessenger{};
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE; // Graphics card
        VkCommandPool m_CommandPool{};

        VkDevice m_Device; // Logical device
        VkSurfaceKHR m_Surface{};
        VkQueue m_GraphicsQueue{};
        VkQueue m_PresentQueue{};

        Window &m_Window;

        const std::vector<const char *> validationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };

        const std::vector<const char *> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                "VK_KHR_maintenance1"
        };


        void CreateInstance();

        void SetupDebugMessenger();

        void PickPhysicalDevice();

        void CreateLogicalDevice();

        bool CheckDeviceExtensionSupport(VkPhysicalDevice vkPhysicalDevice);

        VkSwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice vkPhysicalDevice);

        bool CheckValidationLayerSupport();

        static VkResult CreateDebugMessengerEXT(VkInstance instance,
                                                   const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator,
                                                   VkDebugUtilsMessengerEXT *pDebugMessenger);

        static void DestroyDebugMessengerEXT(VkInstance instance,
                                                VkDebugUtilsMessengerEXT debugMessenger,
                                                const VkAllocationCallbacks *pAllocator);

        static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

        [[nodiscard]] std::vector<const char *> GetRequiredExtensions() const;


        /*
         * The first parameter specifies the severity of the message, which is one of the following flags:

            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Informational message like the creation of a resource
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Message about behavior that is not necessarily an error,
                                                             but very likely a bug in your application
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Message about behavior that is invalid and may cause crashes
         */
        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) {

            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

            return VK_FALSE;
        }

        VkQueueFamilyIndices FindQueueFamilies(VkPhysicalDevice vkPhysicalDevice);

        bool IsSuitable(VkPhysicalDevice vkPhysicalDevice);

    public :
        VkPhysicalDeviceProperties m_Properties{};


        #ifdef DEBUG
                const bool enableValidationLayers = true;
        #else
                const bool enableValidationLayers = false;
        #endif

        explicit Device(Window &window);
        ~Device();

        // Not copyable or movable
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;
        Device(const Device&&) = delete;
        Device& operator=(const Device&&) = delete;

        VkCommandPool GetCommandPool() {
            return m_CommandPool;
        };
        VkDevice vk_GetDevice() {
            return m_Device;
        };
        VkSurfaceKHR GetSurface() {
            return m_Surface;
        };
        VkQueue GetGraphicsQueue() {
            return m_GraphicsQueue;
        };
        VkQueue GetPresentQueue() {
            return m_PresentQueue;
        };

        [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        [[nodiscard]] VkInstance GetInstance() const { return m_VulkanInstance; }

        VkQueueFamilyIndices FindPhysicalQF() { return FindQueueFamilies(m_PhysicalDevice); }

        VkSwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(m_PhysicalDevice); }

        void CreateComandPool();

        void
        CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
                        VkDeviceMemory &bufferMemory);

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        void
        CreateImageWithInfo(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags properties, VkImage &image,
                               VkDeviceMemory &imageMemory);

        VkFormat FindSupportedFormat(
                const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        VkCommandBuffer BeginSingleTimeCommands();
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    };
}


#endif //VK_APP_Device_H
