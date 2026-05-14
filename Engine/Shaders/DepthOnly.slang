import FrameData;

struct PushConstants {
	column_major float4x4 model;
};

[[vk::push_constant]]
PushConstants push;

struct VSInput {
	[[vk::location(0)]] float3 position : POSITION;
};

[shader("vertex")] float4 main(VSInput IN) : SV_Position {
	return mul(AQ_GetMainCamera().viewProjection, mul(push.model, float4(IN.position, 1.0)));
}
