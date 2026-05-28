#include "Aquila/UI/Rendering/ViewRenderingSystem.h"
#include "Aquila/UI/Core/ViewSystem.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Graphics/RenderGraph/RGPassBuilder.h"
#include "Aquila/Graphics/RenderGraph/RGTypes.h"
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/Foundation/Math/Math.h"

namespace Aquila::UI::Rendering {

void ViewRenderingSystem::OnInit(GFX::GfxContext & /*ctx*/) {}

void ViewRenderingSystem::AddPasses(Graphics::RG::RenderGraph &graph, Aquila::Rendering::FrameContext &ctx,
									Graphics::QuadBatcher &r2d) {
	const uint32 w = ctx.width;
	const uint32 h = ctx.height;

	// Composite WorldSpace + ScreenCamera canvas draw lists directly into hSceneColor.
	graph.AddPass(
		"GameUI",
		[&](Graphics::RG::RGPassBuilder &builder) {
			ctx.hSceneColor = builder.SetColorAttachment(0, ctx.hSceneColor, Graphics::RG::AttachmentLoadOp::Load,
														 Graphics::RG::AttachmentStoreOp::Store);
		},
		[&r2d, w, h](GFX::GfxCommandList &cmd, Graphics::RG::RGRegistry &) {
			const mat4 ortho = glm::ortho(0.f, static_cast<float>(w), static_cast<float>(h), 0.f, -1.f, 1.f);
			r2d.Begin(cmd, RHI::TextureFormat::RGBA16F, RHI::SampleCount::x1, ortho);
			Core::ViewSystem::Get()->RenderLayers(r2d, cmd, Core::UILayer::WorldSpace, Core::UILayer::ScreenCamera);
			r2d.End();
		});
}

void ViewRenderingSystem::Render(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd) {
	Core::ViewSystem::Get()->RenderLayers(r2d, cmd, Core::UILayer::ScreenOverlay, Core::UILayer::Editor);
}

void ViewRenderingSystem::OnResize(uint32 width, uint32 height) {
	m_Width = width;
	m_Height = height;
	Core::ViewSystem::Get()->Resize(width, height);
}

} // namespace Aquila::UI::Rendering
