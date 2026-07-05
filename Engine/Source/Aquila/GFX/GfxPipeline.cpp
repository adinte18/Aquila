#include "Aquila/GFX/GfxPipeline.h"
#include "Aquila/GFX/GfxCommandList.h"

namespace Aquila::GFX {

GfxPipeline::GfxPipeline(Unique<RHI::IRHIPipeline> pipeline) : m_pipeline(std::move(pipeline)) {}

void GfxPipeline::bind(GfxCommandList &cmd) {
	m_pipeline->bind(cmd.get_rhi());
}

} // namespace Aquila::GFX
