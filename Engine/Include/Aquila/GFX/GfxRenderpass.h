#pragma once
#include "Aquila/RHI/Backend/IRHIRenderpass.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/GFX/GfxSwapchain.h"
#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::GFX {

// GfxRenderPass
//
// Thin wrapper over IRHIRenderPass.  Its only job is to begin and end
// dynamic rendering for a set of color/depth attachments.
//
// All draw commands, descriptor binding, push constants, pipeline binding,
// and viewport/scissor overrides must go through GfxCommandList.

class GfxRenderPass {
  public:
	explicit GfxRenderPass(Unique<RHI::IRHIRenderPass> impl) : m_Impl(std::move(impl)) {}
	~GfxRenderPass() = default;
	AQUILA_NONCOPYABLE(GfxRenderPass);
	AQUILA_NONMOVEABLE(GfxRenderPass);

	void Begin(GfxCommandList &cmd, GfxSwapchain *swapchain = nullptr, uint32 imageIndex = 0) {
		m_Impl->Begin(cmd.GetRHI(), (swapchain != nullptr) ? &swapchain->GetRHI() : nullptr, imageIndex);
	}
	void End(GfxCommandList &cmd) { m_Impl->End(cmd.GetRHI()); }

	[[nodiscard]] uint32 GetWidth() const { return m_Impl->GetWidth(); }
	[[nodiscard]] uint32 GetHeight() const { return m_Impl->GetHeight(); }
	[[nodiscard]] RHI::TextureFormat GetColorFormat() const { return m_Impl->GetColorFormat(); }
	[[nodiscard]] RHI::SampleCount GetSampleCount() const { return m_Impl->GetSampleCount(); }

	[[nodiscard]] RHI::IRHIRenderPass &GetRHI() { return *m_Impl; }

  private:
	Unique<RHI::IRHIRenderPass> m_Impl;
};

} // namespace Aquila::GFX
