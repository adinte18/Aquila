
#ifndef AQUILA_GBUFFER_SLANG
#define AQUILA_GBUFFER_SLANG

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
// ------
[[vk::binding(0, 1)]]
Sampler2D albedoMap;
[[vk::binding(1, 1)]]
Sampler2D normalMap;
[[vk::binding(2, 1)]]
Sampler2D aoMap;
[[vk::binding(3, 1)]]
Sampler2D emissiveMap;

[[vk::binding(4, 1)]]
cbuffer MaterialUBO {
	float4 albedoColor;
	float4 emissiveColor;
	float metallic;
	float roughness;
};

struct PushConstants {
	column_major float4x4 modelMatrix;
	column_major float4x4 normalMatrix;
};

[[vk::push_constant]]
PushConstants push;

struct VSInput {
	[[vk::location(0)]]
	float3 position : POSITION;
	[[vk::location(1)]]
	float3 color : COLOR;
	[[vk::location(2)]]
	float3 normal : NORMAL;
	[[vk::location(3)]]
	float4 tangent : TANGENT; // w = bitangent sign
	[[vk::location(4)]]
	float2 uv : TEXCOORD0;
};

struct VSOutput {
	float4 sv_position : SV_Position;
	[[vk::location(0)]]
	float3 fragColor : TEXCOORD0;
	[[vk::location(1)]]
	float3 fragPosWorld : TEXCOORD1;
	[[vk::location(2)]]
	float3 fragNormal : TEXCOORD2;

	[[vk::location(3)]]
	float3 fragTangent : TEXCOORD3;
	[[vk::location(4)]]
	float3 fragBitangent : TEXCOORD4;
	[[vk::location(5)]]
	float2 fragUV : TEXCOORD5;
};

struct FSOutput {
	[[vk::location(0)]]
	float4 outPosition : SV_Target0;
	[[vk::location(1)]]
	float4 outNormal : SV_Target1;
	[[vk::location(2)]]
	float4 outAlbedo : SV_Target2;
};

[shader("vertex")]
VSOutput vertexMain(VSInput IN) {
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

	OUT.sv_position = mul(cameraProjection, mul(cameraView, posWorld));

	return OUT;
}

[shader("fragment")]
FSOutput fragmentMain(VSOutput IN) {
	float3x3 TBN = float3x3(normalize(IN.fragTangent), normalize(IN.fragBitangent), normalize(IN.fragNormal));

	float3 sampledAlbedo = pow(albedoMap.Sample(IN.fragUV).rgb, float3(2.2, 2.2, 2.2));
	float sampledAO = aoMap.Sample(IN.fragUV).r;
	float3 sampledNormal = normalMap.Sample(IN.fragUV).rgb;

	float3 localNormal = sampledNormal * 2.0 - 1.0;
	float3 worldNormal = normalize(mul(localNormal, TBN));

	float3 albedo = sampledAlbedo * albedoColor.rgb;

	FSOutput OUT;
	OUT.outPosition = float4(IN.fragPosWorld, metallic);
	OUT.outNormal = float4(worldNormal * 0.5 + 0.5, roughness);
	OUT.outAlbedo = float4(albedo, sampledAO);

	return OUT;
}

#endif // AQUILA_GBUFFER_SLANG
