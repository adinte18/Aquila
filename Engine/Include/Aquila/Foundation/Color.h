#ifndef AQUILA_COLOR_H
#define AQUILA_COLOR_H

#include "Aquila/Foundation/Math/MathTypes.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Aquila::Foundation::Color {

// RGBA clear colors
namespace RGBA {
constexpr vec4 Black = { 0.0F, 0.0F, 0.0F, 1.0F };
constexpr vec4 White = { 1.0F, 1.0F, 1.0F, 1.0F };
constexpr vec4 Red = { 1.0F, 0.0F, 0.0F, 1.0F };
constexpr vec4 Green = { 0.0F, 1.0F, 0.0F, 1.0F };
constexpr vec4 Blue = { 0.0F, 0.0F, 1.0F, 1.0F };
constexpr vec4 Yellow = { 1.0F, 1.0F, 0.0F, 1.0F };
constexpr vec4 Cyan = { 0.0F, 1.0F, 1.0F, 1.0F };
constexpr vec4 Magenta = { 1.0F, 0.0F, 1.0F, 1.0F };
constexpr vec4 Orange = { 1.0F, 0.5F, 0.0F, 1.0F };
constexpr vec4 Gray = { 0.5F, 0.5F, 0.5F, 1.0F };
constexpr vec4 LightGray = { 0.75F, 0.75F, 0.75F, 1.0F };
constexpr vec4 DarkGray = { 0.25F, 0.25F, 0.25F, 1.0F };
} // namespace RGBA

// RGB colors
constexpr vec3 Black_v = { 0.0F, 0.0F, 0.0F };
constexpr vec3 White_v = { 1.0F, 1.0F, 1.0F };
constexpr vec3 Red_v = { 1.0F, 0.0F, 0.0F };
constexpr vec3 Green_v = { 0.0F, 1.0F, 0.0F };
constexpr vec3 Blue_v = { 0.0F, 0.0F, 1.0F };
constexpr vec3 Yellow_v = { 1.0F, 1.0F, 0.0F };
constexpr vec3 Cyan_v = { 0.0F, 1.0F, 1.0F };
constexpr vec3 Magenta_v = { 1.0F, 0.0F, 1.0F };
constexpr vec3 Orange_v = { 1.0F, 0.5F, 0.0F };
constexpr vec3 Gray_v = { 0.5F, 0.5F, 0.5F };
constexpr vec3 LightGray_v = { 0.75F, 0.75F, 0.75F };
constexpr vec3 DarkGray_v = { 0.25F, 0.25F, 0.25F };

// Console colors
constexpr const char *Reset = "\033[0m";
constexpr const char *Bold = "\033[1m";
constexpr const char *Dim = "\033[2m";

constexpr const char *Black = "\033[30m";
constexpr const char *Red = "\033[31m";
constexpr const char *Green = "\033[32m";
constexpr const char *Yellow = "\033[33m";
constexpr const char *Blue = "\033[34m";
constexpr const char *Magenta = "\033[35m";
constexpr const char *Cyan = "\033[36m";
constexpr const char *White = "\033[37m";

constexpr const char *BrightBlack = "\033[90m";
constexpr const char *BrightRed = "\033[91m";
constexpr const char *BrightGreen = "\033[92m";
constexpr const char *BrightYellow = "\033[93m";
constexpr const char *BrightBlue = "\033[94m";
constexpr const char *BrightMagenta = "\033[95m";
constexpr const char *BrightCyan = "\033[96m";
constexpr const char *BrightWhite = "\033[97m";

// HSV <-> RGB conversions. All values in [0, 1].

inline vec3 HsvToRgb(float h, float s, float v) {
	if (s < 1e-6f) {
		return { v, v, v };
	}
	h = std::fmod(h, 1.f) * 6.f;
	const int i = static_cast<int>(h);
	const float f = h - static_cast<float>(i);
	const float p = v * (1.f - s);
	const float q = v * (1.f - s * f);
	const float t = v * (1.f - s * (1.f - f));
	switch (i % 6) {
	case 0: return { v, t, p };
	case 1: return { q, v, p };
	case 2: return { p, v, t };
	case 3: return { p, q, v };
	case 4: return { t, p, v };
	default: return { v, p, q };
	}
}

inline vec3 RgbToHsv(float r, float g, float b) {
	const float mx = std::max({ r, g, b });
	const float mn = std::min({ r, g, b });
	const float d = mx - mn;
	float h = 0.f;
	const float s = (mx > 1e-6f) ? d / mx : 0.f;
	if (d > 1e-6f) {
		if (mx == r) {
			h = (g - b) / d / 6.f + (g < b ? 1.f : 0.f);
		} else if (mx == g) {
			h = (b - r) / d / 6.f + 1.f / 3.f;
		} else {
			h = (r - g) / d / 6.f + 2.f / 3.f;
		}
	}
	return { h, s, mx };
}

