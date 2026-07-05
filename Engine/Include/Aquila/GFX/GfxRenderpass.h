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
	explicit GfxRenderPass(Unique<RHI::IRHIRenderPass> impl) : m_impl(std::move(impl)) {}
	~GfxRenderPass() = default;
	AQUILA_NONCOPYABLE(GfxRenderPass);
	AQUILA_NONMOVEABLE(GfxRenderPass);

	void begin(GfxCommandList &cmd, GfxSwapchain *swapchain = nullptr, Uint32 image_index = 0) {
		m_impl->begin(cmd.get_rhi(), (swapchain != nullptr) ? &swapchain->get_rhi() : nullptr, image_index);
	}
	void end(GfxCommandList &cmd) { m_impl->end(cmd.get_rhi()); }

	[[nodiscard]] Uint32 get_width() const { return m_impl->get_width(); }
	[[nodiscard]] Uint32 get_height() const { return m_impl->get_height(); }
	[[nodiscard]] RHI::TextureFormat get_color_format() const { return m_impl->get_color_format(); }
	[[nodiscard]] RHI::SampleCount get_sample_count() const { return m_impl->get_sample_count(); }

	[[nodiscard]] RHI::IRHIRenderPass &get_rhi() { return *m_impl; }

  private:
	Unique<RHI::IRHIRenderPass> m_impl;
};

} // namespace Aquila::GFX
