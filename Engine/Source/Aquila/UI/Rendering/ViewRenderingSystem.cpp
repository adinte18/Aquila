#include "Aquila/UI/Rendering/ViewRenderingSystem.h"
#include "Aquila/UI/Core/CanvasManager.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Graphics/RenderGraph/RGPassBuilder.h"
#include "Aquila/Graphics/RenderGraph/RGTypes.h"
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/GFX/GfxSwapchain.h"
#include "Aquila/Foundation/Math/Math.h"
#include "Aquila/Foundation/Profiler.h"

namespace Aquila::UI::Rendering {

void ViewRenderingSystem::OnInit(GFX::GfxContext &ctx) {
	m_Ctx = &ctx;
	m_R2D = CreateUnique<Graphics::QuadBatcher>(ctx);
}

void ViewRenderingSystem::RebuildOverlayResources(uint32 w, uint32 h) {
	m_MSAAColor = m_Ctx->CreateTexture({
		.width = w,
		.height = h,
		.format = RHI::TextureFormat::BGRA8,
		.usage = RHI::TextureUsage::ColorAttachment,
		.samples = RHI::SampleCount::x4,
		.debugName = "UIMSAAColor",
	});

	m_OverlayPass = m_Ctx->CreateRenderPass({
		.colorAttachments = { {
			.texture = &m_MSAAColor->GetRHI(),
			.loadOp = RHI::AttachmentLoadOp::Clear,
			.storeOp = RHI::AttachmentStoreOp::DontCare,
		} },
		.useSwapchainAsResolve = true,
		.debugName = "UIOverlay",
	});
}

void ViewRenderingSystem::AddPasses(Graphics::RG::RenderGraph &graph, Aquila::Rendering::FrameContext &ctx) {
	const uint32 w = ctx.width;
	const uint32 h = ctx.height;
	auto *r2d = m_R2D.get();

	// WorldSpace + ScreenCamera composite into scene color.
	graph.AddPass(
		"GameUI",
		[&ctx](Graphics::RG::RGPassBuilder &builder) {
			ctx.hSceneColor = builder.SetColorAttachment(0, ctx.hSceneColor, Graphics::RG::AttachmentLoadOp::Load,
														 Graphics::RG::AttachmentStoreOp::Store);
		},
		[r2d, w, h](GFX::GfxCommandList &cmd, Graphics::RG::RGRegistry &) {
			const mat4 ortho = glm::ortho(0.f, static_cast<float>(w), static_cast<float>(h), 0.f, -1.f, 1.f);
			r2d->Begin(cmd, RHI::TextureFormat::RGBA16F, RHI::SampleCount::x1, ortho);
			Core::CanvasManager::Get()->RenderLayers(*r2d, cmd, Core::UILayer::WorldSpace, Core::UILayer::ScreenCamera);
			r2d->End();
		});
}

void ViewRenderingSystem::BlitToSwapchain(Graphics::RG::RenderGraph &graph, Aquila::Rendering::FrameContext &ctx) {
	if (!ctx.swapchain) {
		return;
	}

	const uint32 w = ctx.width;
	const uint32 h = ctx.height;

	if (!m_OverlayPass || w != m_Width || h != m_Height) {
		m_Width = w;
		m_Height = h;
		RebuildOverlayResources(w, h);
	}

	auto *r2d = m_R2D.get();
	auto *swapchain = ctx.swapchain;
	auto imageIndex = ctx.swapchainImageIndex;
	auto *overlayPass = m_OverlayPass.get();

	// ScreenOverlay + Editor onto swapchain with MSAA resolve, after SwapchainBlit
	graph.AddPass(
		"UIOverlay",
		[&ctx](Graphics::RG::RGPassBuilder &builder) {
			builder.MarkAsSideEffect();
			builder.ReadTexture(ctx.hSceneColor, Graphics::RG::ResourceState::ShaderRead);
		},
		[r2d, swapchain, imageIndex, overlayPass, w, h](GFX::GfxCommandList &cmd, Graphics::RG::RGRegistry &) {
			const mat4 ortho = glm::ortho(0.f, static_cast<float>(w), static_cast<float>(h), 0.f, -1.f, 1.f);
			const bool dirty =
				Core::CanvasManager::Get()->IsAnyLayerDirty(Core::UILayer::ScreenOverlay, Core::UILayer::Editor);

			overlayPass->Begin(cmd, swapchain, imageIndex);
			if (dirty) {
				PROFILE_SCOPE("UIOverlay::DirtyRebuild");
				r2d->BeginCapture();
				r2d->Begin(cmd, RHI::TextureFormat::BGRA8, RHI::SampleCount::x4, ortho);
				Core::CanvasManager::Get()->RenderLayers(*r2d, cmd, Core::UILayer::ScreenOverlay,
														 Core::UILayer::Editor);
				r2d->End();
				Core::CanvasManager::Get()->ClearLayerDirtyFlags(Core::UILayer::ScreenOverlay, Core::UILayer::Editor);
			} else {
				PROFILE_SCOPE("UIOverlay::Replay");
				r2d->ExecuteReplay(cmd);
			}
			overlayPass->End(cmd);
		});
}

void ViewRenderingSystem::OnResize(uint32 width, uint32 height) {
	Core::CanvasManager::Get()->Resize(width, height);
}

} // namespace Aquila::UI::Rendering
