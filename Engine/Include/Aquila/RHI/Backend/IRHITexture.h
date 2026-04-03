#ifndef AQUILA_IRHI_TEXTURE_H
#define AQUILA_IRHI_TEXTURE_H

#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::RHI {

class IRHITexture {
  public:
	virtual ~IRHITexture() = default;

	IRHITexture(const IRHITexture &) = delete;
	IRHITexture &operator=(const IRHITexture &) = delete;

	[[nodiscard]] virtual uint32 GetWidth() const = 0;
	[[nodiscard]] virtual uint32 GetHeight() const = 0;
	[[nodiscard]] virtual uint32 GetMipLevels() const = 0;
	[[nodiscard]] virtual uint32 GetArrayLayers() const = 0;
	[[nodiscard]] virtual TextureFormat GetFormat() const = 0;
	[[nodiscard]] virtual SampleCount GetSampleCount() const = 0;
	[[nodiscard]] virtual const TextureDesc &GetDesc() const = 0;

  protected:
	IRHITexture() = default;
};

} // namespace Aquila::RHI
#endif
