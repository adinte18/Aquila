#ifndef AQUILA_PROCEDURAL_TRIANGLE_SLANG
#define AQUILA_PROCEDURAL_TRIANGLE_SLANG

// A hardcoded RGB triangle — no vertex buffer required.
// Driven entirely by SV_VertexID (draw 3 vertices, no index buffer).

struct VSOutput {
	float4 position : SV_Position;
	[[vk::location(0)]] float3 color : TEXCOORD0;
};

static const float2 Positions[3] = {
	float2( 0.0, -0.6),
	float2( 0.6,  0.6),
	float2(-0.6,  0.6),
};

static const float3 Colors[3] = {
	float3(1.0, 0.2, 0.2),
	float3(0.2, 1.0, 0.2),
	float3(0.2, 0.2, 1.0),
};

[shader("vertex")]
VSOutput main(uint id : SV_VertexID) {
	VSOutput o;
	o.position = float4(Positions[id], 0.0, 1.0);
	o.color = Colors[id];
	return o;
}

[shader("fragment")]
float4 main(VSOutput input) : SV_Target {
	return float4(input.color, 1.0);
}

#endif // AQUILA_PROCEDURAL_TRIANGLE_SLANG
