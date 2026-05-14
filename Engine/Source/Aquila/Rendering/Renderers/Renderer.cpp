#include "Aquila/Rendering/Renderers/Renderer.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxSwapchain.h"
#include "Aquila/RHI/Vulkan/VulkanShaderCompiler.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/SharedConstants.h"

using Aquila::SharedConstants::SHADERS_DIR;

namespace Aquila::Rendering {

void Renderer::OnInit(GFX::GfxContext &ctx) {
	m_Ctx = &ctx;

	// define blit layout
	m_BlitLayout = ctx.CreateDescriptorSetLayout({
		.bindings = { {
			.binding = 0,
			.type = RHI::DescriptorType::CombinedImageSampler,
			.stages = RHI::ShaderStageFlags::Fragment,
			.count = 1,
		} },
	});

	// compile blit shader
	std::vector<RHI::VulkanCompiledStage> stages;
	std::string err;
	if (!RHI::VulkanShaderCompiler::CompileFile(SHADERS_DIR + "Blit.slang", stages, err)) {
		AQUILA_LOG_ERROR("Renderer: blit shader compile failed: {}", err);
		return;
	}

	// describe blit pipeline
	RHI::GraphicsPipelineDesc desc{};
	for (auto &stage : stages) {
		RHI::ShaderStageDesc shaderDesc{ .spirv = stage.spirv, .entryPoint = stage.entryPointName };
		if (stage.stage == VK_SHADER_STAGE_VERTEX_BIT) {
			shaderDesc.stage = RHI::ShaderStageFlags::Vertex;
			desc.vertexShader = shaderDesc;
		} else {
			shaderDesc.stage = RHI::ShaderStageFlags::Fragment;
			desc.fragmentShader = shaderDesc;
		}
	}
	desc.colorFormats = { RHI::TextureFormat::BGRA8 };
	desc.depthFormat = RHI::TextureFormat::None;
	desc.noVertexInput = true;
	desc.raster.cullMode = RHI::CullMode::None;
	desc.setLayouts = { &m_BlitLayout->GetRHI() };
	m_BlitPipeline = ctx.CreateGraphicsPipeline(desc);
	for (auto &set : m_BlitSets) {
		set = ctx.AllocateDescriptorSet(*m_BlitLayout);
	}

	// create swapchain pass
	m_SwapchainPass = ctx.CreateRenderPass({
		.colorAttachments = { {
			.loadOp = RHI::AttachmentLoadOp::DontCare,
			.storeOp = RHI::AttachmentStoreOp::Store,
		} },
		.useSwapchain = true,
		.debugName = "SwapchainBlit",
	});
}

void Renderer::OnShutdown() {
	for (auto &sys : m_Systems) {
		sys->OnShutdown();
	}
}

void Renderer::SetSwapchainTarget(GFX::GfxSwapchain &swapchain, uint32 imageIndex) {
	m_Swapchain = &swapchain;
	m_SwapchainImageIndex = imageIndex;
	m_FrameSlot = (m_FrameSlot + 1) % SharedConstants::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) {
	for (auto &sys : m_Systems) {
		sys->AddPasses(graph, ctx);
	}
}

void Renderer::AddFinalPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) {
	if (!m_BlitPipeline || (m_Swapchain == nullptr)) {
		return;
	}

	auto hSrc = ctx.hSceneColor;
	auto *swapchain = m_Swapchain;
	auto imageIndex = m_SwapchainImageIndex;
	auto *set = m_BlitSets[m_FrameSlot].get();
	auto *pipeline = m_BlitPipeline.get();
	auto *renderPass = m_SwapchainPass.get();

	graph.AddPass(
		"SwapchainBlit",
		[&](Graphics::RG::RGPassBuilder &builder) {
			builder.ReadTexture(hSrc, Graphics::RG::ResourceState::ShaderRead);
			builder.MarkAsSideEffect();
		},
		[hSrc, swapchain, imageIndex, set, pipeline, renderPass](GFX::GfxCommandList &cmd,
																 Graphics::RG::RGRegistry &reg) {
			auto &srcTex = reg.GetTexture(hSrc);
			renderPass->Begin(cmd, swapchain, imageIndex);
			cmd.BindPipeline(*pipeline);
			set->SetTexture(0, srcTex);
			set->Flush();
			cmd.BindDescriptorSet(0, *set);
			cmd.Draw(3);
			renderPass->End(cmd);
		});
}

} // namespace Aquila::Rendering
