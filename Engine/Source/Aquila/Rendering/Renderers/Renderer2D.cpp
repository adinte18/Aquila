#include "Aquila/Rendering/Renderers/Renderer2D.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Graphics/RenderGraph/RGPassBuilder.h"
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/GFX/GfxSwapchain.h"
#include "Aquila/Foundation/Math/Math.h"

namespace Aquila::Rendering {

void Renderer2D::OnInit(GFX::GfxContext &ctx) {
	m_Ctx = &ctx;
	m_R2D = CreateUnique<Graphics::QuadBatcher>(ctx);

	m_SwapchainPass = ctx.CreateRenderPass({
		.colorAttachments = { {
			.loadOp = RHI::AttachmentLoadOp::Load,
			.storeOp = RHI::AttachmentStoreOp::Store,
		} },
		.useSwapchain = true,
		.debugName = "UIOverlay",
	});
}

void Renderer2D::OnShutdown() {
	for (auto &sys : m_Systems) {
		sys->OnShutdown();
	}
}

void Renderer2D::SetSwapchainTarget(GFX::GfxSwapchain &swapchain, uint32 imageIndex) {
	m_Swapchain = &swapchain;
	m_SwapchainImageIndex = imageIndex;
}

void Renderer2D::AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) {
	for (auto &sys : m_Systems) {
		sys->AddPasses(graph, ctx, *m_R2D);
	}
}

void Renderer2D::AddFinalPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) {
	if (m_Swapchain == nullptr) {
		return;
	}

	auto *swapchain = m_Swapchain;
	auto imageIndex = m_SwapchainImageIndex;
	auto *renderPass = m_SwapchainPass.get();
	auto *r2d = m_R2D.get();
	auto *systems = &m_Systems;
	const uint32 w = ctx.width;
	const uint32 h = ctx.height;

	// m_UIDirty is set by the application before Render() via SetUIDirty().
	const bool uiDirty = m_UIDirty;

	graph.AddPass(
		"UIOverlay", [](Graphics::RG::RGPassBuilder &builder) { builder.MarkAsSideEffect(); },
		[swapchain, imageIndex, renderPass, r2d, systems, w, h, uiDirty](GFX::GfxCommandList &cmd,
																		  Graphics::RG::RGRegistry & /*reg*/) {
			const mat4 ortho = glm::ortho(0.f, static_cast<float>(w), static_cast<float>(h), 0.f, -1.f, 1.f);
			renderPass->Begin(cmd, swapchain, imageIndex);
			if (uiDirty) {
				r2d->BeginCapture();
				r2d->Begin(cmd, RHI::TextureFormat::BGRA8, RHI::SampleCount::x1, ortho);
				for (auto &sys : *systems) {
					sys->Render(*r2d, cmd);
				}
				r2d->End();
			} else {
				r2d->ExecuteReplay(cmd);
			}
			renderPass->End(cmd);
		});
}

} // namespace Aquila::Rendering
