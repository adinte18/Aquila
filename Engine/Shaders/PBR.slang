
#ifndef AQUILA_DEFERRED_LIGHT_IBL_SLANG
#define AQUILA_DEFERRED_LIGHT_IBL_SLANG

import "Utils.slang";

static const int LIGHT_TYPE_POINT = 0;
static const int LIGHT_TYPE_DIRECTIONAL = 1;
static const int LIGHT_TYPE_SPOT = 2;

struct LightData {
	float3 color;
	float intensity;
	float3 direction;
	float range;
	float3 position;
	int type;
	float innerCone;
	float outerCone;
	int isActive;
	float _pad;
};

// -- set 0
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

// -- set 1
[[vk::binding(0, 1)]]
cbuffer LightsData {
	LightData lights[32];
};

[[vk::binding(1, 1)]]
cbuffer EnvironmentData {
	float4 shCoeffs[9];
	float3 tint;
	float intensity;
	int useEnvironment;
};
// --------------------

// set 2
[[vk::binding(0, 2)]]
Sampler2D gBufferPosition;
[[vk::binding(1, 2)]]
Sampler2D gBufferNormal;
[[vk::binding(2, 2)]]
Sampler2D gBufferAlbedo;
// --------------------

// set 3
[[vk::binding(0, 3)]]
Sampler2D shadowMap0;
[[vk::binding(1, 3)]]
Sampler2D shadowMap1;
[[vk::binding(2, 3)]]
Sampler2D shadowMap2;
[[vk::binding(3, 3)]]
Sampler2D shadowMap3;

[[vk::binding(4, 3)]]
cbuffer ShadowData {
	column_major float4x4 cascadeViewProj0;
	column_major float4x4 cascadeViewProj1;
	column_major float4x4 cascadeViewProj2;
	column_major float4x4 cascadeViewProj3;
	float4 cascadeSplits[4];
	float3 lightDirection;
	float lightSize;
	float shadowBias;
	float normalBias;
	int pcfSamples;
};
// --------------------

[[vk::push_constant]]
cbuffer PushConstants {
	uint lightCount;
};

struct VSOutput {
	float4 sv_position : SV_Position;
	[[vk::location(0)]]
	float2 inUV : TEXCOORD0;
};

[shader("vertex")]
VSOutput vertexMain(uint vertexIndex: SV_VertexID) {
	VSOutput OUT;
	OUT.sv_position = FullscreenVertexPosition(vertexIndex);
	OUT.inUV = FullscreenTexCoord(vertexIndex);
	return OUT;
}

float3 EvaluateSH(float3 n) {
	float3 irradiance =
		shCoeffs[0].rgb * 0.282095f + shCoeffs[1].rgb * (0.488603f * n.y) + shCoeffs[2].rgb * (0.488603f * n.z) +
		shCoeffs[3].rgb * (0.488603f * n.x) + shCoeffs[4].rgb * (1.092548f * n.x * n.y) +
		shCoeffs[5].rgb * (1.092548f * n.y * n.z) + shCoeffs[6].rgb * (0.315392f * (3.0f * n.z * n.z - 1.0f)) +
		shCoeffs[7].rgb * (1.092548f * n.x * n.z) + shCoeffs[8].rgb * (0.546274f * (n.x * n.x - n.y * n.y));

	return max(irradiance, float3(0.0f, 0.0f, 0.0f));
}

float DistributionGGX(float NdotH, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float d = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
	return a2 / (PI * d * d + EPSILON);
}

float GeometrySchlickGGX(float NdotV, float k) {
	return NdotV / (NdotV * (1.0f - k) + k + EPSILON);
}

float GeometrySmith(float NdotV, float NdotL, float roughness) {
	float k = (roughness * roughness) / 2.0f;
	return GeometrySchlickGGX(NdotV, k) * GeometrySchlickGGX(NdotL, k);
}

