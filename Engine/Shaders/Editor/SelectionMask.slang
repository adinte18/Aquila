#ifndef AQUILA_SELECTION_MASK_SLANG
#define AQUILA_SELECTION_MASK_SLANG

import Utility.FrameData;

using namespace Aquila::Shading;

struct PushConstants {
	column_major float4x4 model;
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
	float4 tangent : TANGENT;
	[[vk::location(4)]]
	float2 uv : TEXCOORD0;
};

struct VSOutput {
	float4 sv_position : SV_Position;
};

[shader("vertex")]
VSOutput main(VSInput IN) {
	VSOutput OUT;

	float4 worldPos = mul(push.model, float4(IN.position, 1.0));

	OUT.sv_position = mul(GetMainCamera().viewProjection, worldPos);

	return OUT;
}

[shader("fragment")]
float main(VSOutput IN) : SV_Target {
	return 1.0;
}

#endif
