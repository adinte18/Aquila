
#ifndef AQUILA_UNLIT_SLANG
#define AQUILA_UNLIT_SLANG

[[vk::binding(0, 0)]]
cbuffer SceneUniformData {
	column_major float4x4 cameraProjection;
	column_major float4x4 cameraView;
	column_major float4x4 inverseView;
};

[[vk::binding(0, 1)]]
Sampler2D mainTexture;

[[vk::binding(1, 1)]]
cbuffer MaterialUBO {
	float4 color;
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
	float3 tangent : TANGENT;
	[[vk::location(4)]]
	float2 uv : TEXCOORD0;
};

struct VSOutput {
	float4 sv_position : SV_Position;
	[[vk::location(0)]]
	float3 fragPosition : TEXCOORD0;
	[[vk::location(1)]]
	float3 fragColor : TEXCOORD1;
	[[vk::location(2)]]
	float3 fragNormal : TEXCOORD2;
	[[vk::location(3)]]
	float3 fragTangent : TEXCOORD3;
	[[vk::location(4)]]
	float2 fragUV : TEXCOORD4;
};

[shader("vertex")]
VSOutput vertexMain(VSInput IN) {
	VSOutput OUT;

	float4 posWorld = mul(push.modelMatrix, float4(IN.position, 1.0));

	float3x3 normalMat = (float3x3)push.normalMatrix;
	OUT.fragNormal = normalize(mul(normalMat, IN.normal));
	OUT.fragTangent = normalize(mul(normalMat, IN.tangent));

	OUT.fragPosition = posWorld.xyz;
	OUT.fragColor = IN.color;
	OUT.fragUV = IN.uv;

	OUT.sv_position = mul(cameraProjection, mul(cameraView, posWorld));

	return OUT;
}

[shader("fragment")]
float4 fragmentMain(VSOutput IN) : SV_Target {
	float4 texColor = mainTexture.Sample(IN.fragUV);
	return texColor * color;
}

#endif // AQUILA_UNLIT_SLANG
