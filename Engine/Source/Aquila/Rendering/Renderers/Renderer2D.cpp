#include "Aquila/Rendering/Renderers/Renderer2D.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/GFX/GfxSwapchain.h"

namespace Aquila::Rendering {

void Renderer2D::OnInit(GFX::GfxContext &ctx) {
	m_Ctx = &ctx;
}

void Renderer2D::SetSwapchainTarget(GFX::GfxSwapchain &swapchain, uint32 imageIndex) {
	m_Swapchain = &swapchain;
	m_SwapchainImageIndex = imageIndex;
}

void Renderer2D::AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) {
	for (auto &sys : m_Systems) {
		sys->AddPasses(graph, ctx);
	}
}

void Renderer2D::BlitToSwapchain(Graphics::RG::RenderGraph &graph, FrameContext &ctx) {
	ctx.swapchain = m_Swapchain;
	ctx.swapchainImageIndex = m_SwapchainImageIndex;
	for (auto &sys : m_Systems) {
		sys->BlitToSwapchain(graph, ctx);
	}
}

void Renderer2D::OnResize(uint32 width, uint32 height) {
	for (auto &sys : m_Systems) {
		sys->OnResize(width, height);
	}
}

void Renderer2D::OnShutdown() {
	for (auto &sys : m_Systems) {
		sys->OnShutdown();
	}
}

} // namespace Aquila::Rendering
