#include "Aquila/Rendering/Systems/DepthPrepassSystem.h"
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/Rendering/SceneFrameData.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Graphics/RenderGraph/RGPassBuilder.h"
#include "Aquila/RHI/Vulkan/VulkanShaderCompiler.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/SharedConstants.h"

namespace Aquila::Rendering {

using namespace SceneManagement::Components;
using namespace Graphics;
using Aquila::SharedConstants::SHADERS_DIR;

struct DepthPushConstants {
	mat4 model;
};

void DepthPrepassSystem::OnInit(GFX::GfxContext &ctx) {
	RenderingSystemBase::OnInit(ctx);

	std::vector<RHI::VulkanCompiledStage> stages;
	std::string err;
	if (!RHI::VulkanShaderCompiler::CompileFile(SHADERS_DIR + "DepthOnly.slang", stages, err)) {
		AQUILA_LOG_ERROR("DepthPrepassSystem: shader compile failed: {}", err);
		return;
	}

	RHI::GraphicsPipelineDesc pipelineDescriptor{};
	for (auto &stage : stages) {
		RHI::ShaderStageDesc shaderDescriptor{ .spirv = stage.spirv, .entryPoint = stage.entryPointName };
		if (stage.stage == VK_SHADER_STAGE_VERTEX_BIT) {
			shaderDescriptor.stage = RHI::ShaderStageFlags::Vertex;
			pipelineDescriptor.vertexShader = shaderDescriptor;
		} else {
			shaderDescriptor.stage = RHI::ShaderStageFlags::Fragment;
			pipelineDescriptor.fragmentShader = shaderDescriptor;
		}
	}

	pipelineDescriptor.colorFormats = {}; // depth-only
	pipelineDescriptor.depthFormat = RHI::TextureFormat::Depth32;
	pipelineDescriptor.topology = RHI::PrimitiveTopology::TriangleList;
	pipelineDescriptor.raster.cullMode = RHI::CullMode::Back;
	pipelineDescriptor.raster.frontFace = RHI::FrontFace::Clockwise;
	pipelineDescriptor.depthStencil.depthTest = true;
	pipelineDescriptor.depthStencil.depthWrite = true;
	pipelineDescriptor.setLayouts = { &SceneFrameData::Get()->GetLayout().GetRHI() };
	pipelineDescriptor.pushConstants = { { RHI::ShaderStageFlags::Vertex, 0, sizeof(DepthPushConstants) } };
	m_Pipeline = ctx.CreateGraphicsPipeline(pipelineDescriptor);
}

void DepthPrepassSystem::AddPasses(RG::RenderGraph &graph, FrameContext &ctx) {
	if (!m_Pipeline) {
		return;
	}

	auto &registry = ctx.scene->GetRegistry();
	auto view = registry.view<TransformComponent, MeshComponent>();

	struct DrawCall {
		Ref<GFX::GfxMesh> gpuMesh;
		mat4 model;
	};

	std::vector<DrawCall> drawCalls;
	drawCalls.reserve(view.size_hint());

	for (auto entity : view) {
		auto &transform = view.get<TransformComponent>(entity);
		auto &mesh = view.get<MeshComponent>(entity);
		if (!mesh.IsValid()) {
			continue;
		}

		drawCalls.push_back({
			.gpuMesh = GetOrUploadMesh(mesh.data),
			.model = transform.GetWorldMatrix(),
		});
	}

	auto *frameData = ctx.frameData;
	const uint32 frameSlot = ctx.frameSlot;

	graph.AddPass(
		"DepthPrepass",
		[&ctx](RG::RGPassBuilder &builder) {
			ctx.hDepth =
				builder.SetDepthAttachment(ctx.hDepth, RG::AttachmentLoadOp::Clear, RG::AttachmentStoreOp::Store,
										   RG::AttachmentLoadOp::DontCare, RG::AttachmentStoreOp::DontCare,
										   /*readOnly=*/false, RG::ClearDepth{ .depth = 1.F });
		},
		[this, drawCalls = std::move(drawCalls), frameData, frameSlot](GFX::GfxCommandList &cmd, RG::RGRegistry &) {
			if (drawCalls.empty()) {
				return;
			}
			cmd.BindPipeline(*m_Pipeline);
			cmd.BindDescriptorSet(0, frameData->GetDescriptorSet(frameSlot));
			for (const auto &drawCall : drawCalls) {
				DepthPushConstants pushConstants{ .model = drawCall.model };
				cmd.PushConstants(pushConstants, RHI::ShaderStageFlags::Vertex);
				cmd.BindVertexBuffer(drawCall.gpuMesh->GetVertexBuffer());
				cmd.BindIndexBuffer(drawCall.gpuMesh->GetIndexBuffer());
				cmd.DrawIndexed(drawCall.gpuMesh->GetIndexCount());
			}
		});
}

} // namespace Aquila::Rendering
