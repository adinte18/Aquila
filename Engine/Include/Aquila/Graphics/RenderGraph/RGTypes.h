#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::Graphics::RG {

template <typename Tag> struct RGHandle {
	static constexpr uint32 Invalid = UINT32_MAX;
	uint32 id = Invalid;

	[[nodiscard]] bool IsValid() const { return id != Invalid; }
	bool operator==(const RGHandle &) const = default;
};

struct RGTextureTag {};
struct RGBufferTag {};

using RGTextureHandle = RGHandle<RGTextureTag>;
using RGBufferHandle = RGHandle<RGBufferTag>;

using ResourceState = RHI::ResourceState;

struct RGTextureDesc {
	uint32 width = 1;
	uint32 height = 1;
	uint32 mipLevels = 1;
	uint32 arrayLayers = 1;
	RHI::TextureFormat format = RHI::TextureFormat::RGBA8;
	RHI::TextureUsage usage = RHI::TextureUsage::ColorAttachment | RHI::TextureUsage::Sampled;
	RHI::SampleCount samples = RHI::SampleCount::x1;
	std::string_view debugName;
};

struct RGBufferDesc {
	uint64 size = 0;
	RHI::BufferUsage usage = RHI::BufferUsage::None;
	RHI::MemoryDomain domain = RHI::MemoryDomain::GPU_ONLY;
	std::string_view debugName;
};

enum class AttachmentLoadOp : uint8 { Load, Clear, DontCare };
enum class AttachmentStoreOp : uint8 { Store, DontCare };

struct ClearColor {
	vec4 color;
};

struct ClearDepth {
	float depth = 1.f;
	uint8 stencil = 0;
};

} // namespace Aquila::Graphics::RG
