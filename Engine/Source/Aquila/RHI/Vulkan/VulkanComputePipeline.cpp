#include "Aquila/RHI/Vulkan/VulkanComputePipeline.h"
#include "Aquila/RHI/Vulkan/VulkanDeletionQueue.h"

#include "Aquila/RHI/Vulkan/VulkanCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

VulkanComputePipeline::VulkanComputePipeline(VulkanDevice &device, VkShaderModule module, const std::string &entry_point,
											 VkPipelineLayout layout)
	: m_device(device), m_layout(layout) {
	AQUILA_ASSERT(module != VK_NULL_HANDLE, "VulkanComputePipeline requires a valid VkShaderModule");
	AQUILA_ASSERT(layout != VK_NULL_HANDLE, "VulkanComputePipeline requires a valid VkPipelineLayout");

	VkPipelineShaderStageCreateInfo stage_info{};
	stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage_info.module = module;
	stage_info.pName = entry_point.c_str();

	VkComputePipelineCreateInfo pipeline_info{};
	pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeline_info.stage = stage_info;
	pipeline_info.layout = layout;

	AQUILA_VULKAN_CHECK(vkCreateComputePipelines(m_device.get_device(), m_device.get_pipeline_cache(), 1, &pipeline_info,
												 nullptr, &m_pipeline));
}

VulkanComputePipeline::~VulkanComputePipeline() {
	auto &dq = m_device.get_deletion_queue();
	if (m_pipeline != VK_NULL_HANDLE) {
		dq.queue_deletion(m_pipeline);
		m_pipeline = VK_NULL_HANDLE;
	}
	if (m_layout != VK_NULL_HANDLE) {
		dq.queue_deletion(m_layout);
		m_layout = VK_NULL_HANDLE;
	}
}

void VulkanComputePipeline::bind(IRHICommandList &cmd) {
	vkCmdBindPipeline(static_cast<VulkanCommandList &>(cmd).get_handle(), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
}

} // namespace Aquila::RHI
