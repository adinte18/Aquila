#ifndef AQUILA_SELECTION_OUTLINE_SLANG
#define AQUILA_SELECTION_OUTLINE_SLANG

import "Utils.slang";

struct PushConstants {
	float4 outlineColor;
	float2 texelSize;
	float thickness;
	float padding;
};

[[vk::push_constant]]
PushConstants push;

[[vk::binding(0, 0)]]
Sampler2D maskTexture;

struct VSOutput {
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

static const float2 RingDirections[8] = {
	float2(1.0, 0.0),  float2(-1.0, 0.0), float2(0.0, 1.0),  float2(0.0, -1.0),
	float2(0.707, 0.707), float2(-0.707, 0.707), float2(0.707, -0.707), float2(-0.707, -0.707)
};

[shader("vertex")]
VSOutput main(uint vertexIndex : SV_VertexID) {
	VSOutput OUT;
	OUT.position = FullscreenVertexPosition(vertexIndex);
	OUT.uv = FullscreenTexCoord(vertexIndex);
	return OUT;
}

[shader("fragment")]
float4 main(VSOutput IN) : SV_Target {
	if (maskTexture.Sample(IN.uv).r > 0.5) {
		discard;
	}

	float neighbour = 0.0;

	for (int ring = 1; ring <= int(push.thickness); ++ring) {
		float radius = float(ring);
		for (int i = 0; i < 8; ++i) {
			float2 offset = RingDirections[i] * radius * push.texelSize;
			neighbour = max(neighbour, maskTexture.Sample(IN.uv + offset).r);
		}
	}

	if (neighbour < 0.5) {
		discard;
	}

	return push.outlineColor;
}

#endif
