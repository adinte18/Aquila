#ifndef AQUILA_PIPELINE_H
#define AQUILA_PIPELINE_H

#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Graphics/Shader/Shader.h"
#include "Aquila/Graphics/Shader/ShaderCompiler.h"
#include "Aquila/Graphics/Shader/ShaderProgram.h"
#include "Aquila/Graphics/Core/DeletionManager.h"

namespace Aquila::Graphics::RenderingPipeline {

using namespace Aquila::Foundation;

struct PipelineConfigInfo {
	PipelineConfigInfo() = default;
	PipelineConfigInfo(const PipelineConfigInfo &) = delete;
	PipelineConfigInfo &operator=(const PipelineConfigInfo &) = delete;

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

	const void *pNext = nullptr;
	VkRenderPass renderPass = VK_NULL_HANDLE;

	std::vector<VkFormat> colorFormats;
	VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	VkFormat stencilFormat = VK_FORMAT_UNDEFINED;
};

class Pipeline {
  public:
	// From a pre-built ShaderProgram (used by the Material system).
	// ShaderProgram already owns the VkShaderModules; Pipeline does NOT take ownership.
	Pipeline(Device &device, const Shader::ShaderProgram &shader, const PipelineConfigInfo &configInfo);

	// From a Material. It reads render state (blend, cull, wireframe, depth) automatically.
	Pipeline(Device &device, const Ref<Material::Material> &material, const PipelineConfigInfo &configInfo);

	// From two pre-compiled SPIR-V files (.vert.spv + .frag.spv).
	// Used by all built-in rendering systems (lighting, shadows, gizmos, composite …).
	// Pipeline owns and destroys the two VkShaderModules it creates.
	Pipeline(Device &device, const std::string &vertSpvPath, const std::string &fragSpvPath,
			 const PipelineConfigInfo &configInfo);

	// From a single .slang source file.
	// ShaderCompiler compiles all [shader("...")] entry points at load time.
	// Entry points named "vertexMain" and "fragmentMain" are expected (but any names work
	// as long as the file declares exactly one vertex and one fragment entry point).
	// Pipeline owns and destroys the VkShaderModules it creates.
	Pipeline(Device &device, const std::string &slangFilePath, const PipelineConfigInfo &configInfo);

	~Pipeline();

	Pipeline(const Pipeline &) = delete;
	Pipeline &operator=(const Pipeline &) = delete;

	[[nodiscard]] VkPipeline GetPipeline() const;
	void Bind(VkCommandBuffer commandBuffer) const;

	static void DefaultPipelineConfig(PipelineConfigInfo &configInfo);

  private:
	// Shared Vulkan pipeline creation — all constructors ultimately call this.
	// `stages` must remain valid until vkCreateGraphicsPipelines returns.
	void CreatePipelineFromStages(const std::vector<VkPipelineShaderStageCreateInfo> &stages,
								  const PipelineConfigInfo &configInfo);

	// Loads a pre-compiled SPIR-V file and wraps it in a VkShaderModule.
	[[nodiscard]] VkShaderModule LoadSpvModule(const std::string &spvPath, const std::string &debugName);

	void CreatePipelineCache();
	static VkCullModeFlags GetVkCullMode(Material::CullMode mode);

	void CreateFromShaderProgram(const Shader::ShaderProgram &shader, const PipelineConfigInfo &configInfo);
	void CreateFromSpvPair(const std::string &vertPath, const std::string &fragPath,
						   const PipelineConfigInfo &configInfo);
	void CreateFromSlang(const std::string &slangPath, const PipelineConfigInfo &configInfo);
	void CreateFromMaterial(const Ref<Material::Material> &material, const PipelineConfigInfo &configInfo);

	Device &m_Device;

	VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
	VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;

	// Owned only when Pipeline created the modules itself (SpvPair / Slang paths).
	// NULL when modules are owned by ShaderProgram or Material.
	VkShaderModule m_VertexShaderModule = VK_NULL_HANDLE;
	VkShaderModule m_FragmentShaderModule = VK_NULL_HANDLE;
};

} // namespace Aquila::Graphics::RenderingPipeline

#endif // AQUILA_PIPELINE_H
