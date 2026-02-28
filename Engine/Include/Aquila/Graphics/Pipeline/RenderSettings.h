#ifndef AQUILA_RENDER_PIPELINE_SETTINGS_H
#define AQUILA_RENDER_PIPELINE_SETTINGS_H

#include "Aquila/Core/Defines.h"
#include "Aquila/Core/AquilaCore.h"

namespace Aquila::Graphics::Defaults {

constexpr bool ShadowsEnabled = true;
constexpr uint32 ShadowMapResolution = 2048;
constexpr uint32 ShadowCascadeCount = 4;
constexpr f32 ShadowCascadeLambda = 0.95f;
constexpr f32 ShadowBias = 0.005f;
constexpr f32 ShadowNormalBias = 0.001f;
constexpr f32 ShadowLightSize = 1.0f;
constexpr int32 ShadowPCFSamples = 16;
constexpr f32 ShadowMaxDistance = 100.0f;

constexpr f32 AmbientIntensity = 0.2f;
constexpr vec3 AmbientColor = { 0.1f, 0.1f, 0.15f };

constexpr uint32 MaxPointLights = 32;
constexpr uint32 MaxSpotLights = 16;
constexpr uint32 MaxDirectionalLights = 4;

constexpr f32 CameraNearPlane = 0.1f;
constexpr f32 CameraFarPlane = 1000.0f;

constexpr vec2 DefaultScreenSize = { 1920.0f, 1080.0f };

} // namespace Aquila::Graphics::Defaults

namespace Aquila::Graphics {

struct ShadowSettings {
	bool enabled = Defaults::ShadowsEnabled;
	uint32 resolution = Defaults::ShadowMapResolution;
	uint32 cascadeCount = Defaults::ShadowCascadeCount;
	f32 cascadeSplitLambda = Defaults::ShadowCascadeLambda;
	f32 shadowBias = Defaults::ShadowBias;
	f32 normalBias = Defaults::ShadowNormalBias;
	f32 lightSize = Defaults::ShadowLightSize;
	int32 pcfSamples = Defaults::ShadowPCFSamples;
	f32 maxShadowDistance = Defaults::ShadowMaxDistance;
};

struct LightingSettings {
	f32 ambientIntensity = 0.2f;
	vec3 ambientColor = vec3(0.1f, 0.1f, 0.15f);
};

} // namespace Aquila::Graphics

#endif
