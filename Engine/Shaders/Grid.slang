#ifndef AQUILA_GRID_SLANG
#define AQUILA_GRID_SLANG

import Utility.FrameData;
import "Utils.slang";

using namespace Aquila::Shading;

static const float kMajorGridDiv = 10.0;
static const float kAxisLineWidth = 0.01;
static const float kMajorLineWidth = 0.005;
static const float kMinorLineWidth = 0.003;
static const float kAxisDashScale = 1;

static const float4 kMajorLineColor = float4(1.0, 1.0, 1.0, 1.0);
static const float4 kMinorLineColor = float4(1.0, 1.0, 1.0, 1.0);
static const float4 kBaseColor = float4(0.0, 0.0, 0.0, 0.0);
static const float4 kXAxisColor = float4(1.0, 0.0, 0.0, 1.0);
static const float4 kXAxisDashColor = float4(0.5, 0.0, 0.0, 1.0);
static const float4 kYAxisColor = float4(0.0, 1.0, 0.0, 1.0);
static const float4 kYAxisDashColor = float4(0.0, 0.5, 0.0, 1.0);
static const float4 kZAxisColor = float4(0.0, 0.0, 1.0, 1.0);
static const float4 kZAxisDashColor = float4(0.0, 0.0, 0.5, 1.0);
static const float4 kCenterColor = float4(1.0, 1.0, 1.0, 1.0);

struct VSOutput {
	float4 position : SV_Position;
	float2 ndc : TEXCOORD0;
};

[shader("vertex")]
VSOutput main(uint vertexIndex : SV_VertexID) {
	VSOutput o;
	float4 clip = FullscreenVertexPosition(vertexIndex);
	o.position = clip;
	o.ndc = clip.xy;
	return o;
}

struct FSOutput {
	float4 color : SV_Target;
	float depth : SV_Depth;
};

[shader("fragment")]
FSOutput main(VSOutput IN) {
	FSOutput o;
	o.color = float4(0.0);
	o.depth = 1.0;

	float3 nearPoint = ClipToWorld(float4(IN.ndc, 0.0, 1.0));
	float3 farPoint = ClipToWorld(float4(IN.ndc, 1.0, 1.0));
	float3 rayDir = farPoint - nearPoint;

	float t = -nearPoint.y / rayDir.y;
	if (t <= 0.0) {
		discard;
		return o;
	}

	float3 worldPos = nearPoint + t * rayDir;
	float3 camPos = GetCameraPosition();

	float div = max(2.0, round(kMajorGridDiv));

	float2 camOff = floor(float2(camPos.z, camPos.x) / div) * div;
	float2 uvXY = float2(worldPos.z, worldPos.x) - camOff;
	float2 uvZW = float2(worldPos.z, worldPos.x);

	float4 uvDDXY = float4(ddx(uvXY), ddy(uvXY));
	float2 uvDeriv = float2(length(uvDDXY.xz), length(uvDDXY.yw));

	float axisLineWidth = max(kMajorLineWidth, kAxisLineWidth);
	float2 axisDrawWidth = max(float2(axisLineWidth), uvDeriv);
	float2 axisLineAA = uvDeriv * 1.5;
	float2 axisLines2 = smoothstep(axisDrawWidth + axisLineAA, axisDrawWidth - axisLineAA, abs(uvZW * 2.0));
	axisLines2 *= saturate(axisLineWidth / axisDrawWidth);

	float2 majorUVDeriv = uvDeriv / div;
	float majorLineWidth = kMajorLineWidth / div;
	float2 majorDrawWidth = clamp(float2(majorLineWidth), majorUVDeriv, float2(0.5));
	float2 majorLineAA = majorUVDeriv * 1.5;
	float2 majorGridUV = 1.0 - abs(frac(uvXY / div) * 2.0 - 1.0);
	float2 majorAxisOffset = (1.0 - saturate(abs(uvZW / div * 2.0))) * 2.0;
	majorGridUV += majorAxisOffset;
	float2 majorGrid2 = smoothstep(majorDrawWidth + majorLineAA, majorDrawWidth - majorLineAA, majorGridUV);
	majorGrid2 *= saturate(majorLineWidth / majorDrawWidth);
	majorGrid2 = saturate(majorGrid2 - axisLines2);
	majorGrid2 = lerp(majorGrid2, float2(majorLineWidth), saturate(majorUVDeriv * 2.0 - 1.0));

	float minorLineWidth = min(kMinorLineWidth, kMajorLineWidth);
	bool minorInvertLine = minorLineWidth > 0.5;
	float minorTargetWidth = minorInvertLine ? 1.0 - minorLineWidth : minorLineWidth;
	float2 minorDrawWidth = clamp(float2(minorTargetWidth), uvDeriv, float2(0.5));
	float2 minorLineAA = uvDeriv * 1.5;
	float2 minorGridUV = abs(frac(uvXY) * 2.0 - 1.0);
	minorGridUV = minorInvertLine ? minorGridUV : 1.0 - minorGridUV;
	float2 minorMajorOffset = (1.0 - saturate((1.0 - abs(frac(uvZW / div) * 2.0 - 1.0)) * div)) * 2.0;
	minorGridUV += minorMajorOffset;
	float2 minorGrid2 = smoothstep(minorDrawWidth + minorLineAA, minorDrawWidth - minorLineAA, minorGridUV);
	minorGrid2 *= saturate(minorTargetWidth / minorDrawWidth);
	minorGrid2 = saturate(minorGrid2 - axisLines2);
	minorGrid2 = lerp(minorGrid2, float2(minorTargetWidth), saturate(uvDeriv * 2.0 - 1.0));
	minorGrid2 = minorInvertLine ? 1.0 - minorGrid2 : minorGrid2;
	minorGrid2 = select(abs(uvZW) > 0.5, minorGrid2, float2(0.0));

	float minorGrid = lerp(minorGrid2.x, 1.0, minorGrid2.y);
	float majorGrid = lerp(majorGrid2.x, 1.0, majorGrid2.y);

	float2 axisDashUV = abs(frac((uvZW + axisLineWidth * 0.5) * kAxisDashScale) * 2.0 - 1.0) - 0.5;
	float2 axisDashDeriv = uvDeriv * kAxisDashScale * 1.5;
	float2 axisDash = smoothstep(-axisDashDeriv, axisDashDeriv, axisDashUV);
	axisDash = select(uvZW < 0.0, axisDash, float2(1.0));

	float4 aAxisColor = kXAxisColor;
	float4 bAxisColor = kZAxisColor;

	aAxisColor = lerp(kXAxisDashColor, aAxisColor, axisDash.y);
	bAxisColor = lerp(kZAxisDashColor, bAxisColor, axisDash.x);
	aAxisColor = lerp(aAxisColor, kCenterColor, axisLines2.y);

	float4 axisLines = lerp(bAxisColor * axisLines2.y, aAxisColor, axisLines2.x);

	float4 col = lerp(kBaseColor, kMinorLineColor, minorGrid * kMinorLineColor.a);
	col = lerp(col, kMajorLineColor, majorGrid * kMajorLineColor.a);
	col = col * (1.0 - axisLines.a) + axisLines;

	float4 clip = mul(GetMainCamera().viewProjection, float4(worldPos, 1.0));
	o.depth = clip.z / clip.w;
	o.color = col;
	return o;
}

#endif // AQUILA_GRID_SLANG
