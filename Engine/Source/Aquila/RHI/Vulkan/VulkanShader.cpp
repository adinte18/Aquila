#include "Aquila/RHI/Vulkan/VulkanShader.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

namespace Aquila::RHI {

std::vector<char> VulkanShader::read_file(const std::string &filename) {
	auto file = Platform::Filesystem::VirtualFileSystem::get()->open_file(filename, AccessMode::Read, OpenMode::Binary);
	if (!file || !file->is_valid()) {
		throw std::runtime_error("Failed to open file: " + filename);
	}

	const Int64 size = file->size();
	std::vector<char> buffer(static_cast<size_t>(size));
	file->read(buffer.data(), static_cast<size_t>(size));
	return buffer;
}

VkShaderModule VulkanShader::create_shader_module(const std::vector<Uint32> &spirv, VulkanDevice &device,
												const std::string &debug_name) {
	VkShaderModuleCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = spirv.size() * sizeof(Uint32);
	create_info.pCode = spirv.data();

	VkShaderModule shader_module = nullptr;
	if (vkCreateShaderModule(device.get_device(), &create_info, nullptr, &shader_module) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	device.set_object_debug_name(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<Uint64>(shader_module), debug_name.c_str());
	return shader_module;
}

VkShaderModule VulkanShader::create_shader_module(const std::vector<char> &code, VulkanDevice &device,
												const std::string &debug_name) {
	VkShaderModuleCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

	VkShaderModule shader_module = nullptr;
	if (vkCreateShaderModule(device.get_device(), &create_info, nullptr, &shader_module) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	device.set_object_debug_name(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<Uint64>(shader_module), debug_name.c_str());
	return shader_module;
}

void VulkanShader::destroy_shader_module(VkShaderModule &module, VulkanDevice &device) {
	vkDestroyShaderModule(device.get_device(), module, nullptr);
}

} // namespace Aquila::RHI
