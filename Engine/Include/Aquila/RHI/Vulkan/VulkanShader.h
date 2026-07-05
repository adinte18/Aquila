#ifndef AQUILA_VULKAN_SHADER_H
#define AQUILA_VULKAN_SHADER_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::RHI {

class VulkanDevice;

class VulkanShader {
  public:
	static std::vector<char> read_file(const std::string &filename);

	static VkShaderModule create_shader_module(const std::vector<char> &code, VulkanDevice &device,
											 const std::string &debug_name);
	static VkShaderModule create_shader_module(const std::vector<Uint32> &spirv, VulkanDevice &device,
											 const std::string &debug_name);

	static void destroy_shader_module(VkShaderModule &module, VulkanDevice &device);
};

} // namespace Aquila::RHI
#endif
