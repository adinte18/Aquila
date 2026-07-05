#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::GFX {
class GfxContext;
}

namespace Aquila::Graphics::RG {
class RenderGraph;
}

namespace Aquila::Rendering {
struct FrameContext;

class IRenderingSystem {
  public:
	virtual ~IRenderingSystem() = default;

	AQUILA_NONCOPYABLE(IRenderingSystem);
	AQUILA_NONMOVEABLE(IRenderingSystem);

	virtual void on_init(GFX::GfxContext &ctx) = 0;
	virtual void add_passes(Graphics::RG::RenderGraph &graph, FrameContext &ctx) = 0;
	virtual void blit_to_swapchain(Graphics::RG::RenderGraph & /*graph*/, FrameContext & /*ctx*/) {}
	virtual void on_resize(Uint32 /*width*/, Uint32 /*height*/) {}
	virtual void on_shutdown() {}

  protected:
	IRenderingSystem() = default;
};

} // namespace Aquila::Rendering
