// Slug resolution-independent text rendering.
// MIT License. Original copyright 2017 Eric Lengyel.
// https://github.com/EricLengyel/Slug

#define kLogBandTextureWidth 12

struct VSInput {
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float4 color : COLOR;
	[[vk::location(2)]] float2 texcoord : TEXCOORD0; // em-space (interpolated)
	[[vk::location(3)]] float texLoc : TEXCOORD1;	 // packed glyphLoc  (asuint)
	[[vk::location(4)]] float bandMax : TEXCOORD2;	 // packed bandMax   (asuint)
	[[vk::location(5)]] float4 banding : TEXCOORD3;	 // band transform
};

struct VSOutput {
	float4 position : SV_Position;
	float4 color : COLOR;
	float2 texcoord : TEXCOORD0;
	nointerpolation float4 banding : TEXCOORD1;
	nointerpolation int4 glyphData : TEXCOORD2; // (locX, locY, bandMaxX, bandMaxY)
};

struct PushConstants {
	column_major float4x4 viewProjection;
};

[[vk::push_constant]]
PushConstants pc;

[[vk::binding(0, 0)]] Texture2D curveTexture;		// RGBA32F: 2 texels per curve
[[vk::binding(1, 0)]] Texture2D<uint4> bandTexture; // RGBA32U: band headers + index lists

// Unpack glyph location from the two reinterpreted floats.
[shader("vertex")] VSOutput main(VSInput input) {
	VSOutput output;
	output.position = mul(pc.viewProjection, float4(input.position, 1.0));
	output.position.y = -output.position.y; // Vulkan Y-flip
	output.color = input.color;
	output.texcoord = input.texcoord;
	output.banding = input.banding;

	uint2 g = uint2(asuint(input.texLoc), asuint(input.bandMax));
	output.glyphData = int4(int(g.x & 0xFFFFu), int(g.x >> 16u), int(g.y & 0xFFFFu), int(g.y >> 16u));
	return output;
}

// Slug algorithm helpers
uint CalcRootCode(float y1, float y2, float y3) {
	uint i1 = asuint(y1) >> 31U;
	uint i2 = asuint(y2) >> 30U;
	uint i3 = asuint(y3) >> 29U;
	uint shift = (i2 & 2U) | (i1 & ~2U);
	shift = (i3 & 4U) | (shift & ~4U);
	return ((0x2E74U >> shift) & 0x0101U);
}

float2 SolveHorizPoly(float4 p12, float2 p3) {
	// Solve for the values of t where the curve crosses y = 0.
	// The quadratic polynomial in t is given by
	//
	//     a t^2 - 2b t + c,
	//
	// where a = p1.y - 2 p2.y + p3.y, b = p1.y - p2.y, and c = p1.y.
	// The discriminant b^2 - ac is clamped to zero, and imaginary
	// roots are treated as a double root at the global minimum
	// where t = b / a.
	float2 a = p12.xy - p12.zw * 2.0 + p3;
	float2 b = p12.xy - p12.zw;
	float ra = 1.0 / a.y;
	float rb = 0.5 / b.y;
	float d = sqrt(max(b.y * b.y - a.y * p12.y, 0.0));
	float t1 = (b.y - d) * ra;
	float t2 = (b.y + d) * ra;
	if (abs(a.y) < 1.0 / 65536.0) {
		t1 = t2 = p12.y * rb;
	}
	return float2((a.x * t1 - b.x * 2.0) * t1 + p12.x, (a.x * t2 - b.x * 2.0) * t2 + p12.x);
}

float2 SolveVertPoly(float4 p12, float2 p3) {
	float2 a = p12.xy - p12.zw * 2.0 + p3;
	float2 b = p12.xy - p12.zw;
	float ra = 1.0 / a.x;
	float rb = 0.5 / b.x;
	float d = sqrt(max(b.x * b.x - a.x * p12.x, 0.0));
	float t1 = (b.x - d) * ra;
	float t2 = (b.x + d) * ra;
	if (abs(a.x) < 1.0 / 65536.0) {
		t1 = t2 = p12.x * rb;
	}
	return float2((a.y * t1 - b.y * 2.0) * t1 + p12.y, (a.y * t2 - b.y * 2.0) * t2 + p12.y);
}

