#include "Aquila/RHI/Vulkan/VulkanShader.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

namespace Aquila::RHI {

std::vector<char> VulkanShader::ReadFile(const std::string &filename) {
	auto file = Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(filename, AccessMode::Read, OpenMode::Binary);
	if (!file || !file->IsValid()) {
		throw std::runtime_error("Failed to open file: " + filename);
	}

	const int64 size = file->Size();
	std::vector<char> buffer(static_cast<size_t>(size));
	file->Read(buffer.data(), static_cast<size_t>(size));
	return buffer;
}

VkShaderModule VulkanShader::CreateShaderModule(const std::vector<uint32> &spirv, VulkanDevice &device,
												const std::string &debugName) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirv.size() * sizeof(uint32);
	createInfo.pCode = spirv.data();

	VkShaderModule shaderModule = nullptr;
	if (vkCreateShaderModule(device.GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	device.SetObjectDebugName(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64>(shaderModule), debugName.c_str());
	return shaderModule;
}

VkShaderModule VulkanShader::CreateShaderModule(const std::vector<char> &code, VulkanDevice &device,
												const std::string &debugName) {
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

void VulkanShader::DestroyShaderModule(VkShaderModule &module, VulkanDevice &device) {
	vkDestroyShaderModule(device.GetDevice(), module, nullptr);
}

} // namespace Aquila::RHI
