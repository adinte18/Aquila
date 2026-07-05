#include "Aquila/Rendering/Renderers/Renderer2D.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/GFX/GfxSwapchain.h"

namespace Aquila::Rendering {

void Renderer2D::on_init(GFX::GfxContext &ctx) {
	m_ctx = &ctx;
}

void Renderer2D::set_swapchain_target(GFX::GfxSwapchain &swapchain, Uint32 image_index) {
	m_swapchain = &swapchain;
	m_swapchain_image_index = image_index;
}

void Renderer2D::add_passes(Graphics::RG::RenderGraph &graph, FrameContext &ctx) {
	for (auto &sys : m_systems) {
		sys->add_passes(graph, ctx);
	}
}

void Renderer2D::blit_to_swapchain(Graphics::RG::RenderGraph &graph, FrameContext &ctx) {
	ctx.swapchain = m_swapchain;
	ctx.swapchain_image_index = m_swapchain_image_index;
	for (auto &sys : m_systems) {
		sys->blit_to_swapchain(graph, ctx);
	}
}

void Renderer2D::on_resize(Uint32 width, Uint32 height) {
	for (auto &sys : m_systems) {
		sys->on_resize(width, height);
	}
}

void Renderer2D::on_shutdown() {
	for (auto &sys : m_systems) {
		sys->on_shutdown();
	}
}

} // namespace Aquila::Rendering
