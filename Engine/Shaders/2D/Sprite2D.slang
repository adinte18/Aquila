#ifndef AQUILA_SPRITE2D_SLANG
#define AQUILA_SPRITE2D_SLANG

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
Sampler2D tex;

[shader("vertex")] VSOutput main(VSInput input) {
	VSOutput output;
	output.position = mul(pc.viewProjection, float4(input.position, 1.0));
	output.position.y = -output.position.y; // Vulkan y-flip
	output.color = input.color;
	output.uv = input.uv;
	return output;
}

	[shader("fragment")] float4 main(VSOutput input)
	: SV_Target {
	float4 texColor = tex.Sample(input.uv);
	return texColor * input.color;
}

#endif // AQUILA_SPRITE2D_SLANG
