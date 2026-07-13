#ifndef AQUILA_GUI2D_SLANG
#define AQUILA_GUI2D_SLANG

struct VSInput {
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float4 color : COLOR;
	[[vk::location(2)]] float2 uv : TEXCOORD0;
	[[vk::location(3)]] float2 size : TEXCOORD1;
	[[vk::location(4)]] float4 radius : TEXCOORD2;
	[[vk::location(5)]] float borderWidth : TEXCOORD3;
	[[vk::location(6)]] float4 borderColor : TEXCOORD4;
	[[vk::location(8)]] float borderStyle : TEXCOORD5;
};

struct VSOutput {
	float4 position : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
	float2 size : TEXCOORD1;
	float4 radius : TEXCOORD2;
	float borderWidth : TEXCOORD3;
	float4 borderColor : TEXCOORD4;
	float borderStyle : TEXCOORD5;
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
	output.borderStyle = input.borderStyle;
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

float perimeterCoord(float2 point, float2 halfSize) {
	float distToVertical = halfSize.x - abs(point.x);
	float distToHorizontal = halfSize.y - abs(point.y);
	if (distToHorizontal <= distToVertical) {
		if (point.y < 0.0) {
			return point.x + halfSize.x;
		}
		return 2.0 * halfSize.x + 2.0 * halfSize.y + (halfSize.x - point.x);
	}
	if (point.x > 0.0) {
		return 2.0 * halfSize.x + (point.y + halfSize.y);
	}
	return 4.0 * halfSize.x + 2.0 * halfSize.y + (halfSize.y - point.y);
}

[shader("fragment")] float4 main(VSOutput input) : SV_Target {
	float2 localPos = (input.uv - 0.5) * input.size;
	float2 halfSize = input.size * 0.5;

	float dist = sdRoundBox(localPos, halfSize, input.radius);

	float outerAlpha = 1.0 - smoothstep(-0.5, 0.5, dist);
	float innerAlpha = 1.0 - smoothstep(-0.5, 0.5, dist + input.borderWidth);

	float ringCoverage = saturate(outerAlpha - innerAlpha);

	int style = (int)(input.borderStyle + 0.5);
	if (style != 0 && input.borderWidth > 0.0) {
		float period;
		float duty;
		if (style == 2) {
			period = max(input.borderWidth * 2.0, 2.0);
			duty = 0.5;
		} else {
			period = max(input.borderWidth * 3.0, 8.0);
			duty = 0.6;
		}
		float phase = frac(perimeterCoord(localPos, halfSize) / period);
		float aa = fwidth(phase);
		float on = 1.0 - smoothstep(duty - aa, duty + aa, phase);
		ringCoverage *= on;
	}

	float fillAlpha = input.color.a * innerAlpha;
	float borderAlpha = input.borderColor.a * ringCoverage;
	float outAlpha = borderAlpha + fillAlpha * (1.0 - borderAlpha);
	float3 outRgb =
		(input.borderColor.rgb * borderAlpha + input.color.rgb * fillAlpha * (1.0 - borderAlpha)) / max(outAlpha, 1e-5);
	return float4(outRgb, outAlpha);
}

#endif // AQUILA_GUI2D_SLANG
