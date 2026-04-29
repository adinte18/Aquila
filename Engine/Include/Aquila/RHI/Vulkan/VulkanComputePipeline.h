#ifndef AQUILA_VULKAN_COMPUTE_PIPELINE_H
#define AQUILA_VULKAN_COMPUTE_PIPELINE_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHIPipeline.h"

namespace Aquila::RHI {

class VulkanDevice;
class VulkanCommandList;

class VulkanComputePipeline final : public IRHIPipeline {
  public:
	VulkanComputePipeline(VulkanDevice &device, VkShaderModule module, const std::string &entryPoint,
						  VkPipelineLayout layout);
	~VulkanComputePipeline() override;

	AQUILA_NONCOPYABLE(VulkanComputePipeline);

	// IRHIPipeline
	void Bind(IRHICommandList &cmd) override;

	// Vulkan-specific
	void Bind(VulkanCommandList &cmd) const;
	void Dispatch(VulkanCommandList &cmd, uint32 x, uint32 y, uint32 z) const;

	[[nodiscard]] VkPipeline GetPipeline() const { return m_Pipeline; }

  private:
	void CreatePipelineCache();

	VulkanDevice &m_Device;
	VkPipeline m_Pipeline = VK_NULL_HANDLE;
	VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;
};

} // namespace Aquila::RHI
#endif
