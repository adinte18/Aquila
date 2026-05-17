#ifndef AQUILA_WAVE_SLANG
#define AQUILA_WAVE_SLANG

struct PushConstants {
	column_major float4x4 mvp;
	float time;
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
	[[vk::location(0)]] float3 color : TEXCOORD0;
};

[shader("vertex")] VSOutput main(VSInput IN) {
	VSOutput OUT;

	float3 pos = IN.position;
	pos.y += sin(pos.x * 5.0 + push.time * 3.0) * 0.1;

	OUT.sv_position = mul(push.mvp, float4(pos, 1.0));
	OUT.color = IN.color;
	return OUT;
}

	[shader("fragment")] float4 main(VSOutput IN)
	: SV_Target {
	return float4(IN.color, 1.0);
}

#endif // AQUILA_WAVE_SLANG
