#ifndef VK_APP_SHADER_H_
#define VK_APP_SHADER_H_

#include <fstream>
#include <vector>
#include <vulkan/vulkan.h>

class Shader {
public:
    static std::vector<char> ReadFile(const std::string& filename);
    static VkShaderModule CreateShaderModule(const std::vector<char>& code, VkDevice device);
};

#endif
