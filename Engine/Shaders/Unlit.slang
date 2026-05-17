#ifndef AQUILA_UNLIT_SLANG
#define AQUILA_UNLIT_SLANG

import Utility.FrameData;
import Utility.SurfaceData;

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
	[[vk::location(1)]] float3 color    : COLOR;
	[[vk::location(2)]] float3 normal   : NORMAL;
	[[vk::location(3)]] float4 tangent  : TANGENT;
	[[vk::location(4)]] float2 uv       : TEXCOORD0;
};

struct VSOutput {
	float4 sv_position             : SV_Position;
	[[vk::location(0)]] float2 uv  : TEXCOORD0;
};

[shader("vertex")]
VSOutput main(VSInput IN) {
	VSOutput OUT;
	float4 worldPos  = mul(push.model, float4(IN.position, 1.0));
	OUT.sv_position  = mul(GetMainCamera().viewProjection, worldPos);
	OUT.uv           = IN.uv;
	return OUT;
}

[shader("fragment")]
float4 main(VSOutput IN) : SV_Target {
	SurfaceData surf = GetSurface(push.materialIndex);
	float3 color     = surf.albedo.rgb * push.color.rgb;
	return float4(color, surf.albedo.a);
}

#endif // AQUILA_UNLIT_SLANG
