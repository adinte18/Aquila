#ifndef AQUILA_COMPUTE_SYSTEM_BASE_H
#define AQUILA_COMPUTE_SYSTEM_BASE_H

#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Graphics/Shader/ShaderProgram.h"
#include "Aquila/Graphics/Pipeline/ComputePipeline.h"
#include "Aquila/Scene/Scene.h"

namespace Aquila::Rendering::Systems {

using namespace Aquila::Graphics;
using namespace Aquila::Graphics::RenderingPipeline;
using namespace Aquila::Graphics::Resources;
using namespace Aquila::Graphics::Helpers;

class ComputeSystemBase {
	struct ComputeSpec {
		SceneManagement::Scene *scene = nullptr;
		uint32 frameIndex = 0;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	};

  public:
	ComputeSystemBase(Device &device) : m_Device(device) {}
	virtual ~ComputeSystemBase() { Cleanup(); }

	virtual void OnDispatch(const ComputeSpec &spec) = 0;
	virtual void OnUpdate(const ComputeSpec &spec) {}

  protected:
	void BuildPipeline(Aquila::Graphics::Shader::ShaderProgram &program) {
		VkComputePipelineCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		info.layout = program.m_Layout;
		info.stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
					   .module = program.m_Stages[0].module,
					   .pName = program.m_Stages[0].entryPointName.c_str() };

		AQUILA_VULKAN_CHECK(
			vkCreateComputePipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &info, nullptr, &m_Pipeline));
	}

	void BindAndDispatch(VkCommandBuffer cmd, VkDescriptorSet set, VkPipelineLayout layout, uint32 x, uint32 y,
						 uint32 z) {
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &set, 0, nullptr);
		vkCmdDispatch(cmd, x, y, z);
	}

	void Cleanup() {
		if (m_Pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(m_Device.GetDevice(), m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}
	}

	Device &m_Device;
	VkPipeline m_Pipeline = VK_NULL_HANDLE;
};
} // namespace Aquila::Rendering::Systems

#endif
