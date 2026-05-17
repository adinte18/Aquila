#ifndef AQUILA_BASIC_SLANG
#define AQUILA_BASIC_SLANG

import Utility.FrameData;
import Utility.LightData;
import Utility.SurfaceData;
import Utility.ClusterData;

using namespace Aquila::Shading;

struct PushConstants {
	column_major float4x4 model;
	float4 color;
	uint materialIndex;
};

[[vk::push_constant]]
PushConstants push;

struct VSInput {
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float3 color : COLOR;
	[[vk::location(2)]] float3 normal : NORMAL;
	[[vk::location(3)]] float4 tangent : TANGENT;
	[[vk::location(4)]] float2 uv : TEXCOORD0;
};

struct VSOutput {
	float4 sv_position : SV_Position;
	[[vk::location(0)]] float3 worldPos : TEXCOORD0;
	[[vk::location(1)]] float3 normal : TEXCOORD1;
	[[vk::location(2)]] float2 uv : TEXCOORD2;
};

[shader("vertex")] VSOutput main(VSInput IN) {
	VSOutput OUT;
	float4 worldPos = mul(push.model, float4(IN.position, 1.0));
	OUT.sv_position = mul(GetMainCamera().viewProjection, worldPos);
	OUT.worldPos = worldPos.xyz;
	OUT.normal = normalize(mul((float3x3)push.model, IN.normal));
	OUT.uv = IN.uv;
	return OUT;
}

	[shader("fragment")] float4 main(VSOutput IN)
	: SV_Target {
	SurfaceData surf = GetSurface(push.materialIndex);
	float3 albedo = surf.albedo.rgb * push.color.rgb;

	float3 N = normalize(IN.normal);
	float3 V = normalize(GetCameraPosition() - IN.worldPos);

	float viewZ = mul(GetMainCamera().view, float4(IN.worldPos, 1.0)).z;
	uint clusterIdx = GetClusterIndex(IN.sv_position.xy, viewZ);
	ClusterLightInfo clusterInfo = GetClusterInfo(clusterIdx);

	float3 radiance = float3(0.0);

	for (uint i = 0; i < clusterInfo.count; ++i) {
		uint lightIdx = GetLightListIndex(clusterInfo.offset + i);
		LightData light = GetLight(lightIdx);
		float3 lightColor = GetLightColor(light) * GetLightIntensity(light);

		if (IsDirectionalLight(light)) {
			float3 L = normalize(-GetLightDirection(light));
			float NdotL = max(dot(N, L), 0.0);
			radiance += lightColor * NdotL;
		} else {
			float3 toLight = GetLightPosition(light) - IN.worldPos;
			float dist = length(toLight);
			float3 L = toLight / dist;
			float NdotL = max(dot(N, L), 0.0);
			float atten = Attenuation(dist, GetLightRange(light));

			float spotAtten = 1.0;
			if (IsSpotLight(light)) {
				spotAtten = SpotAttenuation(L, light);
			}

			radiance += lightColor * NdotL * atten * spotAtten;
		}
	}

	float3 ambient = albedo * surf.aoStrength * 0.03;
	float3 color = ambient + albedo * radiance;

	return float4(color, surf.albedo.a);
}

#endif // AQUILA_BASIC_SLANG
