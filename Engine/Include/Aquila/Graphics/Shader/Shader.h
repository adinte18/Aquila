#ifndef AQUILA_SHADER_H
#define AQUILA_SHADER_H

#include "Aquila/Core/AquilaCore.h"
#include "Aquila/Graphics/Core/Device.h"

namespace Aquila::Graphics::Shader {
class Shader {
  public:
	static std::vector<char> ReadFile(const std::string &filename);

	static VkShaderModule CreateShaderModule(const std::vector<char> &code, Device &device,
											 const std::string &debugName);
	static VkShaderModule CreateShaderModule(const std::vector<uint32> &spirv, Device &device,
											 const std::string &debugName);

	static void DestroyShaderModule(VkShaderModule &module, Device &device);

  private:
	std::string moduleName;
};
} // namespace Aquila::Graphics::Shader
#endif
