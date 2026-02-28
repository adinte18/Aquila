
#ifndef UTILITIES_SLANG
#define UTILITIES_SLANG

// Constants
static const float PI = 3.14159265359;
static const float TWO_PI = 6.28318530718;
static const float INV_PI = 0.31830988618;
static const float INV_2PI = 0.15915494309;
static const float EPSILON = 1e-5;

static const float2 FullscreenTrianglePositions[3] = { float2(-1.0, -1.0), float2(3.0, -1.0), float2(-1.0, 3.0) };

inline float4 FullscreenVertexPosition(uint vertexIndex) {
	float2 pos = FullscreenTrianglePositions[vertexIndex];
	return float4(pos, 0.0, 1.0);
}

inline float2 FullscreenTexCoord(uint vertexIndex) {
	float2 pos = FullscreenTrianglePositions[vertexIndex];
	return pos * 0.5 + 0.5;
}

#endif
