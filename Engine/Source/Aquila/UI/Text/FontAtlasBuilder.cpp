#include "Aquila/UI/Text/FontAtlasBuilder.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Foundation/Math/Math.h"

#include <algorithm>

namespace Aquila::UI::Text::Internal {

std::vector<std::array<f32, 4>> BuildCurveTextureData(const std::vector<GlyphBuild> &builds,
													  const std::vector<uint32> &glyphCurveStart,
													  uint32 curveTexelCount, uint32 count) {
	const uint32 texH = (curveTexelCount + SharedConstants::FONT_TEX_WIDTH - 1) / SharedConstants::FONT_TEX_WIDTH;
	std::vector<std::array<f32, 4>> data(texH * SharedConstants::FONT_TEX_WIDTH, { 0.f, 0.f, 0.f, 0.f });

	for (uint32 i = 0; i < count; ++i) {
		const uint32 base = glyphCurveStart[i];
		const auto &curves = builds[i].curves;

		for (uint32 c = 0; c < static_cast<uint32>(curves.size()); ++c) {
			const auto &cv = curves[c];
			data[base + c * 2] = { cv.p0.x, cv.p0.y, cv.p1.x, cv.p1.y };
			data[base + c * 2 + 1] = { cv.p2.x, cv.p2.y, 0.f, 0.f };
		}
	}
	return data;
}

GlyphBandData BucketCurvesIntoBands(const GlyphBuild &build, uint32 curveBase, f32 scaleX, f32 scaleY, f32 offsetX,
									f32 offsetY) {
	GlyphBandData result{};

	for (uint32 c = 0; c < static_cast<uint32>(build.curves.size()); ++c) {
		const auto &cv = build.curves[c];
		const uint32 absTexel = curveBase + c * SharedConstants::FONT_TEXELS_PER_CURVE;
		const uint32 tx = absTexel % SharedConstants::FONT_TEX_WIDTH;
		const uint32 ty = absTexel / SharedConstants::FONT_TEX_WIDTH;

		const auto bounds = Math::Bezier::ComputeBounds(cv);
		const f32 xMin = bounds.min.x, xMax = bounds.max.x;
		const f32 yMin = bounds.min.y, yMax = bounds.max.y;

		const int byMin = Math::Max(0, static_cast<int>(Math::Floor(yMin * scaleY + offsetY)));
		const int byMax = Math::Min(static_cast<int>(SharedConstants::FONT_BAND_MAX),
									static_cast<int>(Math::Floor(yMax * scaleY + offsetY)));
		for (int b = byMin; b <= byMax; ++b) {
			result.horizontal[static_cast<uint32>(b)].push_back({ tx, ty, xMax });
		}

		const int bxMin = Math::Max(0, static_cast<int>(Math::Floor(xMin * scaleX + offsetX)));
		const int bxMax = Math::Min(static_cast<int>(SharedConstants::FONT_BAND_MAX),
									static_cast<int>(Math::Floor(xMax * scaleX + offsetX)));
		for (int b = bxMin; b <= bxMax; ++b) {
			result.vertical[static_cast<uint32>(b)].push_back({ tx, ty, yMax });
		}
	}

	auto descSort = [](const BandEntry &a, const BandEntry &b) { return a.sortKey > b.sortKey; };
	for (auto &hb : result.horizontal) {
		std::sort(hb.begin(), hb.end(), descSort);
	}
	for (auto &vb : result.vertical) {
		std::sort(vb.begin(), vb.end(), descSort);
	}

	return result;
}

void WriteGlyphBandEntries(std::vector<std::array<uint32, 4>> &bandTexData, uint32 bandStart,
						   const GlyphBandData &bands) {
	auto ensureSize = [&](uint32 needed) {
		if (bandTexData.size() < needed) {
			bandTexData.resize(needed, { 0u, 0u, 0u, 0u });
		}
	};

	uint32 totalHIdx = 0;
	for (const auto &hb : bands.horizontal) {
		totalHIdx += static_cast<uint32>(hb.size());
	}
	uint32 totalVIdx = 0;
	for (const auto &vb : bands.vertical) {
		totalVIdx += static_cast<uint32>(vb.size());
	}

	const uint32 headerSlots = 2 * SharedConstants::FONT_BAND_COUNT;
	ensureSize(bandStart + headerSlots + totalHIdx + totalVIdx);

	const uint32 hIdxBase = bandStart + headerSlots;
	uint32 hIdxCursor = hIdxBase;
	for (uint32 b = 0; b < SharedConstants::FONT_BAND_COUNT; ++b) {
		const uint32 count = static_cast<uint32>(bands.horizontal[b].size());
		const uint32 offset = (count > 0) ? (hIdxCursor - bandStart) : 0u;
		bandTexData[bandStart + b] = { count, offset, 0u, 0u };
		hIdxCursor += count;
	}

	const uint32 vIdxBase = hIdxBase + totalHIdx;
	uint32 vIdxCursor = vIdxBase;
	for (uint32 b = 0; b < SharedConstants::FONT_BAND_COUNT; ++b) {
		const uint32 count = static_cast<uint32>(bands.vertical[b].size());
		const uint32 offset = (count > 0) ? (vIdxCursor - bandStart) : 0u;
		bandTexData[bandStart + SharedConstants::FONT_BAND_COUNT + b] = { count, offset, 0u, 0u };
		vIdxCursor += count;
	}

	uint32 absIdx = hIdxBase;
	for (uint32 b = 0; b < SharedConstants::FONT_BAND_COUNT; ++b) {
		for (const auto &e : bands.horizontal[b]) {
			bandTexData[absIdx++] = { e.texelX, e.texelY, 0u, 0u };
		}
	}

	absIdx = vIdxBase;
	for (uint32 b = 0; b < SharedConstants::FONT_BAND_COUNT; ++b) {
		for (const auto &e : bands.vertical[b]) {
			bandTexData[absIdx++] = { e.texelX, e.texelY, 0u, 0u };
		}
	}
}

} // namespace Aquila::UI::Text::Internal
