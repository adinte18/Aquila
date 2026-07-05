#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHICommandList.h"
#include "Aquila/RHI/Backend/IRHIPipeline.h"

namespace Aquila::GFX {

class GfxContext;
class GfxCommandList;

class GfxPipeline {
  public:
	~GfxPipeline() = default;
	AQUILA_NONCOPYABLE(GfxPipeline);

	void bind(GfxCommandList &cmd);

	[[nodiscard]] RHI::IRHIPipeline &get_rhi() { return *m_pipeline; }

  private:
	friend class GfxContext;
	explicit GfxPipeline(Unique<RHI::IRHIPipeline> pipeline);
	Unique<RHI::IRHIPipeline> m_pipeline;
};

} // namespace Aquila::GFX
