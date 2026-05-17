#include "Aquila/Rendering/Systems/LightCullingSystem.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/Graphics/RenderGraph/RGTypes.h"
#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/RHI/Vulkan/VulkanShaderCompiler.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Rendering/SceneFrameData.h"
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Graphics/RenderGraph/RGPassBuilder.h"

namespace Aquila::Rendering {

using Aquila::SharedConstants::SHADERS_DIR;

void LightCullingSystem::OnInit(GFX::GfxContext &ctx) {
	RenderingSystemBase::OnInit(ctx);

	std::vector<RHI::VulkanCompiledStage> stages;
	std::string err;
	if (!RHI::VulkanShaderCompiler::CompileFile(SHADERS_DIR + "LightCullCompute.slang", stages, err)) {
		AQUILA_LOG_ERROR("LightCullingSystem: shader compile failed: {}", err);
		return;
	}

	m_StorageLayout = ctx.CreateDescriptorSetLayout({
		.bindings = {
			{ .binding = 0, .type = RHI::DescriptorType::StorageBuffer, .stages = RHI::ShaderStageFlags::Compute, .count = 1 },
			{ .binding = 1, .type = RHI::DescriptorType::StorageBuffer, .stages = RHI::ShaderStageFlags::Compute, .count = 1 },
			{ .binding = 2, .type = RHI::DescriptorType::StorageBuffer, .stages = RHI::ShaderStageFlags::Compute, .count = 1 },
			{ .binding = 3, .type = RHI::DescriptorType::StorageBuffer, .stages = RHI::ShaderStageFlags::Compute, .count = 1 },
		},
	});

	RHI::ComputePipelineDesc pipelineDesc{};
	pipelineDesc.computeShader = {
		.stage = RHI::ShaderStageFlags::Compute,
		.spirv = stages[0].spirv,
		.entryPoint = stages[0].entryPointName,
	};
	pipelineDesc.setLayouts = { &SceneFrameData::Get()->GetLayout().GetRHI(), &m_StorageLayout->GetRHI() };
	pipelineDesc.debugName = "LightCull";
	m_Pipeline = ctx.CreateComputePipeline(pipelineDesc);

	m_GlobalIndexCounter = ctx.CreateBuffer({
		.size = sizeof(uint32),
		.usage = RHI::BufferUsage::StorageBuffer,
		.domain = RHI::MemoryDomain::GPU_ONLY,
		.debugName = "GlobalIndexCounter",
	});

	auto *sfd = SceneFrameData::Get();
	m_StorageSet = ctx.AllocateDescriptorSet(*m_StorageLayout);
	m_StorageSet->SetBuffer(1, sfd->GetLightIndexListBuffer())
		.SetBuffer(2, sfd->GetClusterLightInfoBuffer())
		.SetBuffer(3, *m_GlobalIndexCounter)
		.Flush();
}

void LightCullingSystem::AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) {
	if (!m_Pipeline) {
		return;
	}

	auto *frameData = ctx.frameData;
	const uint32 frameSlot = ctx.frameSlot;

	if (ctx.hClusterAABBs.IsValid() && !m_AABBBufferBound) {
		GFX::GfxBuffer &aabbBuffer = graph.GetRegistry().GetBuffer(ctx.hClusterAABBs);
		m_StorageSet->SetBuffer(0, aabbBuffer).Flush();
		m_AABBBufferBound = true;
	}

	auto hAABBs = ctx.hClusterAABBs;
	auto hLightList = graph.ImportBuffer(&SceneFrameData::Get()->GetLightIndexListBuffer(), "LightList",
										 Graphics::RG::ResourceState::UnorderedAccess);
	auto hClusterLightInfo = graph.ImportBuffer(&SceneFrameData::Get()->GetClusterLightInfoBuffer(), "ClusterLightInfo",
												Graphics::RG::ResourceState::UnorderedAccess);
	auto hGlobalCounter = graph.ImportBuffer(m_GlobalIndexCounter.get(), "GlobalIndexCounter",
											 Graphics::RG::ResourceState::UnorderedAccess);

	graph.AddPass(
		"LightCull",
		[&hAABBs, &hLightList, &hClusterLightInfo, &hGlobalCounter](Graphics::RG::RGPassBuilder &builder) {
			builder.ReadBuffer(hAABBs, Graphics::RG::ResourceState::ShaderRead);
			hLightList = builder.WriteBuffer(hLightList);
			hClusterLightInfo = builder.WriteBuffer(hClusterLightInfo);
			hGlobalCounter = builder.WriteBuffer(hGlobalCounter);
		},
		[this, frameData, frameSlot](GFX::GfxCommandList &cmd, Graphics::RG::RGRegistry &) {
			cmd.FillBuffer(*m_GlobalIndexCounter, 0u);
			cmd.BindPipeline(*m_Pipeline);
			cmd.BindDescriptorSet(0, frameData->GetDescriptorSet(frameSlot));
			cmd.BindDescriptorSet(1, *m_StorageSet);
			cmd.Dispatch(4, 3, 6);
		});

	if (ctx.hClusterAABBs.IsValid()) {
		ctx.hLightList = hLightList;
		ctx.hClusterLightInfo = hClusterLightInfo;
	}
}

} // namespace Aquila::Rendering
