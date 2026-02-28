
#ifndef AQUILA_COMPOSITE_SLANG
#define AQUILA_COMPOSITE_SLANG

import "Utils.slang";

[[vk::binding(0, 0)]]
Sampler2D litColor;
[[vk::binding(1, 0)]]
Sampler2D skyboxColor;
[[vk::binding(2, 0)]]
Sampler2D gizmoColor;

struct VSOutput {
	float4 position : SV_Position;
	float2 fragTexCoord : TEXCOORD0;
}

[shader("vertex")]
VSOutput vertexMain(uint vertexIndex: SV_VertexID) {
	VSOutput o;
	o.position = FullscreenVertexPosition(vertexIndex);
	o.fragTexCoord = FullscreenTexCoord(vertexIndex);
	return o;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) {
	float4 outColor = 0;
	float3 lit = litColor.Sample(input.fragTexCoord).rgb;
	float3 skybox = skyboxColor.Sample(input.fragTexCoord).rgb;
	float4 gizmo = gizmoColor.Sample(input.fragTexCoord);

	float3 scene = lit + skybox;

	float3 final = lerp(scene, gizmo.rgb, gizmo.a);

	outColor = float4(final, 1.0);
	return outColor;
}

#endif
