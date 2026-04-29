#pragma once
#include "Aquila/Foundation/Defines.h"

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

	virtual void OnInit(GFX::GfxContext &ctx) = 0;
	virtual void AddPasses(Graphics::RG::RenderGraph &graph, FrameContext &ctx) = 0;
	virtual void OnShutdown() {}

  protected:
	IRenderingSystem() = default;
};

} // namespace Aquila::Rendering
