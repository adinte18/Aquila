#include "Aquila/GFX/GfxSwapchain.h"

namespace Aquila::GFX {

GfxSwapchain::GfxSwapchain(Unique<RHI::IRHISwapchain> swapchain) : m_swapchain(std::move(swapchain)) {}

bool GfxSwapchain::acquire_next_image(Uint32 &out_image_index, bool drive_device_frame) {
	return m_swapchain->acquire_next_image(out_image_index, drive_device_frame);
}

Uint32 GfxSwapchain::get_width() const {
	return m_swapchain->get_width();
}
Uint32 GfxSwapchain::get_height() const {
	return m_swapchain->get_height();
}
RHI::TextureFormat GfxSwapchain::get_format() const {
	return m_swapchain->get_format();
}
Uint32 GfxSwapchain::get_image_count() const {
	return m_swapchain->get_image_count();
}
bool GfxSwapchain::needs_resize() const {
	return m_swapchain->needs_resize();
}
void GfxSwapchain::resize(Uint32 width, Uint32 height) {
	m_swapchain->resize(width, height);
}
Uint32 GfxSwapchain::get_current_frame_slot() const {
	return m_swapchain->get_current_frame_slot();
}

} // namespace Aquila::GFX
