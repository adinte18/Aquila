#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHISwapchain.h"

namespace Aquila::GFX {

class GfxContext;

class GfxSwapchain {
  public:
	~GfxSwapchain() = default;
	AQUILA_NONCOPYABLE(GfxSwapchain);

	bool AcquireNextImage(uint32 &outImageIndex);

	[[nodiscard]] uint32 GetWidth() const;
	[[nodiscard]] uint32 GetHeight() const;
	[[nodiscard]] RHI::TextureFormat GetFormat() const;
	[[nodiscard]] uint32 GetImageCount() const;
	[[nodiscard]] bool NeedsResize() const;
	void Resize(uint32 width, uint32 height);
	[[nodiscard]] uint32 GetCurrentFrameSlot() const;
	[[nodiscard]] RHI::IRHISwapchain &GetRHI() { return *m_Swapchain; }

  private:
	friend class GfxContext;
	explicit GfxSwapchain(Unique<RHI::IRHISwapchain> swapchain);
	Unique<RHI::IRHISwapchain> m_Swapchain;
};

} // namespace Aquila::GFX
