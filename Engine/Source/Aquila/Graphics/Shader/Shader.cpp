//
// Created by adinte on 6/28/24.
//

#include "Aquila/Graphics/Shader/Shader.h"
#include "Aquila/Core/AquilaCore.h"

namespace Aquila::Graphics::Shader {
std::vector<char> Shader::ReadFile(const std::string &filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filename);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

VkShaderModule Shader::CreateShaderModule(const std::vector<uint32_t> &spirv, Device &device,
										  const std::string &debugName) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirv.size() * sizeof(unsigned int);
	createInfo.pCode = spirv.data();

	VkShaderModule shaderModule = nullptr;
	if (vkCreateShaderModule(device.GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	device.SetObjectDebugName(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64>(shaderModule), debugName.c_str());

	return shaderModule;
}

VkShaderModule Shader::CreateShaderModule(const std::vector<char> &code, Device &device, const std::string &debugName) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

	VkShaderModule shaderModule = nullptr;
	if (vkCreateShaderModule(device.GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	device.SetObjectDebugName(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64>(shaderModule), debugName.c_str());

	return shaderModule;
}

void Shader::DestroyShaderModule(VkShaderModule &module, Device &device) {
	vkDestroyShaderModule(device.GetDevice(), module, nullptr);
}

} // namespace Aquila::Graphics::Shader
