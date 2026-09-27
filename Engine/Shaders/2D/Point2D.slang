#ifndef AQUILA_POINT2D_SLANG
#define AQUILA_POINT2D_SLANG

struct VSInput {
	[[vk::location(0)]] float2 position : POSITION;
	[[vk::location(1)]] float4 color : COLOR;
	[[vk::location(2)]] float size : PSIZE;
};

struct VSOutput {
	float4 position : SV_Position;
	float point_size : SV_PointSize;
	[[vk::location(0)]] float4 color : COLOR;
	[[vk::location(1)]] float size : TEXCOORD0;
};

struct PSInput {
	float4 position : SV_Position;
	[[vk::location(0)]] float4 color : COLOR;
	[[vk::location(1)]] float size : TEXCOORD0;
};

struct PushConstants {
	column_major float4x4 viewProjection;
};

[[vk::push_constant]]
PushConstants pc;

[shader("vertex")]
VSOutput main(VSInput input) {
	VSOutput output;
	output.position = mul(pc.viewProjection, float4(input.position, 0.0, 1.0));
	output.position.y = -output.position.y;
	output.point_size = max(input.size, 1.0);
	output.color = input.color;
	output.size = max(input.size, 1.0);
	return output;
}

[shader("fragment")]
float4 main(PSInput input, float2 point_coord : SV_PointCoord) : SV_Target {
	float radius = input.size * 0.5;
	float distance = length(point_coord - 0.5) * input.size;
	float coverage = saturate(radius - distance + 0.5);
	return float4(input.color.rgb, input.color.a * coverage);
}

#endif
