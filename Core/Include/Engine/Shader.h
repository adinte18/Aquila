#ifndef VK_APP_SHADER_H_
#define VK_APP_SHADER_H_

#include <fstream>
#include <vector>
#include <vulkan/vulkan.h>
#include <cstdlib>
#include <iostream>

#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>

class Shader {
public:
    static std::vector<char> ReadFile(const std::string& filename);
    static std::vector<std::uint32_t> CompileShaderGlslang(EShLanguage shaderType, const std::string& filename);

    static VkShaderModule CreateShaderModule(const std::vector<char>& code, VkDevice device);
    static VkShaderModule CreateShaderModule(const std::vector<uint32_t> &spirv, VkDevice device);
};

#endif