// Texture pixel data generators. All return RGBA8 data (w * h * 4 bytes).

inline std::vector<uint8> GenSV(float hue, uint32 w, uint32 h) {
	std::vector<uint8> px(w * h * 4);
	for (uint32 y = 0; y < h; ++y) {
		const float v = 1.f - y / static_cast<float>(h - 1);
		for (uint32 x = 0; x < w; ++x) {
			const float s = x / static_cast<float>(w - 1);
			const vec3 rgb = HsvToRgb(hue, s, v);
			const size_t i = (static_cast<size_t>(y) * w + x) * 4;
			px[i + 0] = uint8(rgb.r * 255.f);
			px[i + 1] = uint8(rgb.g * 255.f);
			px[i + 2] = uint8(rgb.b * 255.f);
			px[i + 3] = 255;
		}
	}
	return px;
}

inline std::vector<uint8> GenHue(uint32 w, uint32 h) {
	std::vector<uint8> px(w * h * 4);
	for (uint32 y = 0; y < h; ++y) {
		for (uint32 x = 0; x < w; ++x) {
			const vec3 rgb = HsvToRgb(x / static_cast<float>(w - 1), 1.f, 1.f);
			const size_t i = (static_cast<size_t>(y) * w + x) * 4;
			px[i + 0] = uint8(rgb.r * 255.f);
			px[i + 1] = uint8(rgb.g * 255.f);
			px[i + 2] = uint8(rgb.b * 255.f);
			px[i + 3] = 255;
		}
	}
	return px;
}

inline std::vector<uint8> GenChannelGrad(vec4 color, float h, float s, float v, int ch, bool isHSV, uint32 w, uint32 texH) {
	std::vector<uint8> px(w * texH * 4);
	for (uint32 x = 0; x < w; ++x) {
		const float t = x / static_cast<float>(w - 1);
		for (uint32 y = 0; y < texH; ++y) {
			const size_t i = (static_cast<size_t>(y) * w + x) * 4;
			if (ch == 3) {
				const float bg = ((x / 6) + (y / 6)) % 2 == 0 ? 0.75f : 0.5f;
				px[i + 0] = uint8((color.r * t + bg * (1.f - t)) * 255.f);
				px[i + 1] = uint8((color.g * t + bg * (1.f - t)) * 255.f);
				px[i + 2] = uint8((color.b * t + bg * (1.f - t)) * 255.f);
				px[i + 3] = 255;
			} else if (!isHSV) {
				float cr = color.r, cg = color.g, cb = color.b;
				switch (ch) {
				case 0: cr = t; break;
				case 1: cg = t; break;
				case 2: cb = t; break;
				}
				px[i + 0] = uint8(cr * 255.f);
				px[i + 1] = uint8(cg * 255.f);
				px[i + 2] = uint8(cb * 255.f);
				px[i + 3] = 255;
			} else {
				float ch_h = h, ch_s = s, ch_v = v;
				switch (ch) {
				case 0: ch_h = t; break;
				case 1: ch_s = t; break;
				case 2: ch_v = t; break;
				}
				const vec3 rgb = HsvToRgb(ch_h, ch_s, ch_v);
				px[i + 0] = uint8(rgb.r * 255.f);
				px[i + 1] = uint8(rgb.g * 255.f);
				px[i + 2] = uint8(rgb.b * 255.f);
				px[i + 3] = 255;
			}
		}
	}
	return px;
}

inline std::vector<uint8> GenAlpha(vec3 rgb, uint32 w, uint32 h) {
	std::vector<uint8> px(w * h * 4);
	for (uint32 y = 0; y < h; ++y) {
		for (uint32 x = 0; x < w; ++x) {
			const float a = x / static_cast<float>(w - 1);
			const float bg = ((x / 6) + (y / 6)) % 2 == 0 ? 0.75f : 0.5f;
			const size_t i = (static_cast<size_t>(y) * w + x) * 4;
			px[i + 0] = uint8((rgb.r * a + bg * (1.f - a)) * 255.f);
			px[i + 1] = uint8((rgb.g * a + bg * (1.f - a)) * 255.f);
			px[i + 2] = uint8((rgb.b * a + bg * (1.f - a)) * 255.f);
			px[i + 3] = 255;
		}
	}
	return px;
}

} // namespace Aquila::Foundation::Color

#endif
