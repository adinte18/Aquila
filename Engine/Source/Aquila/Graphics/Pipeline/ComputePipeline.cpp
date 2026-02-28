#include "Aquila/Graphics/Pipeline/ComputePipeline.h"
#include "Aquila/Graphics/Shader/Shader.h"
#include "Aquila/Graphics/Shader/ShaderCompiler.h"

namespace Aquila::Graphics::RenderingPipeline {

ComputePipeline::ComputePipeline(Device &device, const Shader::ShaderProgram &program) : m_Device(device) {
	AQUILA_ASSERT(program.GetShaderType() == Shader::ShaderProgram::ShaderType::Compute,
				  "ComputePipeline requires a Compute ShaderProgram");
	AQUILA_ASSERT(!program.m_Stages.empty(), "ShaderProgram has no stages");

	const auto &stage = program.m_Stages[0];
	AQUILA_ASSERT(stage.stage == VK_SHADER_STAGE_COMPUTE_BIT, "First stage is not a compute stage");

	CreatePipelineCache();
	CreateFromStage(stage.module, stage.entryPointName, program.m_Layout);

	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64>(m_Pipeline), program.m_Name.c_str());
}

ComputePipeline::ComputePipeline(Device &device, const std::string &slangPath, VkPipelineLayout layout)
	: m_Device(device) {
	std::string errorLog;
	std::vector<Shader::CompiledStage> compiled;

	if (!Shader::ShaderCompiler::CompileFile(slangPath, compiled, errorLog)) {
		AQUILA_LOG_ERROR("ComputePipeline: Slang compilation failed for '{}': {}", slangPath, errorLog);
		return;
	}

	const Shader::CompiledStage *computeStage = nullptr;
	for (const auto &cs : compiled) {
		if (cs.stage == VK_SHADER_STAGE_COMPUTE_BIT) {
			computeStage = &cs;
			break;
		}
	}

	if (computeStage == nullptr) {
		AQUILA_LOG_ERROR("ComputePipeline: no compute stage found in '{}'", slangPath);
		return;
	}

	m_OwnedModule = Shader::Shader::CreateShaderModule(computeStage->spirv, m_Device,
													   slangPath + "::" + computeStage->entryPointName);

	CreatePipelineCache();
	CreateFromStage(m_OwnedModule, computeStage->entryPointName, layout);

	std::string debugName = slangPath.substr(slangPath.find_last_of("/\\") + 1);
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64>(m_Pipeline), debugName.c_str());

	AQUILA_LOG_INFO("ComputePipeline: created from Slang '{}'", slangPath);
}

ComputePipeline::~ComputePipeline() {
	if (m_Device.GetDevice() != nullptr) {
		if (m_OwnedModule != VK_NULL_HANDLE) {
			m_Device.GetDeletionManager().QueueDeletion(m_OwnedModule);
			m_OwnedModule = VK_NULL_HANDLE;
		}
		if (m_Pipeline != VK_NULL_HANDLE) {
			m_Device.GetDeletionManager().QueueDeletion(m_Pipeline);
			m_Pipeline = VK_NULL_HANDLE;
		}
		if (m_PipelineCache != VK_NULL_HANDLE) {
			m_Device.GetDeletionManager().QueueDeletion(m_PipelineCache);
			m_PipelineCache = VK_NULL_HANDLE;
		}
	} else {
		if (m_OwnedModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(m_Device.GetDevice(), m_OwnedModule, nullptr);
		}
		if (m_Pipeline != VK_NULL_HANDLE) {
			m_Device.Wait();
			vkDestroyPipeline(m_Device.GetDevice(), m_Pipeline, nullptr);
		}
		if (m_PipelineCache != VK_NULL_HANDLE) {
			vkDestroyPipelineCache(m_Device.GetDevice(), m_PipelineCache, nullptr);
		}
	}
}

void ComputePipeline::Bind(VkCommandBuffer cmd) const {
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
}

void ComputePipeline::Dispatch(VkCommandBuffer cmd, uint32 x, uint32 y, uint32 z) const {
	vkCmdDispatch(cmd, x, y, z);
}

void ComputePipeline::CreateFromStage(VkShaderModule module, const std::string &entryPoint, VkPipelineLayout layout) {
	AQUILA_ASSERT(layout != VK_NULL_HANDLE, "Pipeline layout must be set before creating compute pipeline");

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

void ComputePipeline::CreatePipelineCache() {
	VkPipelineCacheCreateInfo cacheInfo{};
	cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	AQUILA_VULKAN_CHECK(vkCreatePipelineCache(m_Device.GetDevice(), &cacheInfo, nullptr, &m_PipelineCache));
}

} // namespace Aquila::Graphics::RenderingPipeline
