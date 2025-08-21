//
// Created by adinte on 6/28/24.
//

#include "Engine/Renderer/Shader.h"

std::vector<char> Shader::ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
//
// std::vector<std::uint32_t> Shader::CompileShaderGlslang(EShLanguage shaderType, const std::string& filename) {
//     glslang::InitializeProcess();
//
//     glslang::TShader shader(shaderType);
//
//     const std::vector<char>& shaderCode = Shader::ReadFile(filename);
//     const char* str = shaderCode.data();
//     shader.setStrings(&str, 1);
//
//     shader.setEnvInput(glslang::EShSource::EShSourceGlsl,
//                        shaderType,
//                        glslang::EShClient::EShClientVulkan,
//                        glslang::EshTargetClientVersion::EShTargetVulkan_1_4);
//
//     shader.setEnvClient(glslang::EShClient::EShClientVulkan,
//                         glslang::EshTargetClientVersion::EShTargetVulkan_1_4);
//
//     shader.setEnvTarget(glslang::EShTargetLanguage::EshTargetSpv,
//                         glslang::EShTargetLanguageVersion::EShTargetSpv_1_0);
//
//     auto includer = glslang::TShader::ForbidIncluder{};
//     const TBuiltInResource* resources = GetDefaultResources();
//
//     glslang::TProgram program;
//     program.addShader(&shader);
//
//     if (!shader.parse(resources,
//                       100,
//                       false,
//                       EShMsgDefault))
//     {
//         std::cerr << "Shader preprocessing failed: " << shader.getInfoLog() << std::endl;
//         glslang::FinalizeProcess();
//         throw std::runtime_error("Shader preprocessing failed.");
//     }
//
//     if (!program.link(EShMessages::EShMsgDefault)) {
//         throw std::runtime_error("Failed to link shader program.");
//     }
//
//     std::vector<std::uint32_t> spirv;
//     glslang::GlslangToSpv(*program.getIntermediate(shaderType), spirv);
//     glslang::FinalizeProcess();
//
//     return spirv;
// }
//

VkShaderModule Shader::CreateShaderModule(const std::vector<uint32_t>& spirv, VkDevice device) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(unsigned int);
    createInfo.pCode = spirv.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

VkShaderModule Shader::CreateShaderModule(const std::vector<char>& code, VkDevice device) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}