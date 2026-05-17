#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Graphics {

struct alignas(16) GpuSurfaceData {
	vec4 albedo{ 1.f, 1.f, 1.f, 1.f };
	vec4 emissive{ 0.f, 0.f, 0.f, 0.f };
	f32 metallic = 0.f;
	f32 roughness = 0.5f;
	f32 normalStrength = 1.f;
	f32 aoStrength = 1.f;
	vec4 extra0{ 0.f };
	vec4 extra1{ 0.f };
};
static_assert(sizeof(GpuSurfaceData) == 80, "GpuSurfaceData must be 80 bytes (std430)");

} // namespace Aquila::Graphics
