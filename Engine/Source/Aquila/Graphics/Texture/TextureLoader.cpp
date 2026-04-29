#include "Aquila/Graphics/Texture/TextureLoader.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Aquila::Graphics::Texture {

TextureLoader::RawImageData TextureLoader::LoadFromFile(const std::string &filepath) {
	RawImageData data;

	int width = 0, height = 0, channels = 0;
	stbi_uc *pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

	data.width = static_cast<uint32>(width);
	data.height = static_cast<uint32>(height);
	data.channels = static_cast<uint32>(channels);

	if (pixels != nullptr) {
		size_t size = static_cast<size_t>(data.width * data.height) * 4;
		data.pixels = CreateUnique<uint8[]>(size);
		std::memcpy(data.pixels.get(), pixels, size);
		stbi_image_free(pixels);
	}

	return data;
}

TextureLoader::RawImageData TextureLoader::LoadFromVFS(const std::string &filepath) {
	auto file = Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(filepath, AccessMode::Read, OpenMode::Binary);
	if (!file || !file->IsValid()) {
		throw std::runtime_error("Failed to open VFS file: " + filepath);
	}

	int64 size = file->Size();
	if (size <= 0) {
		throw std::runtime_error("VFS file has invalid size: " + filepath);
	}

	std::vector<char> buffer(size);
	size_t bytesRead = file->Read(buffer.data(), size);
	if (bytesRead != static_cast<size_t>(size)) {
		throw std::runtime_error("Failed to read complete VFS file: " + filepath);
	}

	RawImageData data;

	int width = 0, height = 0, channels = 0;
	stbi_uc *pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(buffer.data()),
											static_cast<int>(bytesRead), &width, &height, &channels, STBI_rgb_alpha);

	data.width = static_cast<uint32>(width);
	data.height = static_cast<uint32>(height);
	data.channels = static_cast<uint32>(channels);

	if (pixels != nullptr) {
		size_t pixelSize = static_cast<size_t>(data.width * data.height) * 4;
		data.pixels = CreateUnique<uint8[]>(pixelSize);
		std::memcpy(data.pixels.get(), pixels, pixelSize);
		stbi_image_free(pixels);
	}

	return data;
}

TextureLoader::RawHDRData TextureLoader::LoadHDRFromFile(const std::string &filepath) {
	stbi_set_flip_vertically_on_load(false);

	RawHDRData data{};

	if (filepath.find("://") != std::string::npos) {
		// Load from VFS
		auto file =
			Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(filepath, AccessMode::Read, OpenMode::Binary);
		if (!file || !file->IsValid()) {
			AQUILA_LOG_ERROR("Failed to open VFS file: {}", filepath);
			return data;
		}

		int64 size = file->Size();
		if (size <= 0) {
			AQUILA_LOG_ERROR("VFS file has invalid size: {}", filepath);
			return data;
		}

		std::vector<char> buffer(size);
		size_t bytesRead = file->Read(buffer.data(), size);
		if (bytesRead != static_cast<size_t>(size)) {
			AQUILA_LOG_ERROR("Failed to read complete VFS file: {}", filepath);
			return data;
		}

		int width = 0, height = 0, channels = 0;
		f32 *pixels = stbi_loadf_from_memory(reinterpret_cast<const stbi_uc *>(buffer.data()),
											 static_cast<int>(bytesRead), &width, &height, &channels, 4);

		data.width = static_cast<uint32>(width);
		data.height = static_cast<uint32>(height);
		data.channels = static_cast<uint32>(channels);

		if (pixels == nullptr) {
			AQUILA_LOG_ERROR("Failed to load HDR from VFS: {} - {}", filepath, stbi_failure_reason());
			return data;
		}

		size_t pixelCount = static_cast<size_t>(data.width) * data.height * 4;
		data.pixels = CreateUnique<f32[]>(pixelCount);
		std::memcpy(data.pixels.get(), pixels, pixelCount * sizeof(f32));
		stbi_image_free(pixels);

		AQUILA_LOG_INFO("Loaded HDR texture from VFS: {} ({}x{})", filepath, data.width, data.height);
	} else {
		int width = 0, height = 0, channels = 0;
		f32 *pixels = stbi_loadf(filepath.c_str(), &width, &height, &channels, 4);

		data.width = static_cast<uint32>(width);
		data.height = static_cast<uint32>(height);
		data.channels = static_cast<uint32>(channels);

		if (pixels == nullptr) {
			AQUILA_LOG_ERROR("Failed to load HDR file: {} - {}", filepath, stbi_failure_reason());
			return data;
		}

		size_t pixelCount = static_cast<size_t>(data.width) * data.height * 4;
		data.pixels = CreateUnique<f32[]>(pixelCount);
		std::memcpy(data.pixels.get(), pixels, pixelCount * sizeof(f32));
		stbi_image_free(pixels);

		AQUILA_LOG_INFO("Loaded HDR texture from file: {} ({}x{})", filepath, data.width, data.height);
	}

	return data;
}


std::array<uint8, 4> TextureLoader::ColorToPixel(vec4 color) {
	return { static_cast<uint8>(color.r * 255.0f), static_cast<uint8>(color.g * 255.0f),
			 static_cast<uint8>(color.b * 255.0f), static_cast<uint8>(color.a * 255.0f) };
}

} // namespace Aquila::Graphics::Texture
