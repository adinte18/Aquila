#ifndef AQUILA_RHI_BACKEND_H
#define AQUILA_RHI_BACKEND_H

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHIDevice.h"

struct GLFWwindow;

namespace Aquila::RHI {


// Creates a Vulkan backend device. The window is used to create the surface.
[[nodiscard]] Unique<IRHIDevice> CreateVulkanBackend(GLFWwindow &nativeWindow);

} // namespace Aquila::RHI
#endif
