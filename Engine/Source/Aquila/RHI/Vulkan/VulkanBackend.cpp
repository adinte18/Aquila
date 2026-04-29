#include "Aquila/RHI/RHIBackend.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

Unique<IRHIDevice> CreateVulkanBackend(GLFWwindow &nativeWindow) {
	return CreateUnique<VulkanDevice>(nativeWindow);
}

} // namespace Aquila::RHI
