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
	VulkanComputePipeline(VulkanDevice &device, VkShaderModule module, const std::string &entry_point,
						  VkPipelineLayout layout);
	~VulkanComputePipeline() override;

	AQUILA_NONCOPYABLE(VulkanComputePipeline);

	// IRHIPipeline
	void bind(IRHICommandList &cmd) override;
	[[nodiscard]] PipelineBindPoint get_bind_point() const override { return PipelineBindPoint::Compute; }

	// Vulkan-specific
	[[nodiscard]] VkPipeline get_pipeline() const { return m_pipeline; }
	[[nodiscard]] VkPipelineLayout get_layout() const { return m_layout; }

  private:
	VulkanDevice &m_device;
	VkPipeline m_pipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_layout = VK_NULL_HANDLE;
};

} // namespace Aquila::RHI
#endif
