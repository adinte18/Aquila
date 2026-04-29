#ifndef AQUILA_TEXTURELOADER_H
#define AQUILA_TEXTURELOADER_H

#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::Graphics::Texture {

class TextureLoader {
  public:
	struct ImageData {
		Unique<f32[]> pixels;
		uint32 width = 0;
		uint32 height = 0;
		RHI::TextureFormat format = RHI::TextureFormat::RGBA8;
		uint32 mipLevels = 1;
	};

	struct RawImageData {
		Unique<uint8[]> pixels;
		uint32 width = 0;
		uint32 height = 0;
		uint32 channels = 0;

		[[nodiscard]] bool IsValid() const { return pixels != nullptr && width > 0 && height > 0; }
		[[nodiscard]] size_t SizeBytes() const { return static_cast<size_t>(width) * height * 4; }
	};

	struct RawHDRData {
		Unique<f32[]> pixels;
		uint32 width = 0;
		uint32 height = 0;
		uint32 channels = 0;

		[[nodiscard]] bool IsValid() const { return pixels != nullptr && width > 0 && height > 0; }
		[[nodiscard]] size_t SizeBytes() const { return static_cast<size_t>(width) * height * 4 * sizeof(f32); }
	};

	TextureLoader() = default;

	RawImageData LoadFromFile(const std::string &filepath);
	RawImageData LoadFromVFS(const std::string &filepath);
	RawHDRData LoadHDRFromFile(const std::string &filepath);

  private:
	static std::array<uint8, 4> ColorToPixel(vec4 color);
};

} // namespace Aquila::Graphics::Texture
#endif
