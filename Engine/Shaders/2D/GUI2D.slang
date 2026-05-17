#ifndef AQUILA_GUI2D_SLANG
#define AQUILA_GUI2D_SLANG

struct VSInput {
	float3 position : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
	float2 size : TEXCOORD1;
	float4 radius : TEXCOORD2;
	float borderWidth : TEXCOORD3;
	float4 borderColor : TEXCOORD4;
};

struct VSOutput {
	float4 position : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
	float2 size : TEXCOORD1;
	float4 radius : TEXCOORD2;
	float borderWidth : TEXCOORD3;
	float4 borderColor : TEXCOORD4;
};

struct PushConstants {
	column_major float4x4 viewProjection;
};

[[vk::push_constant]]
PushConstants pc;

[shader("vertex")] VSOutput main(VSInput input) {
	VSOutput output;
	float4 pos = float4(input.position, 1.0);
	output.position = mul(pc.viewProjection, pos);
	output.position.y = -output.position.y; // Vulkan Y flip
	output.color = input.color;
	output.radius = input.radius;
	output.borderColor = input.borderColor;
	output.borderWidth = input.borderWidth;
	output.uv = input.uv;
	output.size = input.size;
	return output;
}

float sdRoundBox(float2 point, float2 halfSize, float4 r) {
	float2 pick = (point.x > 0.0) ? r.xy : r.zw;
	float corner = (point.y > 0.0) ? pick.x : pick.y;
	float2 q = abs(point) - halfSize + corner;
	return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - corner;
}

[shader("fragment")] float4 main(VSOutput input) : SV_Target {
	float2 localPos = (input.uv - 0.5) * input.size;
	float2 halfSize = input.size * 0.5;

	float dist = sdRoundBox(localPos, halfSize, input.radius);

	float outerAlpha = 1.0 - smoothstep(-0.5, 0.5, dist);
	float innerAlpha = 1.0 - smoothstep(-0.5, 0.5, dist + input.borderWidth);

	float4 col = lerp(input.borderColor, input.color, innerAlpha);
	col.a *= outerAlpha;
	return col;
}

#endif // AQUILA_GUI2D_SLANG
