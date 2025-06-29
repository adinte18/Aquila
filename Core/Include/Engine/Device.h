#ifndef VK_APP_Device_H
#define VK_APP_Device_H

#include "AquilaCore.h"

#include "Engine/Window.h"

#include <vector>
#include <optional>
#include <iostream>
#include <set>

namespace Engine {
    struct VkSwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct VkQueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        [[nodiscard]] bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    class Device {
    private :
        VkInstance vkInstance{};
        VkDebugUtilsMessengerEXT debugMessenger{};
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // Graphics card
        VkCommandPool commandPool{};

        VkDevice device; // Logical device
        VkSurfaceKHR surface{};
        VkQueue graphicsQueue{};
        VkQueue presentQueue{};

        Window &window;

        const std::vector<const char *> validationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };

        const std::vector<const char *> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                "VK_KHR_maintenance1"
        };


        void vk_CreateInstance();

        void vk_SetupDebugMessenger();

        void vk_PickPhysicalDevice();

        void vk_CreateLogicalDevice();

        bool vk_CheckDeviceExtensionSupport(VkPhysicalDevice vkPhysicalDevice);

        VkSwapChainSupportDetails vk_QuerySwapChainSupport(VkPhysicalDevice vkPhysicalDevice);

        bool vk_CheckValidationLayerSupport();

        static VkResult vk_CreateDebugMessengerEXT(VkInstance instance,
                                                   const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator,
                                                   VkDebugUtilsMessengerEXT *pDebugMessenger);

        static void vk_DestroyDebugMessengerEXT(VkInstance instance,
                                                VkDebugUtilsMessengerEXT debugMessenger,
                                                const VkAllocationCallbacks *pAllocator);

        static void vk_PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

        [[nodiscard]] std::vector<const char *> vk_GetRequiredExtensions() const;


        /*
         * The first parameter specifies the severity of the message, which is one of the following flags:

            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Informational message like the creation of a resource
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Message about behavior that is not necessarily an error,
                                                             but very likely a bug in your application
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Message about behavior that is invalid and may cause crashes
         */
        static VKAPI_ATTR VkBool32 VKAPI_CALL vk_DebugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) {

            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

            return VK_FALSE;
        }

        VkQueueFamilyIndices vk_FindQueueFamilies(VkPhysicalDevice vkPhysicalDevice);

        bool IsSuitable(VkPhysicalDevice vkPhysicalDevice);

    public :
        VkPhysicalDeviceProperties properties{};


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

        VkCommandPool vk_GetCommandPool() {
            return commandPool;
        };
        VkDevice vk_GetDevice() {
            return device;
        };
        VkSurfaceKHR vk_GetSurface() {
            return surface;
        };
        VkQueue vk_GetGraphicsQueue() {
            return graphicsQueue;
        };
        VkQueue vk_GetPresentQueue() {
            return presentQueue;
        };

        [[nodiscard]] VkPhysicalDevice vk_GetPhysicalDevice() const { return physicalDevice; }
        [[nodiscard]] VkInstance vk_GetInstance() const { return vkInstance; }

        VkQueueFamilyIndices vk_FindPhysicalQF() { return vk_FindQueueFamilies(physicalDevice); }

        VkSwapChainSupportDetails vk_GetSwapChainSupport() { return vk_QuerySwapChainSupport(physicalDevice); }

        void vk_CreateComandPool();

        void
        vk_CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
                        VkDeviceMemory &bufferMemory);

        uint32_t vk_FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        void
        vk_CreateImageWithInfo(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags properties, VkImage &image,
                               VkDeviceMemory &imageMemory);

        VkFormat vk_FindSupportedFormat(
                const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        VkCommandBuffer vk_BeginSingleTimeCommands();
        void vk_EndSingleTimeCommands(VkCommandBuffer commandBuffer);

        void vk_CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    };
}


#endif //VK_APP_Device_H
