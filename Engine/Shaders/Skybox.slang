
#ifndef AQUILA_SKYBOX_SLANG
#define AQUILA_SKYBOX_SLANG

import "Utils.slang";

[[vk::binding(0, 0)]]
cbuffer CameraData {
	column_major float4x4 cameraView;
	column_major float4x4 cameraProjection;
	column_major float4x4 cameraViewProjection;
	column_major float4x4 cameraInverseView;
	column_major float4x4 cameraInverseProjection;
	float3 cameraPosition;
	float nearPlane;
	float farPlane;
};

[[vk::binding(0, 1)]]
Sampler2D skyboxEquirect;

[[vk::push_constant]]
cbuffer SkyboxPush {
	float intensity;
	float lod;
};

struct VSInput {
	[[vk::location(0)]]
	float3 position : POSITION;
};

struct VSOutput {
	float4 sv_position : SV_Position;
	[[vk::location(0)]]
	float3 uvw : TEXCOORD0;
};

[shader("vertex")]
VSOutput vertexMain(VSInput IN) {
	VSOutput OUT;

	float4x4 viewNoTranslation = float4x4(float4(cameraView[0].xyz, 0.0f), float4(cameraView[1].xyz, 0.0f),
										  float4(cameraView[2].xyz, 0.0f), float4(0.0f, 0.0f, 0.0f, 1.0f));

	float4 pos = mul(cameraProjection, mul(viewNoTranslation, float4(IN.position, 1.0f)));

	OUT.sv_position = pos.xyww;

	OUT.uvw = IN.position;

	return OUT;
}

float2 SampleSphericalMap(float3 v) {
	float3 dir = normalize(float3(v.x, -v.y, v.z));

	float theta = atan2(dir.x, dir.z);
	float phi = asin(clamp(dir.y, -1.0f, 1.0f));

	return float2(theta * INV_2PI + 0.5f, phi * INV_PI + 0.5f);
}

[shader("fragment")]
float4 fragmentMain(VSOutput IN) : SV_Target {
	float2 uv = SampleSphericalMap(IN.uvw);

	float lodFloor = floor(lod);
	float lodCeil = lodFloor + 1.0f;
	float lodFrac = frac(lod);

	float3 colorLow = skyboxEquirect.SampleLevel(uv, lodFloor).rgb;
	float3 colorHigh = skyboxEquirect.SampleLevel(uv, lodCeil).rgb;
	float3 color = lerp(colorLow, colorHigh, lodFrac);

	return float4(color * intensity, 1.0f);
}
#endif