int2 CalcBandLoc(int2 glyphLoc, uint offset) {
	int2 bandLoc = int2(glyphLoc.x + int(offset), glyphLoc.y);
	bandLoc.y += bandLoc.x >> kLogBandTextureWidth;
	bandLoc.x &= (1 << kLogBandTextureWidth) - 1;
	return bandLoc;
}

float CalcCoverage(float xcov, float ycov, float xwgt, float ywgt) {
	// Combine coverages from the horizontal and vertical rays using their weights.
	// Absolute values ensure that either winding direction convention works.

	float coverage = max(abs(xcov * xwgt + ycov * ywgt) / max(xwgt + ywgt, 1.0 / 65536.0), min(abs(xcov), abs(ycov)));
	return saturate(coverage);
}

//  Slug band-accelerated coverage.
[shader("fragment")] float4 main(VSOutput input) : SV_Target {
	float2 renderCoord = input.texcoord;
	float4 bandTransform = input.banding;
	int4 glyphData = input.glyphData;

	float2 emsPerPixel = fwidth(renderCoord);
	float2 pixelsPerEm = 1.0 / emsPerPixel;

	int2 bandMax = glyphData.zw;
	bandMax.y &= 0x00FF;

	int2 bandIndex = clamp(int2(renderCoord * bandTransform.xy + bandTransform.zw), int2(0, 0), bandMax);
	int2 glyphLoc = glyphData.xy;

	// Horizontal bands: accumulate x-coverage via horizontal ray.

	float xcov = 0.0;
	float xwgt = 0.0;

	uint2 hbandData = bandTexture.Load(int3(glyphLoc.x + bandIndex.y, glyphLoc.y, 0)).xy;
	int2 hbandLoc = CalcBandLoc(glyphLoc, hbandData.y);

	for (int ci = 0; ci < int(hbandData.x); ci++) {
		int2 curveLoc = int2(bandTexture.Load(int3(hbandLoc.x + ci, hbandLoc.y, 0)).xy);
		float4 p12 = curveTexture.Load(int3(curveLoc, 0)) - float4(renderCoord, renderCoord);
		float2 p3 = curveTexture.Load(int3(curveLoc.x + 1, curveLoc.y, 0)).xy - renderCoord;

		if (max(max(p12.x, p12.z), p3.x) * pixelsPerEm.x < -0.5) {
			break;
		}

		uint code = CalcRootCode(p12.y, p12.w, p3.y);
		if (code != 0U) {
			float2 r = SolveHorizPoly(p12, p3) * pixelsPerEm.x;
			if ((code & 1U) != 0U) {
				xcov += saturate(r.x + 0.5);
				xwgt = max(xwgt, saturate(1.0 - abs(r.x) * 2.0));
			}
			if (code > 1U) {
				xcov -= saturate(r.y + 0.5);
				xwgt = max(xwgt, saturate(1.0 - abs(r.y) * 2.0));
			}
		}
	}

	// Vertical bands: accumulate y-coverage via vertical ray.

	float ycov = 0.0;
	float ywgt = 0.0;

	uint2 vbandData = bandTexture.Load(int3(glyphLoc.x + bandMax.y + 1 + bandIndex.x, glyphLoc.y, 0)).xy;
	int2 vbandLoc = CalcBandLoc(glyphLoc, vbandData.y);

	for (int ci = 0; ci < int(vbandData.x); ci++) {
		int2 curveLoc = int2(bandTexture.Load(int3(vbandLoc.x + ci, vbandLoc.y, 0)).xy);
		float4 p12 = curveTexture.Load(int3(curveLoc, 0)) - float4(renderCoord, renderCoord);
		float2 p3 = curveTexture.Load(int3(curveLoc.x + 1, curveLoc.y, 0)).xy - renderCoord;

		if (max(max(p12.y, p12.w), p3.y) * pixelsPerEm.y < -0.5) {
			break;
		}

		uint code = CalcRootCode(p12.x, p12.z, p3.x);
		if (code != 0U) {
			float2 r = SolveVertPoly(p12, p3) * pixelsPerEm.y;
			if ((code & 1U) != 0U) {
				ycov -= saturate(r.x + 0.5);
				ywgt = max(ywgt, saturate(1.0 - abs(r.x) * 2.0));
			}
			if (code > 1U) {
				ycov += saturate(r.y + 0.5);
				ywgt = max(ywgt, saturate(1.0 - abs(r.y) * 2.0));
			}
		}
	}

	float coverage = CalcCoverage(xcov, ycov, xwgt, ywgt);
	return input.color * coverage;
}
