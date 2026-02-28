
#ifndef AQUILA_SHADOWS_SLANG
#define AQUILA_SHADOWS_SLANG

[[vk::binding(0, 0)]]
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

[[vk::push_constant]]
cbuffer PushConstants {
	column_major float4x4 modelMatrix;
	uint cascadeIndex;
};

struct VSInput {
	[[vk::location(0)]]
	float3 position : POSITION;
};

struct VSOutput {
	float4 sv_position : SV_Position;
};

[shader("vertex")]
VSOutput vertexMain(VSInput IN) {
	VSOutput OUT;
	float4 worldPos = mul(modelMatrix, float4(IN.position, 1.0f));

	column_major float4x4 lightMatrices[4] = { cascadeViewProj0, cascadeViewProj1, cascadeViewProj2, cascadeViewProj3 };

	OUT.sv_position = mul(lightMatrices[cascadeIndex], worldPos);
	return OUT;
}

[shader("fragment")]
void fragmentMain(VSOutput IN) {}

#endif
