#include "Aquila/RHI/Vulkan/VulkanComputePipeline.h"
#include "Aquila/RHI/Vulkan/VulkanDeletionQueue.h"

#include "Aquila/RHI/Vulkan/VulkanCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"

namespace Aquila::RHI {

VulkanComputePipeline::VulkanComputePipeline(VulkanDevice &device, VkShaderModule module, const std::string &entryPoint,
											 VkPipelineLayout layout)
	: m_Device(device) {
	AQUILA_ASSERT(module != VK_NULL_HANDLE, "VulkanComputePipeline requires a valid VkShaderModule");
	AQUILA_ASSERT(layout != VK_NULL_HANDLE, "VulkanComputePipeline requires a valid VkPipelineLayout");

	CreatePipelineCache();

	VkPipelineShaderStageCreateInfo stageInfo{};
	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageInfo.module = module;
	stageInfo.pName = entryPoint.c_str();

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = stageInfo;
	pipelineInfo.layout = layout;

	AQUILA_VULKAN_CHECK(
		vkCreateComputePipelines(m_Device.GetDevice(), m_PipelineCache, 1, &pipelineInfo, nullptr, &m_Pipeline));
}

VulkanComputePipeline::~VulkanComputePipeline() {
	if (m_Pipeline != VK_NULL_HANDLE) {
		m_Device.GetDeletionQueue().QueueDeletion(m_Pipeline);
		m_Pipeline = VK_NULL_HANDLE;
	}
	if (m_PipelineCache != VK_NULL_HANDLE) {
		m_Device.GetDeletionQueue().QueueDeletion(m_PipelineCache);
		m_PipelineCache = VK_NULL_HANDLE;
	}
}

void VulkanComputePipeline::Bind(IRHICommandList &cmd) {
	vkCmdBindPipeline(static_cast<VulkanCommandList &>(cmd).GetHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
}

void VulkanComputePipeline::Bind(VulkanCommandList &cmd) const {
	vkCmdBindPipeline(cmd.GetHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
}

void VulkanComputePipeline::Dispatch(VulkanCommandList &cmd, uint32 x, uint32 y, uint32 z) const {
	vkCmdDispatch(cmd.GetHandle(), x, y, z);
}

void VulkanComputePipeline::CreatePipelineCache() {
	VkPipelineCacheCreateInfo cacheInfo{};
	cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	AQUILA_VULKAN_CHECK(vkCreatePipelineCache(m_Device.GetDevice(), &cacheInfo, nullptr, &m_PipelineCache));
}

} // namespace Aquila::RHI
