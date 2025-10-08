#ifndef AQUILA_SHADER_PROGRAM_H
#define AQUILA_SHADER_PROGRAM_H

#include "Material/MaterialParameters.h"
#include "Shader.h"

namespace Engine {
namespace Graphics {
struct ShaderStage {
  VkShaderStageFlagBits m_Stage;
  std::string m_Filepath;
  VkShaderModule m_Module = VK_NULL_HANDLE;
};

class ShaderProgram {
public:
  std::string m_Name;
  std::vector<ShaderStage> m_Stages;
  VkPipelineLayout m_Layout = VK_NULL_HANDLE;
  Ref<DescriptorSetLayout> m_DescriptorSetLayout;
  std::unordered_map<std::string, Engine::Material::ParameterType>
      m_ExpectedProperties;

  ShaderProgram(Device &device, const std::string &n)
      : m_Name(n), m_Device(device) {}

  ~ShaderProgram() { Cleanup(); }

  void AddStage(VkShaderStageFlagBits stage, const std::string &path) {
    auto code = Shader::ReadFile(path);
    VkShaderModule module = Shader::CreateShaderModule(code, m_Device, m_Name);
    m_Stages.push_back({stage, path, module});
  }

  void Cleanup() {

    for (auto &stage : m_Stages) {
      if (stage.m_Module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_Device.GetDevice(), stage.m_Module, nullptr);
        stage.m_Module = VK_NULL_HANDLE;
      }
    }
    m_Stages.clear();

    if (m_Layout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(m_Device.GetDevice(), m_Layout, nullptr);
      m_Layout = VK_NULL_HANDLE;
    }

    m_DescriptorSetLayout.reset();
  }

  bool IsValid() const { return !m_Stages.empty(); }

private:
  Device &m_Device;
};
} // namespace Graphics
} // namespace Engine

#endif