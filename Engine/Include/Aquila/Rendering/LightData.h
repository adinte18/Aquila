#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Rendering {

enum class GpuLightType : int8 {
	Point = 0,
	Directional = 1,
	Spot = 2,
	Area = 3,
};

struct alignas(16) GpuLightData {
	vec4 m_PositionAndRange;
	vec4 m_ColorAndIntensity;
	vec4 m_DirectionAndType;
	vec4 m_RightAndWidth;
	vec4 m_UpAndHeight;
	f32 m_CosInnerAngle;
	f32 m_CosOuterAngle;
	int32 m_ShadowIndex;
	uint32 m_Flags;
};

static_assert(sizeof(GpuLightData) == 96, "GpuLightData must be 96 bytes for std430 packing");

struct alignas(16) GpuEnvironmentData {
	std::array<vec4, 9> m_ShCoeffs;
	vec4 m_TintAndIntensity;
	int32 m_Enabled;
	std::array<int32, 3> m_Padding;
};

static_assert(sizeof(GpuEnvironmentData) == 176, "GpuEnvironmentData layout mismatch");

} // namespace Aquila::Rendering
