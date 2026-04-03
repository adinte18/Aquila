#pragma once
#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/RHI/Backend/IRHICommandList.h"
#include "Aquila/RHI/Backend/IRHISwapchain.h"

namespace Aquila::RHI {

// IRHIRenderPass
//
// Sole responsibilities:
//   - Begin dynamic rendering (sets up color/depth attachments, issues pre-barriers when
//     externalBarriers = false, sets full-extent viewport+scissor).
//   - End dynamic rendering (issues post-barriers when externalBarriers = false).
//   - Report attachment format and sample count for pipeline key derivation.
//
// All draw commands, descriptor binding, pipeline binding, push constants, and
// viewport/scissor overrides live on IRHICommandList.

class IRHIRenderPass {
  public:
	virtual ~IRHIRenderPass() = default;
	AQUILA_NONCOPYABLE(IRHIRenderPass);
	AQUILA_NONMOVEABLE(IRHIRenderPass);

	// swapchain/imageIndex are only required when useSwapchain=true in the desc.
	virtual void Begin(IRHICommandList &cmd, IRHISwapchain *swapchain = nullptr, uint32 imageIndex = 0) = 0;
	virtual void End(IRHICommandList &cmd) = 0;

	[[nodiscard]] virtual uint32 GetWidth() const = 0;
	[[nodiscard]] virtual uint32 GetHeight() const = 0;
	[[nodiscard]] virtual RHI::TextureFormat GetColorFormat() const = 0;
	[[nodiscard]] virtual RHI::SampleCount GetSampleCount() const = 0;

  protected:
	IRHIRenderPass() = default;
};

} // namespace Aquila::RHI
