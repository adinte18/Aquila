#include "Aquila/Rendering/Systems/ComputeTestSystem.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanShaderCompiler.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Rendering/SceneFrameData.h"
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Graphics/RenderGraph/RGPassBuilder.h"

namespace Aquila::Rendering {

using Aquila::SharedConstants::SHADERS_DIR;

static constexpr uint32 kElementCount = 3456; // cluster size so 16x9x24

void ClusterComputeSystem::OnInit(GFX::GfxContext &ctx) {
	RenderingSystemBase::OnInit(ctx);

	std::vector<RHI::VulkanCompiledStage> stages;
	std::string err;
	if (!RHI::VulkanShaderCompiler::CompileFile(SHADERS_DIR + "ClusterCompute.slang", stages, err)) {
		AQUILA_LOG_ERROR("ClusterComputeSystem: shader compile failed: {}", err);
		return;
	}

	m_GridData.grid.x = 16;
	m_GridData.grid.y = 9;
	m_GridData.grid.z = 24;

	m_StorageLayout = ctx.CreateDescriptorSetLayout({
		.bindings = { {
						  .binding = 0,
						  .type = RHI::DescriptorType::StorageBuffer,
						  .stages = RHI::ShaderStageFlags::Compute,
						  .count = 1,
					  },
					  {
						  .binding = 1,
						  .type = RHI::DescriptorType::UniformBuffer,
						  .stages = RHI::ShaderStageFlags::Compute,
						  .count = 1,
					  } },
	});

	RHI::ComputePipelineDesc pipelineDesc{};
	pipelineDesc.computeShader = {
		.stage = RHI::ShaderStageFlags::Compute,
		.spirv = stages[0].spirv,
		.entryPoint = stages[0].entryPointName,
	};
	pipelineDesc.setLayouts = { &SceneFrameData::Get()->GetLayout().GetRHI(), &m_StorageLayout->GetRHI() };
	pipelineDesc.debugName = "ClusterCompute";
	m_Pipeline = ctx.CreateComputePipeline(pipelineDesc);

	m_OutputBuffer = ctx.CreateBuffer({
		.size = sizeof(AABB) * kElementCount,
		.usage = RHI::BufferUsage::StorageBuffer,
		.domain = RHI::MemoryDomain::GPU_TO_CPU,
		.debugName = "ComputeTest_Output",
	});

	m_GridBuffer = ctx.CreateBuffer({
		.size = sizeof(GridData),
		.usage = RHI::BufferUsage::UniformBuffer,
		.domain = RHI::MemoryDomain::CPU_TO_GPU,
		.debugName = "ComputeTest_GridDAta",
	});

	m_GridBuffer->Write(&m_GridData);

	m_StorageSet = ctx.AllocateDescriptorSet(*m_StorageLayout);
	m_StorageSet->SetBuffer(0, *m_OutputBuffer).Flush();
	m_StorageSet->SetBuffer(1, *m_GridBuffer).Flush();
}

void ClusterComputeSystem::AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) {
	if (!m_Pipeline) {
		return;
	}

	auto *frameData = ctx.frameData;
	const uint32 frameSlot = ctx.frameSlot;

	graph.AddPass(
		"ClusterCompute", [](Graphics::RG::RGPassBuilder &builder) { builder.MarkAsSideEffect(); },
		[this, frameData, frameSlot](GFX::GfxCommandList &cmd, Graphics::RG::RGRegistry &) {
			cmd.BindPipeline(*m_Pipeline);
			cmd.BindDescriptorSet(0, frameData->GetDescriptorSet(frameSlot));
			cmd.BindDescriptorSet(1, *m_StorageSet);
			cmd.Dispatch((m_GridData.grid.x + 4 - 1) / 4, (m_GridData.grid.y + 4 - 1) / 4,
						 (m_GridData.grid.z + 4 - 1) / 4);

			if (!m_Verified) {
				auto *data = static_cast<uint32 *>(m_OutputBuffer->Map());
				if (data) {
					AQUILA_LOG_INFO("ClusterCompute: output[0]={} output[1]={} output[63]={}", data[0], data[1],
									data[63]);
					m_OutputBuffer->Unmap();
				}
				m_Verified = true;
			}
		});
}

} // namespace Aquila::Rendering
