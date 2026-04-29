#ifndef AQUILA_VULKAN_PIPELINE_H
#define AQUILA_VULKAN_PIPELINE_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHIPipeline.h"
#include "Aquila/RHI/Vulkan/VulkanVertex.h"

namespace Aquila::RHI {

class VulkanDevice;
class VulkanCommandList;

struct VulkanPipelineConfig {
	VulkanPipelineConfig() = default;
	AQUILA_NONCOPYABLE(VulkanPipelineConfig);

	std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
	VkPipelineViewportStateCreateInfo viewportInfo{};
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
	VkPipelineMultisampleStateCreateInfo multisampleInfo{};
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{};
	VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	std::vector<VkDynamicState> dynamicStateEnables{};
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	std::optional<VertexBindingDesc> customVertexLayout;
	const void *pNext = nullptr;
	VkRenderPass renderPass = VK_NULL_HANDLE;
	std::vector<VkFormat> colorFormats;
	VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	VkFormat stencilFormat = VK_FORMAT_UNDEFINED;
};

class VulkanPipeline final : public IRHIPipeline {
  public:
	VulkanPipeline(VulkanDevice &device, const std::vector<VkPipelineShaderStageCreateInfo> &stages,
				   const VulkanPipelineConfig &configInfo);
	~VulkanPipeline() override;

	AQUILA_NONCOPYABLE(VulkanPipeline);

	// IRHIPipeline
	void Bind(IRHICommandList &cmd) override;

	// Vulkan-specific
	void Bind(VulkanCommandList &cmd) const;
	[[nodiscard]] VkPipeline GetPipeline() const { return m_GraphicsPipeline; }
	[[nodiscard]] VkPipelineLayout GetLayout() const { return m_Layout; }

	static void DefaultPipelineConfig(VulkanPipelineConfig &configInfo);

  private:
	void CreatePipelineFromStages(const std::vector<VkPipelineShaderStageCreateInfo> &stages,
								  const VulkanPipelineConfig &configInfo);
	void CreatePipelineCache();

	VulkanDevice &m_Device;
	VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_Layout = VK_NULL_HANDLE;
	VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;
};

} // namespace Aquila::RHI
#endif
