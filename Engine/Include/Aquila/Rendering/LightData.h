#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Rendering {

enum class GpuLightType : int32 {
	Point = 0,
	Directional = 1,
	Spot = 2,
	Area = 3,
};

struct alignas(16) GpuLightData {
	vec4 positionAndRange;
	vec4 colorAndIntensity;
	vec4 directionAndType;
	vec4 rightAndWidth;
	vec4 upAndHeight;
	f32 cosInnerAngle;
	f32 cosOuterAngle;
	int32 shadowIndex;
	uint32 flags;
};

static_assert(sizeof(GpuLightData) == 96, "GpuLightData must be 96 bytes for std430 packing");

struct alignas(16) GpuEnvironmentData {
	vec4 shCoeffs[9];
	vec4 tintAndIntensity;
	int32 enabled;
	int32 _pad[3];
};

static_assert(sizeof(GpuEnvironmentData) == 176, "GpuEnvironmentData layout mismatch");

} // namespace Aquila::Rendering
