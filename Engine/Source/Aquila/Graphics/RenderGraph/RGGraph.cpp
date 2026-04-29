#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Graphics/RenderGraph/RGCompiler.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/GFX/GfxRenderpass.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/GFX/GfxBuffer.h"

namespace Aquila::Graphics::RG {

void RenderGraph::Compile(GFX::GfxContext &ctx) {
	AQUILA_ASSERT(!m_Passes.empty(), "RenderGraph::Compile called with no passes registered");
	m_Compiled.Reset();
	m_Compiled = RGCompiler::Compile(m_Passes, m_Registry, ctx);
}

void RenderGraph::Execute(GFX::GfxCommandList &cmd) {
	AQUILA_ASSERT(m_Compiled.valid, "RenderGraph::Execute called before Compile()");

	uint32 schedPos = 0;

	for (const uint32 pi : m_Compiled.passOrder) {
		const RGPassData &pass = m_Passes[pi];
		AQUILA_ASSERT(pass.RenderPassExecute, "A pass has no execute function");

		cmd.PushDebugGroup(pass.name.c_str());

		// Pre-pass texture barriers
		const uint32 texBarBegin = m_Compiled.passTexBarStart[schedPos];
		const uint32 texBarEnd = m_Compiled.passTexBarStart[schedPos + 1];
		for (uint32 bi = texBarBegin; bi < texBarEnd; ++bi) {
			const RGTexBarrier &bar = m_Compiled.texBarriers[bi];
			GFX::GfxTexture &tex = m_Registry.GetTexture(bar.handle);
			cmd.TransitionTexture(tex, bar.oldState, bar.newState);
		}

		//  Pre-pass buffer barriers
		const uint32 bufBarBegin = m_Compiled.passBufBarStart[schedPos];
		const uint32 bufBarEnd = m_Compiled.passBufBarStart[schedPos + 1];
		for (uint32 bi = bufBarBegin; bi < bufBarEnd; ++bi) {
			const RGBufBarrier &bar = m_Compiled.bufBarriers[bi];
			GFX::GfxBuffer &buf = m_Registry.GetBuffer(bar.handle);
			cmd.TransitionBuffer(buf, bar.oldState, bar.newState);
		}

		// Begin renderpass (graphics passes only)
		GFX::GfxRenderPass *renderPass = m_Compiled.passRenderPasses[schedPos].get();
		if (renderPass != nullptr) {
			renderPass->Begin(cmd);
		}

		// User execute callback
		pass.RenderPassExecute(cmd, m_Registry);

		// End renderpass
		if (renderPass != nullptr) {
			renderPass->End(cmd);
		}

		cmd.PopDebugGroup();
		++schedPos;
	}
}

void RenderGraph::Reset() {
	m_Compiled.Reset();
	m_Passes.clear();
	m_Registry.Reset();
}

} // namespace Aquila::Graphics::RG
