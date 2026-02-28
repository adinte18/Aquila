// ComputePipeline.h
#ifndef AQUILA_COMPUTE_PIPELINE_H
#define AQUILA_COMPUTE_PIPELINE_H

#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Graphics/Shader/ShaderProgram.h"

namespace Aquila::Graphics::RenderingPipeline {

class ComputePipeline {
  public:
	ComputePipeline(Device &device, const Shader::ShaderProgram &program);

	ComputePipeline(Device &device, const std::string &slangPath, VkPipelineLayout layout);

	~ComputePipeline();

	ComputePipeline(const ComputePipeline &) = delete;
	ComputePipeline &operator=(const ComputePipeline &) = delete;

	void Bind(VkCommandBuffer cmd) const;
	void Dispatch(VkCommandBuffer cmd, uint32 x, uint32 y, uint32 z) const;

	[[nodiscard]] VkPipeline GetPipeline() const { return m_Pipeline; }

  private:
	void CreateFromStage(VkShaderModule module, const std::string &entryPoint, VkPipelineLayout layout);
	void CreatePipelineCache();

	Device &m_Device;

	VkPipeline m_Pipeline = VK_NULL_HANDLE;
	VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;

	VkShaderModule m_OwnedModule = VK_NULL_HANDLE;
};

} // namespace Aquila::Graphics::RenderingPipeline

#endif
