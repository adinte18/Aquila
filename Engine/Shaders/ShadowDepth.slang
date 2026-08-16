#ifndef AQUILA_SHADOW_DEPTH_SLANG
#define AQUILA_SHADOW_DEPTH_SLANG

struct PushConstants {
	column_major float4x4 model;
	column_major float4x4 cascadeViewProj;
};

[[vk::push_constant]]
PushConstants push;

struct VSInput {
	[[vk::location(0)]] float3 position : POSITION;
};

struct VSOutput {
	float4 sv_position : SV_Position;
};

[shader("vertex")] VSOutput vertexMain(VSInput IN) {
	VSOutput OUT;
	float4 worldPos = mul(push.model, float4(IN.position, 1.0));
	OUT.sv_position = mul(push.cascadeViewProj, worldPos);
	return OUT;
}

#endif // AQUILA_SHADOW_DEPTH_SLANG