float3 FresnelSchlick(float cosTheta, float3 F0) {
	return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

float SampleShadowMap(int layer, float2 uv) {
	if (layer == 0)
		return shadowMap0.Sample(uv).r;
	else if (layer == 1)
		return shadowMap1.Sample(uv).r;
	else if (layer == 2)
		return shadowMap2.Sample(uv).r;
	else
		return shadowMap3.Sample(uv).r;
}

static const float2 poissonDisk32[32] = {
	float2(-0.94201624f, -0.39906216f), float2(0.94558609f, -0.76890725f),	float2(-0.094184101f, -0.92938870f),
	float2(0.34495938f, 0.29387760f),	float2(-0.91588581f, 0.45771432f),	float2(-0.81544232f, -0.87912464f),
	float2(-0.38277543f, 0.27676845f),	float2(0.97484398f, 0.75648379f),	float2(0.44323325f, -0.97511554f),
	float2(0.53742981f, -0.47373420f),	float2(-0.26496911f, -0.41893023f), float2(0.79197514f, 0.19090188f),
	float2(-0.24188840f, 0.99706507f),	float2(-0.81409955f, 0.91437590f),	float2(0.19984126f, 0.78641367f),
	float2(0.14383161f, -0.14100790f),	float2(-0.52584900f, 0.68013500f),	float2(-0.12833400f, 0.38928800f),
	float2(0.63664100f, 0.54947400f),	float2(-0.65450600f, -0.23901200f), float2(0.08837800f, -0.64228900f),
	float2(-0.45765200f, -0.77105900f), float2(0.71832700f, -0.21784000f),	float2(-0.13578100f, 0.83421300f),
	float2(0.28888300f, -0.27462200f),	float2(0.85044500f, 0.39589700f),	float2(-0.72395400f, 0.12210800f),
	float2(0.31790600f, 0.69786300f),	float2(-0.33393500f, -0.56830500f), float2(0.58299600f, -0.89964700f),
	float2(-0.78148900f, 0.52506700f),	float2(0.11348800f, 0.27096900f)
};

float random(float2 seed) {
	return frac(sin(dot(seed, float2(12.9898f, 78.233f))) * 43758.5453f);
}

float SearchWidth(float receiverDepth, float nearPlane) {
	return lightSize * (receiverDepth - nearPlane) / receiverDepth;
}

float FindBlockerDistance(int layer, float2 uv, float currentDepth, float searchWidth) {
	float blockerSum = 0.0f;
	int numBlockers = 0;

	for (int i = 0; i < 16; ++i) {
		float2 offset = poissonDisk32[i] * searchWidth;
		float shadowMapDepth = SampleShadowMap(layer, uv + offset);
		if (shadowMapDepth < currentDepth) {
			blockerSum += shadowMapDepth;
			numBlockers++;
		}
	}

	if (numBlockers == 0)
		return -1.0f;
	return blockerSum / float(numBlockers);
}

float PCF(int layer, float2 uv, float currentDepth, float bias, float filterRadius) {
	float shadow = 0.0f;
	float angle = random(uv) * 6.28318f;
	float cosA = cos(angle);
	float sinA = sin(angle);

	int sampleCount = pcfSamples <= 3 ? 8 : (pcfSamples <= 9 ? 16 : 32);

	for (int i = 0; i < sampleCount; ++i) {
		float2 offset = float2(cosA * poissonDisk32[i].x - sinA * poissonDisk32[i].y,
							   sinA * poissonDisk32[i].x + cosA * poissonDisk32[i].y) *
						filterRadius;
		float pcfDepth = SampleShadowMap(layer, uv + offset);
		shadow += ((currentDepth - bias) > pcfDepth) ? 1.0f : 0.0f;
	}
	return shadow / float(sampleCount);
}
// https://developer.download.nvidia.com/shaderlibrary/docs/shadow_PCSS.pdf
// https://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
float CalculateShadow(float3 worldPos, float3 normal, float3 lightDir) {
	float NdotL = clamp(dot(normal, lightDir), 0.0f, 1.0f);

	float3 offsetPos = worldPos + normal * normalBias * (1.0f - NdotL);

	float4 viewPos = mul(cameraView, float4(offsetPos, 1.0f));
	float depth = abs(viewPos.z);

	int layer = 3;
	for (int i = 0; i < 4; ++i) {
		if (depth < cascadeSplits[i].x) {
			layer = i;
			break;
		}
	}

	column_major float4x4 lightMatrices[4] = { cascadeViewProj0, cascadeViewProj1, cascadeViewProj2, cascadeViewProj3 };

	float4 lightSpacePos = mul(lightMatrices[layer], float4(offsetPos, 1.0f));
	float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
	projCoords.xy = projCoords.xy * 0.5f + 0.5f;

	float currentDepth = projCoords.z;
	if (currentDepth > 1.0f || currentDepth < 0.0f)
		return 0.0f;

	float bias = shadowBias;

	float2 texelSize = 1.0f / float2(2048.0f, 2048.0f);

	float searchWidth = SearchWidth(currentDepth, nearPlane) * texelSize.x;
	float avgBlockerDepth = FindBlockerDistance(layer, projCoords.xy, currentDepth, searchWidth);

	if (avgBlockerDepth < 0.0f)
		return 0.0f;

	float penumbra = (currentDepth - avgBlockerDepth) / avgBlockerDepth;
	float filterRadius = penumbra * nearPlane * texelSize.x * lightSize * 50.0f;
	float maxRadius = texelSize.x * (pcfSamples <= 3 ? 4.0f : (pcfSamples <= 9 ? 8.0f : 16.0f));
	filterRadius = clamp(filterRadius, texelSize.x * 2.0f, maxRadius);

	return PCF(layer, projCoords.xy, currentDepth, bias, filterRadius);
}
float3 EvaluateLightPBR(LightData light, float3 N, float3 V, float3 L, float3 albedo, float metallic, float roughness,
						float ao, float3 fragPos, float shadowFactor) {
	float3 H = normalize(L + V);
	float NdotH = max(dot(N, H), 0.0f);
	float NdotV = max(dot(N, V), 1e-4f);
	float NdotL = max(dot(N, L), 0.0f);
	float HdotV = max(dot(H, V), 0.0f);

	float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

	float D = DistributionGGX(NdotH, roughness);
	float G = GeometrySmith(NdotV, NdotL, roughness);
	float3 F = FresnelSchlick(HdotV, F0);
	float3 specular = (D * G * F) / (4.0f * NdotV * NdotL + EPSILON);

	float3 kD = (float3(1.0f, 1.0f, 1.0f) - F) * (1.0f - metallic);
	float3 diffuse = (albedo * INV_PI) * kD;

	float attenuation = 1.0f;
	if (light.type == LIGHT_TYPE_POINT || light.type == LIGHT_TYPE_SPOT) {
		float dist = length(light.position - fragPos);
		attenuation = 1.0f / (1.0f + 0.09f * dist + 0.032f * dist * dist);
		attenuation *= clamp(1.0f - pow(dist / light.range, 4.0f), 0.0f, 1.0f);
	}

	float3 radiance = light.color * light.intensity * attenuation;
	return (diffuse + specular) * radiance * NdotL * ao * shadowFactor;
}

float3 CalculateDirectionalLight(LightData light, float3 N, float3 V, float3 albedo, float3 fragPos, float metallic,
								 float roughness, float ao) {
	float3 L = normalize(-light.direction);
	float shadow = CalculateShadow(fragPos, N, L);
	return EvaluateLightPBR(light, N, V, L, albedo, metallic, roughness, ao, fragPos, 1.0f - shadow);
}

float3 CalculatePointLight(LightData light, float3 N, float3 V, float3 albedo, float3 fragPos, float metallic,
						   float roughness, float ao) {
	float3 L = normalize(light.position - fragPos);
	float dist = length(light.position - fragPos);
	if (dist > light.range)
		return float3(0.0f, 0.0f, 0.0f);
	return EvaluateLightPBR(light, N, V, L, albedo, metallic, roughness, ao, fragPos, 1.0f);
}

float3 CalculateSpotLight(LightData light, float3 N, float3 V, float3 albedo, float3 fragPos, float metallic,
						  float roughness, float ao) {
	float3 L = normalize(light.position - fragPos);
	float dist = length(light.position - fragPos);
	if (dist > light.range)
		return float3(0.0f, 0.0f, 0.0f);

	float theta = dot(L, normalize(-light.direction));
	float epsilon = light.innerCone - light.outerCone;
	float spotScale = clamp((theta - light.outerCone) / epsilon, 0.0f, 1.0f);
	if (spotScale <= 0.0f)
		return float3(0.0f, 0.0f, 0.0f);

	float3 result = EvaluateLightPBR(light, N, V, L, albedo, metallic, roughness, ao, fragPos, 1.0f);
	return result * spotScale;
}

float3 CalculateDiffuseBRDF(float3 albedo, float metallic) {
	float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
	float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - F0, float3(0.0f, 0.0f, 0.0f), metallic);
	return kD * albedo * INV_PI;
}

