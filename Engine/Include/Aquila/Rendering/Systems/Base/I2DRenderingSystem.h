#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::GFX {
class GfxContext;
class GfxCommandList;
}

namespace Aquila::Graphics {
class QuadBatcher;
}

namespace Aquila::Graphics::RG {
class RenderGraph;
}

namespace Aquila::Rendering {
struct FrameContext;

class I2DRenderingSystem {
  public:
	virtual ~I2DRenderingSystem() = default;

	AQUILA_NONCOPYABLE(I2DRenderingSystem);
	AQUILA_NONMOVEABLE(I2DRenderingSystem);

	virtual void OnInit(GFX::GfxContext &ctx) = 0;
	virtual void AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx, Graphics::QuadBatcher &r2d) {}
	virtual void Render(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd) {}
	virtual void OnResize(uint32 width, uint32 height) {}
	virtual void OnShutdown() {}

  protected:
	I2DRenderingSystem() = default;
};

} // namespace Aquila::Rendering
