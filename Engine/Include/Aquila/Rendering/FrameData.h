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
	mat4 prevViewProjection; // reserved for TAA; matches current VP until TAA is wired up

	vec4 position; // .xyz = world-space position, .w unused
	vec4 forward;  // .xyz = world-space forward direction, .w unused
	vec4 up;	   // .xyz = world-space up direction, .w unused
	vec4 right;	   // .xyz = world-space right direction, .w unused

	float nearPlane;
	float farPlane;
	float fov; // vertical FOV in degrees
	float aspectRatio;

	vec2 resolution; // viewport resolution in pixels
	vec2 jitter;	 // sub-pixel TAA jitter offset (unused until TAA)

	uint32 isOrthographic; // 1 if orthographic, 0 if perspective
	uint32 cameraIndex;	   // index of this camera in the cameras[] array
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
	vec2 _pad;
};

} // namespace Aquila::Rendering
