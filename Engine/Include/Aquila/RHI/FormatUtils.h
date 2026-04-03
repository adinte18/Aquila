#pragma once
#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::RHI {

AQUILA_FORCE_INLINE bool IsDepthFormat(TextureFormat format) {
	switch (format) {
	case TextureFormat::Depth16:
	case TextureFormat::Depth32:
	case TextureFormat::Depth24Stencil8:
	case TextureFormat::Depth32Stencil8:
		return true;
	default:
		return false;
	}
}

AQUILA_FORCE_INLINE bool IsStencilFormat(TextureFormat format) {
	return format == TextureFormat::Depth24Stencil8 || format == TextureFormat::Depth32Stencil8;
}

AQUILA_FORCE_INLINE bool IsColorFormat(TextureFormat format) {
	return !IsDepthFormat(format);
}

AQUILA_FORCE_INLINE uint32 GetFormatBytesPerPixel(TextureFormat format) {
	switch (format) {
	case TextureFormat::R8:
		return 1;
	case TextureFormat::RG8:
		return 2;
	case TextureFormat::RGB8:
		return 3;
	case TextureFormat::RGBA8:
	case TextureFormat::RGBA8_SRGB:
	case TextureFormat::BGRA8:
	case TextureFormat::BGRA8_SRGB:
	case TextureFormat::R32F:
	case TextureFormat::R32UI:
	case TextureFormat::Depth32:
	case TextureFormat::Depth24Stencil8:
		return 4;
	case TextureFormat::R16F:
	case TextureFormat::RG16F:
		return 4;
	case TextureFormat::RG32F:
		return 8;
	case TextureFormat::RGB16F:
		return 6;
	case TextureFormat::RGB32F:
		return 12;
	case TextureFormat::RGBA16F:
		return 8;
	case TextureFormat::RGBA32F:
		return 16;
	case TextureFormat::Depth32Stencil8:
		return 5;
	default:
		AQUILA_ASSERT(false, "Unknown TextureFormat for byte size");
		return 0;
	}
}

} // namespace Aquila::RHI
