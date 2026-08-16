#ifndef AQUILA_SHADOW_DATA_SLANG
#define AQUILA_SHADOW_DATA_SLANG

import FrameData;

namespace Aquila::Shading {

struct ShadowData {
	column_major float4x4 cascadeViewProj[4];
	float4 cascadeSplits;
	float4 lightDirection;
	float4 params; // x = depth bias, y = normal bias, z = pcf texel scale, w = shadow map size
	int enabled;
	int numCascades;
	int2 _pad;
};

[vk::binding(6, 0)] ConstantBuffer<ShadowData> shadowData;
[vk::binding(7, 0)] Sampler2DShadow shadowMap0;
[vk::binding(8, 0)] Sampler2DShadow shadowMap1;
[vk::binding(9, 0)] Sampler2DShadow shadowMap2;
[vk::binding(10, 0)] Sampler2DShadow shadowMap3;

float SampleCascade(int cascade, float2 uv, float depthRef) {
	if (cascade == 0) {
		return shadowMap0.SampleCmp(uv, depthRef);
	} else if (cascade == 1) {
		return shadowMap1.SampleCmp(uv, depthRef);
	} else if (cascade == 2) {
		return shadowMap2.SampleCmp(uv, depthRef);
	}
	return shadowMap3.SampleCmp(uv, depthRef);
}

float SampleCascadeVisibility(int cascade, float3 worldPos, float3 N, float3 L) {
	float NdotL = saturate(dot(N, L));
	float slope = saturate(1.0 - NdotL);
	float3 offsetPos = worldPos + N * shadowData.params.y * (1.0 + float(cascade)) * (0.5 + slope);

	float4 lightSpace = mul(transpose(shadowData.cascadeViewProj[cascade]), float4(offsetPos, 1.0));
	float3 proj = lightSpace.xyz / lightSpace.w;
	float2 uv = proj.xy * 0.5 + 0.5;
	float depthRef = proj.z;

	if (depthRef <= 0.0 || depthRef >= 1.0) {
		return 1.0;
	}
	if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
		return 1.0;
	}

	float bias = shadowData.params.x * (1.0 + float(cascade));
	depthRef -= bias;

	float texel = 1.0 / shadowData.params.w;
	float pcfScale = max(shadowData.params.z, 1.0);

	float sum = 0.0;
	const int R = 2;
	for (int x = -R; x <= R; ++x) {
		for (int y = -R; y <= R; ++y) {
			float2 offset = float2(float(x), float(y)) * texel * pcfScale;
			sum += SampleCascade(cascade, uv + offset, depthRef);
		}
	}
	return sum / float((2 * R + 1) * (2 * R + 1));
}

float ComputeDirectionalShadow(float3 worldPos, float3 N, float3 L) {
	if (shadowData.enabled == 0) {
		return 1.0;
	}

	float viewZ = mul(GetMainCamera().view, float4(worldPos, 1.0)).z;

	int cascade = -1;
	for (int i = 0; i < shadowData.numCascades; ++i) {
		if (viewZ <= shadowData.cascadeSplits[i]) {
			cascade = i;
			break;
		}
	}
	if (cascade < 0) {
		return 1.0;
	}

	float visibility = SampleCascadeVisibility(cascade, worldPos, N, L);

	float splitFar = shadowData.cascadeSplits[cascade];
	float splitNear = (cascade == 0) ? 0.0 : shadowData.cascadeSplits[cascade - 1];
	float bandStart = lerp(splitNear, splitFar, 0.8);
	if (cascade + 1 < shadowData.numCascades && viewZ > bandStart) {
		float t = saturate((viewZ - bandStart) / max(splitFar - bandStart, 1e-4));
		float nextVisibility = SampleCascadeVisibility(cascade + 1, worldPos, N, L);
		visibility = lerp(visibility, nextVisibility, t);
	}

	return visibility;
}

float3 CascadeDebugColor(float3 worldPos) {
	float viewZ = mul(GetMainCamera().view, float4(worldPos, 1.0)).z;
	int cascade = -1;
	for (int i = 0; i < shadowData.numCascades; ++i) {
		if (viewZ <= shadowData.cascadeSplits[i]) {
			cascade = i;
			break;
		}
	}
	if (cascade == 0) {
		return float3(1.0, 0.3, 0.3);
	}
	if (cascade == 1) {
		return float3(0.3, 1.0, 0.3);
	}
	if (cascade == 2) {
		return float3(0.3, 0.3, 1.0);
	}
	if (cascade == 3) {
		return float3(1.0, 1.0, 0.3);
	}
	return float3(0.3, 0.3, 0.3);
}

} // namespace Aquila::Shading

#endif // AQUILA_SHADOW_DATA_SLANG
