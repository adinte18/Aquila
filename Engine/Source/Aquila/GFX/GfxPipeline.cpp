#include "Aquila/GFX/GfxPipeline.h"
#include "Aquila/GFX/GfxCommandList.h"

namespace Aquila::GFX {

GfxPipeline::GfxPipeline(Unique<RHI::IRHIPipeline> pipeline) : m_Pipeline(std::move(pipeline)) {}

void GfxPipeline::Bind(GfxCommandList &cmd) {
	m_Pipeline->Bind(cmd.GetRHI());
}

} // namespace Aquila::GFX
