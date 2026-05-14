#pragma once

#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Foundation/Math/Geometry/Bezier.h"

#include <array>
#include <vector>

namespace Aquila::UI::Text {

// GlyphBuild accumulates the quadratic bezier curves for one glyph/icon during baking.
struct GlyphBuild {
	std::vector<Math::Bezier::QuadraticBezier> curves;
	vec2 emMin{ 0.f, 0.f };
	vec2 emMax{ 0.f, 0.f };
};

} // namespace Aquila::UI::Text

namespace Aquila::UI::Text::Internal {

struct BandEntry {
	uint32 texelX;
	uint32 texelY;
	f32 sortKey;
};

struct GlyphBandData {
	std::array<std::vector<BandEntry>, SharedConstants::FONT_BAND_COUNT> horizontal;
	std::array<std::vector<BandEntry>, SharedConstants::FONT_BAND_COUNT> vertical;
};

// Packs curve p0/p1/p2 data into a flat RGBA32F texel array.
// count = number of builds to process (may be less than builds.size()).
std::vector<std::array<f32, 4>> BuildCurveTextureData(const std::vector<GlyphBuild> &builds,
													   const std::vector<uint32> &glyphCurveStart,
													   uint32 curveTexelCount, uint32 count);

// Buckets the curves of a single glyph/icon into horizontal and vertical bands.
GlyphBandData BucketCurvesIntoBands(const GlyphBuild &build, uint32 curveBase, f32 scaleX, f32 scaleY,
									 f32 offsetX, f32 offsetY);

// Writes one glyph/icon's band data (headers + index lists) into the flat RGBA32U texel vector.
void WriteGlyphBandEntries(std::vector<std::array<uint32, 4>> &bandTexData, uint32 bandStart,
						   const GlyphBandData &bands);

} // namespace Aquila::UI::Text::Internal
