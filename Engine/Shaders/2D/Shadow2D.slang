// Box-shadow SDF shader.
// borderWidth  = blur radius
// borderColor  = (shadowOffsetX, shadowOffsetY, sdfBoxHalfW, sdfBoxHalfH)

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
	float blur : TEXCOORD3;
	float4 shadowParams : TEXCOORD4;
};

struct PushConstants {
	column_major float4x4 viewProjection;
};

[[vk::push_constant]]
PushConstants pc;

[shader("vertex")] VSOutput main(VSInput input) {
	VSOutput output;
	output.position = mul(pc.viewProjection, float4(input.position, 1.0));
	output.position.y = -output.position.y;
	output.color = input.color;
	output.uv = input.uv;
	output.size = input.size;
	output.radius = input.radius;
	output.blur = input.borderWidth;
	output.shadowParams = input.borderColor;
	return output;
}

float sdRoundBox(float2 p, float2 halfSize, float4 r) {
	float2 pick = (p.x > 0.0) ? r.xy : r.zw;
	float corner = (p.y > 0.0) ? pick.x : pick.y;
	float2 q = abs(p) - halfSize + corner;
	return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - corner;
}

[shader("fragment")] float4 main(VSOutput input) : SV_Target {
	float2 localPos = (input.uv - 0.5) * input.size;
	float2 shadowOff = input.shadowParams.xy;
	float2 boxHalfSize = input.shadowParams.zw;
	float blur = max(input.blur, 1.0);

	// Shift localPos into widget-box space (box center = shadow quad center - offset)
	float2 sdfPos = localPos + shadowOff;

	float dist = sdRoundBox(sdfPos, boxHalfSize, input.radius);
	float alpha = exp(-max(dist, 0.0) * 2.0 / blur) * input.color.a;

	return float4(input.color.rgb, alpha);
}
