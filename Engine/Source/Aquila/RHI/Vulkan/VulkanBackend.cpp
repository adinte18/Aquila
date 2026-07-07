#include "Aquila/RHI/RHIBackend.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

Unique<IRHIDevice> create_vulkan_backend(GLFWwindow &native_window) {
	return std::make_unique<VulkanDevice>(native_window);
}

} // namespace Aquila::RHI
