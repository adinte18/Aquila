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

class IRenderer {
  public:
	virtual ~IRenderer() = default;

	AQUILA_NONCOPYABLE(IRenderer);
	AQUILA_NONMOVEABLE(IRenderer);

	virtual void OnInit(GFX::GfxContext &ctx) = 0;
	virtual void AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) = 0;
	virtual void BlitToSwapchain(Graphics::RG::RenderGraph & /*graph*/, FrameContext & /*ctx*/) {}
	virtual void OnResize(uint32 width, uint32 height) {}
	virtual void OnShutdown() {}

  protected:
	IRenderer() = default;
};

} // namespace Aquila::Rendering
