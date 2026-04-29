
struct PushConstants {
	column_major float4x4 mvp;
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
	OUT.sv_position = mul(push.mvp, float4(IN.position, 1.0));
	OUT.color = IN.color;
	return OUT;
}

	[shader("fragment")] float4 main(VSOutput IN)
	: SV_Target {
	return float4(IN.color, 1.0);
}
