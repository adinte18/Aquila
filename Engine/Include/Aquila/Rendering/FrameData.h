#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/SharedConstants.h"

namespace Aquila::Rendering {

struct alignas(16) GpuCameraData {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat4 inverseView;
	mat4 inverseProjection;
	mat4 inverseViewProjection;
	mat4 prevViewProjection;

	vec4 position;
	vec4 forward;
	vec4 up;
	vec4 right;

	float nearPlane;
	float farPlane;
	float fov;
	float aspectRatio;

	vec2 resolution;
	vec2 jitter;

	uint32 isOrthographic;
	uint32 cameraIndex;
	uint32 _pad[2];
};

struct GpuFrameData {
	GpuCameraData mainCamera;
	GpuCameraData cameras[SharedConstants::MAX_CAMERAS];

	uint32 cameraCount;
	float time;
	float deltaTime;
	uint32 frameIndex;

	vec2 screenResolution;
	uint32 lightCount;
	uint32 _pad;
};

} // namespace Aquila::Rendering
