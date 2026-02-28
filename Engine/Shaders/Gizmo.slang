
#ifndef AQUILA_GIZMO_SLANG
#define AQUILA_GIZMO_SLANG

[[vk::binding(0, 0)]]
cbuffer GizmoUBO {
	column_major float4x4 view;
	column_major float4x4 projection;
};

struct VSInput {
	[[vk::location(0)]]
	float3 position : POSITION;
	[[vk::location(1)]]
	float3 color : COLOR;
};

struct VSOutput {
	float4 sv_position : SV_Position;
	[[vk::location(0)]]
	float3 fragColor : TEXCOORD0;
};

[shader("vertex")]
VSOutput vertexMain(VSInput IN) {
	VSOutput OUT;
	OUT.sv_position = mul(projection, mul(view, float4(IN.position, 1.0f)));
	OUT.fragColor = IN.color;
	return OUT;
}

[shader("fragment")]
float4 fragmentMain(VSOutput IN) : SV_Target {
	return float4(IN.fragColor, 1.0f);
}

#endif
