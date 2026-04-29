struct VSInput {
	float3 position : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};

struct VSOutput {
	float4 position : SV_Position;
	float4 color : COLOR;
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
	return output;
}

	[shader("fragment")] float4 main(VSOutput input)
	: SV_Target {
	return input.color;
}
