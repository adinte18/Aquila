#include "Aquila/Rendering/Systems/GeometrySystem.h"
#include "Aquila/Foundation/Color.h"
#include "Aquila/Rendering/FrameContext.h"
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
using Aquila::SharedConstants::kShadersDir;

struct MeshPushConstants {
	mat4 mvp;
	vec4 color = vec4(1.F); // per-draw tint; white = no tint
};

void GeometrySystem::OnInit(GFX::GfxContext &ctx) {
	RenderingSystemBase::OnInit(ctx);

	std::vector<RHI::VulkanCompiledStage> stages;
	std::string err;
	if (!RHI::VulkanShaderCompiler::CompileFile(kShadersDir + "Basic.slang", stages, err)) {
		AQUILA_LOG_ERROR("GeometrySystem: shader compile failed: {}", err);
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

	pipelineDescriptor.colorFormats = { RHI::TextureFormat::RGBA16F };
	pipelineDescriptor.depthFormat = RHI::TextureFormat::Depth32;
	pipelineDescriptor.topology = RHI::PrimitiveTopology::TriangleList;
	pipelineDescriptor.raster.cullMode = RHI::CullMode::Back;
	pipelineDescriptor.raster.frontFace = RHI::FrontFace::Clockwise;
	pipelineDescriptor.depthStencil.depthTest = true;
	pipelineDescriptor.depthStencil.depthWrite = false; // depth prepass already wrote it
	pipelineDescriptor.pushConstants = { { RHI::ShaderStageFlags::Vertex | RHI::ShaderStageFlags::Fragment, 0,
										   sizeof(MeshPushConstants) } };

	m_Pipeline = ctx.CreateGraphicsPipeline(pipelineDescriptor);
}

void GeometrySystem::AddPasses(RG::RenderGraph &graph, FrameContext &ctx) {
	if (!m_Pipeline) {
		return;
	}

	// Collect draw calls synchronously during setup so the execute lambda is allocation-free.
	auto &registry = ctx.scene->GetRegistry();
	auto view = registry.view<TransformComponent, MeshComponent>();

	struct DrawCall {
		Ref<GFX::GfxMesh> gpuMesh;
		mat4 mvp;
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
			.mvp = ctx.viewProjection * transform.GetWorldMatrix(),
		});
	}

	graph.AddPass(
		"Geometry",
		[&ctx](RG::RGPassBuilder &builder) {
			// Clear colour on first write; load depth from the prepass (no clear).
			ctx.hSceneColor =
				builder.SetColorAttachment(0, ctx.hSceneColor, RG::AttachmentLoadOp::Clear,
										   RG::AttachmentStoreOp::Store, { Foundation::Color::RGBA::DarkGray });

			builder.SetDepthAttachment(ctx.hDepth, RG::AttachmentLoadOp::Clear, RG::AttachmentStoreOp::Store,
									   RG::AttachmentLoadOp::DontCare, RG::AttachmentStoreOp::DontCare,
									   /*readOnly=*/false, RG::ClearDepth{ .depth = 1.F });
		},
		[this, drawCalls = std::move(drawCalls)](GFX::GfxCommandList &cmd, RG::RGRegistry &) {
			if (drawCalls.empty()) {
				return;
			}
			cmd.BindPipeline(*m_Pipeline);
			for (const auto &drawCall : drawCalls) {
				MeshPushConstants pushConstants{ .mvp = drawCall.mvp };
				cmd.PushConstants(pushConstants, RHI::ShaderStageFlags::Vertex | RHI::ShaderStageFlags::Fragment);
				cmd.BindVertexBuffer(drawCall.gpuMesh->GetVertexBuffer());
				cmd.BindIndexBuffer(drawCall.gpuMesh->GetIndexBuffer());
				cmd.DrawIndexed(drawCall.gpuMesh->GetIndexCount());
			}
		});
}

} // namespace Aquila::Rendering
