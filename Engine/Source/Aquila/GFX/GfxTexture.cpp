#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::GFX {

GfxTexture::GfxTexture(Unique<RHI::IRHITexture> texture) : m_Texture(std::move(texture)) {}

uint32 GfxTexture::GetWidth() const {
	return m_Texture->GetWidth();
}
uint32 GfxTexture::GetHeight() const {
	return m_Texture->GetHeight();
}
uint32 GfxTexture::GetMipLevels() const {
	return m_Texture->GetMipLevels();
}
uint32 GfxTexture::GetArrayLayers() const {
	return m_Texture->GetArrayLayers();
}
RHI::TextureFormat GfxTexture::GetFormat() const {
	return m_Texture->GetFormat();
}
const RHI::TextureDesc &GfxTexture::GetDesc() const {
	return m_Texture->GetDesc();
}

} // namespace Aquila::GFX
