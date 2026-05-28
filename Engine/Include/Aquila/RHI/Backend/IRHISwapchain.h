#ifndef AQUILA_IRHI_SWAPCHAIN_H
#define AQUILA_IRHI_SWAPCHAIN_H

#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::RHI {

class IRHISwapchain {
  public:
	virtual ~IRHISwapchain() = default;

	IRHISwapchain(const IRHISwapchain &) = delete;
	IRHISwapchain &operator=(const IRHISwapchain &) = delete;

	// Returns false when the swapchain is out-of-date (must call Resize before next frame).
	virtual bool AcquireNextImage(uint32 &outImageIndex) = 0;

	[[nodiscard]] virtual uint32 GetWidth() const = 0;
	[[nodiscard]] virtual uint32 GetHeight() const = 0;
	[[nodiscard]] virtual TextureFormat GetFormat() const = 0;
	[[nodiscard]] virtual uint32 GetImageCount() const = 0;

	// True after a suboptimal present; also call Resize to fix it.
	[[nodiscard]] virtual bool NeedsResize() const = 0;
	virtual void Resize(uint32 width, uint32 height) = 0;

	[[nodiscard]] virtual uint32 GetCurrentFrameSlot() const = 0;

  protected:
	IRHISwapchain() = default;
};

} // namespace Aquila::RHI
#endif
