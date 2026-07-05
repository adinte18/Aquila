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
	Vec2 em_min{ 0.F, 0.F };
	Vec2 em_max{ 0.F, 0.F };
};

} // namespace Aquila::UI::Text

namespace Aquila::UI::Text::Internal {

struct BandEntry {
	Uint32 texel_x;
	Uint32 texel_y;
	F32 sort_key;
};

struct GlyphBandData {
	std::array<std::vector<BandEntry>, SharedConstants::FONT_BAND_COUNT> horizontal;
	std::array<std::vector<BandEntry>, SharedConstants::FONT_BAND_COUNT> vertical;
};

// Packs curve p0/p1/p2 data into a flat RGBA32F texel array.
// count = number of builds to process (may be less than builds.size()).
std::vector<std::array<F32, 4>> build_curve_texture_data(const std::vector<GlyphBuild> &builds,
														 const std::vector<Uint32> &glyph_curve_start,
														 Uint32 curve_texel_count, Uint32 count);

// Buckets the curves of a single glyph/icon into horizontal and vertical bands.
GlyphBandData bucket_curves_into_bands(const GlyphBuild &build, Uint32 curve_base, F32 scale_x, F32 scale_y,
									   F32 offset_x, F32 offset_y);

// Writes one glyph/icon's band data (headers + index lists) into the flat RGBA32U texel vector.
void write_glyph_band_entries(std::vector<std::array<Uint32, 4>> &band_tex_data, Uint32 band_start,
							  const GlyphBandData &bands);

} // namespace Aquila::UI::Text::Internal
