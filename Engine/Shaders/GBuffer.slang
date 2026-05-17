
#ifndef AQUILA_GBUFFER_SLANG
#define AQUILA_GBUFFER_SLANG

import Utility.FrameData;
import Utility.SurfaceData;

// set 1: material textures
[[vk::binding(0, 1)]] Sampler2D albedoMap;
[[vk::binding(1, 1)]] Sampler2D normalMap;
[[vk::binding(2, 1)]] Sampler2D aoMap;
[[vk::binding(3, 1)]] Sampler2D emissiveMap;

struct PushConstants {
	column_major float4x4 modelMatrix;
	column_major float4x4 normalMatrix;
	uint materialIndex;
};

[[vk::push_constant]]
PushConstants push;

struct VSInput {
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float3 color : COLOR;
	[[vk::location(2)]] float3 normal : NORMAL;
	[[vk::location(3)]] float4 tangent : TANGENT; // w = bitangent sign
	[[vk::location(4)]] float2 uv : TEXCOORD0;
};

struct VSOutput {
	float4 sv_position : SV_Position;
	[[vk::location(0)]] float3 fragColor : TEXCOORD0;
	[[vk::location(1)]] float3 fragPosWorld : TEXCOORD1;
	[[vk::location(2)]] float3 fragNormal : TEXCOORD2;
	[[vk::location(3)]] float3 fragTangent : TEXCOORD3;
	[[vk::location(4)]] float3 fragBitangent : TEXCOORD4;
	[[vk::location(5)]] float2 fragUV : TEXCOORD5;
};

struct FSOutput {
	[[vk::location(0)]] float4 outPosition : SV_Target0; // xyz=worldPos,  w=metallic
	[[vk::location(1)]] float4 outNormal : SV_Target1;	 // xyz=normal,    w=roughness
	[[vk::location(2)]] float4 outAlbedo : SV_Target2;	 // xyz=albedo,    w=ao
	[[vk::location(3)]] float4 outEmissive : SV_Target3; // xyz=emissive,  w=emissiveIntensity
};

[shader("vertex")] VSOutput vertexMain(VSInput IN) {
	VSOutput OUT;

	float4 posWorld = mul(push.modelMatrix, float4(IN.position, 1.0));
	float3x3 normalMat = (float3x3)push.normalMatrix;

	float3 N = normalize(mul(normalMat, IN.normal));
	OUT.fragNormal = N;
	OUT.fragPosWorld = posWorld.xyz;
	OUT.fragColor = IN.color;
	OUT.fragUV = IN.uv;

	if (length(IN.tangent.xyz) > 0.01) {
		float3 T = normalize(mul(normalMat, IN.tangent.xyz));
		T = normalize(T - dot(T, N) * N);
		float3 B = cross(N, T) * IN.tangent.w;
		OUT.fragTangent = T;
		OUT.fragBitangent = B;
	} else {
		OUT.fragTangent = float3(1, 0, 0);
		OUT.fragBitangent = float3(0, 1, 0);
	}

	OUT.sv_position = mul(aqFrameData.mainCamera.projection, mul(aqFrameData.mainCamera.view, posWorld));
	return OUT;
}

	[shader("fragment")] FSOutput fragmentMain(VSOutput IN) {
	AqSurfaceData surf = AQ_GetSurface(push.materialIndex);

	float3x3 TBN = float3x3(normalize(IN.fragTangent), normalize(IN.fragBitangent), normalize(IN.fragNormal));

	// Albedo: texture * material tint; sRGB -> linear
	float3 texAlbedo = pow(albedoMap.Sample(IN.fragUV).rgb, float3(2.2, 2.2, 2.2));
	float3 albedo = texAlbedo * surf.albedo.rgb;

	// Normal mapping scaled by normalStrength
	float3 sampledNormal = normalMap.Sample(IN.fragUV).rgb * 2.0 - 1.0;
	sampledNormal.xy *= surf.normalStrength;
	float3 worldNormal = normalize(mul(sampledNormal, TBN));

	float ao = aoMap.Sample(IN.fragUV).r * surf.aoStrength;
	float3 emissive = emissiveMap.Sample(IN.fragUV).rgb * surf.emissive.rgb;

	FSOutput OUT;
	OUT.outPosition = float4(IN.fragPosWorld, surf.metallic);
	OUT.outNormal = float4(worldNormal * 0.5 + 0.5, surf.roughness);
	OUT.outAlbedo = float4(albedo, ao);
	OUT.outEmissive = float4(emissive, surf.emissive.a);
	return OUT;
}

#endif // AQUILA_GBUFFER_SLANG