[shader("fragment")]
float4 fragmentMain(VSOutput IN) : SV_Target {
	float4 positionSample = gBufferPosition.Sample(IN.inUV);
	float4 normalSample = gBufferNormal.Sample(IN.inUV);
	float4 albedoSample = gBufferAlbedo.Sample(IN.inUV);

	float3 worldPos = positionSample.rgb;
	float3 N = normalize(normalSample.rgb * 2.0f - 1.0f);
	float3 albedo = albedoSample.rgb;
	float ao = albedoSample.a;
	float metallic = positionSample.a;
	float roughness = normalSample.a;

	float3 V = normalize(cameraPosition.xyz - worldPos);

	float3 directLighting = float3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < lightCount && i < 4; ++i) {
		LightData light = lights[i];
		if (light.isActive == 0)
			continue;

		if (light.type == LIGHT_TYPE_DIRECTIONAL)
			directLighting += CalculateDirectionalLight(light, N, V, albedo, worldPos, metallic, roughness, ao);
		else if (light.type == LIGHT_TYPE_POINT)
			directLighting += CalculatePointLight(light, N, V, albedo, worldPos, metallic, roughness, ao);
		else if (light.type == LIGHT_TYPE_SPOT)
			directLighting += CalculateSpotLight(light, N, V, albedo, worldPos, metallic, roughness, ao);
	}

	float3 indirectLighting = float3(0.0f, 0.0f, 0.0f);
	if (useEnvironment > 0) {
		float3 irradiance = EvaluateSH(N) * intensity * tint;
		indirectLighting = CalculateDiffuseBRDF(albedo, metallic) * irradiance * ao;
	}

	float3 finalColor = directLighting + indirectLighting;

	if (lightCount == 0 && useEnvironment == 0) {
		return float4(pow(albedo, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f)), 1.0f);
	}

	finalColor = finalColor / (finalColor + float3(1.0f, 1.0f, 1.0f));
	finalColor = pow(finalColor, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

	return float4(finalColor, 1.0f);
}

#endif
