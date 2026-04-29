#ifndef AQUILA_VULKAN_SHADER_H
#define AQUILA_VULKAN_SHADER_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::RHI {

class VulkanDevice;

class VulkanShader {
  public:
	static std::vector<char> ReadFile(const std::string &filename);

	static VkShaderModule CreateShaderModule(const std::vector<char> &code, VulkanDevice &device,
											 const std::string &debugName);
	static VkShaderModule CreateShaderModule(const std::vector<uint32> &spirv, VulkanDevice &device,
											 const std::string &debugName);

	static void DestroyShaderModule(VkShaderModule &module, VulkanDevice &device);
};

} // namespace Aquila::RHI
#endif
