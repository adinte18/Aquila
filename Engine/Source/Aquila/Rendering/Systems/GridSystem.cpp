#include "Aquila/Rendering/Systems/GridSystem.h"

namespace Aquila::Rendering {

void GridSystem::on_init(GFX::GfxContext &ctx) {
	RenderingSystemBase::on_init(ctx);
}

void GridSystem::add_passes(Graphics::RG::RenderGraph &graph, FrameContext &ctx) {
	auto *frame_data = ctx.frame_data;
	const Uint32 frame_slot = ctx.frame_slot;
}

} // namespace Aquila::Rendering
