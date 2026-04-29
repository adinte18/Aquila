#include "Aquila/GFX/GfxSwapchain.h"

namespace Aquila::GFX {

GfxSwapchain::GfxSwapchain(Unique<RHI::IRHISwapchain> swapchain) : m_Swapchain(std::move(swapchain)) {}

bool GfxSwapchain::AcquireNextImage(uint32 &outImageIndex) {
	return m_Swapchain->AcquireNextImage(outImageIndex);
}

bool GfxSwapchain::Present(uint32 imageIndex) {
	return m_Swapchain->Present(imageIndex);
}
uint32 GfxSwapchain::GetWidth() const {
	return m_Swapchain->GetWidth();
}
uint32 GfxSwapchain::GetHeight() const {
	return m_Swapchain->GetHeight();
}
RHI::TextureFormat GfxSwapchain::GetFormat() const {
	return m_Swapchain->GetFormat();
}
uint32 GfxSwapchain::GetImageCount() const {
	return m_Swapchain->GetImageCount();
}
bool GfxSwapchain::NeedsResize() const {
	return m_Swapchain->NeedsResize();
}
void GfxSwapchain::Resize(uint32 width, uint32 height) {
	m_Swapchain->Resize(width, height);
}

} // namespace Aquila::GFX
