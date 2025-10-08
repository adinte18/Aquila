#ifndef AQUILA_SHADER_H
#define AQUILA_SHADER_H

#include "AquilaCore.h"
#include "Engine/Renderer/Device.h"

class Shader {
public:
  static std::vector<char> ReadFile(const std::string &filename);

  static VkShaderModule CreateShaderModule(const std::vector<char> &code,
                                           Engine::Device &device,
                                           const std::string &debugName);
  static VkShaderModule CreateShaderModule(const std::vector<uint32_t> &spirv,
                                           Engine::Device &device,
                                           const std::string &debugName);

  static void DestroyShaderModule(VkShaderModule &module,
                                  Engine::Device &device);

private:
  std::string moduleName;
};

#endif
