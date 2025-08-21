#ifndef VK_APP_Device_H
#define VK_APP_Device_H

#include "AquilaCore.h"

#include "Defines.h"
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


        static const char* GetObjectTypeName(VkObjectType objectType) {
            switch (objectType) {
                case VK_OBJECT_TYPE_BUFFER: return "Buffer";
                case VK_OBJECT_TYPE_IMAGE: return "Image";
                case VK_OBJECT_TYPE_IMAGE_VIEW: return "ImageView";
                case VK_OBJECT_TYPE_SAMPLER: return "Sampler";
                case VK_OBJECT_TYPE_DESCRIPTOR_SET: return "DescriptorSet";
                case VK_OBJECT_TYPE_FRAMEBUFFER: return "Framebuffer";
                case VK_OBJECT_TYPE_COMMAND_BUFFER: return "CommandBuffer";
                case VK_OBJECT_TYPE_PIPELINE: return "Pipeline";

                default: return "Unknown";
            }
        }


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

                const char* severityColor = "";
                const char* severityText = "";
                const char* resetColor = "\033[0m";
                
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

                std::string typeStr = "";
                if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) typeStr += "GENERAL";
                if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) typeStr += "VALIDATION";
                if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) typeStr += "PERFORMANCE";
                
                std::cerr << severityColor << "[VULKAN " << severityText << "]" << resetColor;
                if (!typeStr.empty()) {
                    std::cerr << " [" << typeStr << "]";
                }
                std::cerr << std::endl;
                
                if (pCallbackData->pMessageIdName) {
                    std::cerr << "  ID: " << pCallbackData->pMessageIdName << " (" << pCallbackData->messageIdNumber << ")" << std::endl;
                }
                
                std::cerr << "  Message: " << pCallbackData->pMessage << std::endl;
                
                if (pCallbackData->objectCount > 0) {
                    std::cerr << "  Objects involved:" << std::endl;
                    for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
                        const auto& obj = pCallbackData->pObjects[i];
                        std::cerr << "    [" << i << "] " << GetObjectTypeName(obj.objectType);
                        std::cerr << " (0x" << std::hex << obj.objectHandle << std::dec << ")";
                        if (obj.pObjectName) {
                            std::cerr << " \"" << obj.pObjectName << "\"";
                        }
                        std::cerr << std::endl;
                    }
                }
                
                std::cerr << std::endl;
                
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

        AQUILA_NONCOPYABLE(Device);
        AQUILA_NONMOVEABLE(Device);

        VkCommandPool GetCommandPool() {
            return m_CommandPool;
        };
        VkDevice GetDevice() {
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

        void Wait() { vkDeviceWaitIdle(m_Device); }

        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    };
}


#endif //VK_APP_Device_H
