#include "Aquila/Rendering/Renderers/Scene2DRenderer.h"
#include "Aquila/Rendering/FrameContext.h"

namespace Aquila::Rendering {

void Scene2DRenderer::OnInit(GFX::GfxContext &ctx) {
	m_Ctx = &ctx;
	m_Renderer2D = CreateUnique<Graphics::Renderer2D>(ctx);
}

void Scene2DRenderer::OnShutdown() {
	for (auto &sys : m_Systems)
		sys->OnShutdown();
}

void Scene2DRenderer::AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) {
	for (auto &sys : m_Systems)
		sys->AddPasses(graph, ctx, *m_Renderer2D);
}

} // namespace Aquila::Rendering
