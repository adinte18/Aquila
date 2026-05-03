struct VSInput {
	float3 position : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};

struct VSOutput {
	float4 position : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};

struct PushConstants {
	column_major float4x4 viewProjection;
};

[[vk::push_constant]]
PushConstants pc;

[[vk::binding(0, 0)]]
Sampler2D atlas;

[shader("vertex")] VSOutput main(VSInput input) {
	VSOutput output;
	output.position = mul(pc.viewProjection, float4(input.position, 1.0));
	output.position.y = -output.position.y; // Vulkan Y flip
	output.color = input.color;
	output.uv = input.uv;
	return output;
}

	[shader("fragment")] float4 main(VSOutput input)
	: SV_Target {
	float alpha = atlas.Sample(input.uv).r;
	return float4(input.color.rgb, input.color.a * alpha);
}
