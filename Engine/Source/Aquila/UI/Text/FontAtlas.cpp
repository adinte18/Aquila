#include "Aquila/UI/Text/FontAtlas.h"
#include "Aquila/UI/Text/FontAtlasBuilder.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

namespace Aquila::UI::Text {

using namespace Internal;

void FontAtlas::BuildGlyphCurves(const stbtt_fontinfo &fontInfo, int glyphIndex, f32 scale, GlyphBuild &out) {
	int bx0 = 0;
	int by0 = 0;
	int bx1 = 0;
	int by1 = 0;
	stbtt_GetGlyphBox(&fontInfo, glyphIndex, &bx0, &by0, &bx1, &by1);

	out.emMin = { 0.f, 0.f };
	out.emMax = { static_cast<f32>(bx1 - bx0) * scale, static_cast<f32>(by1 - by0) * scale };

	auto toLocal = [&](int x, int y) -> vec2 {
		return { (static_cast<f32>(x) - static_cast<f32>(bx0)) * scale,
				 (static_cast<f32>(y) - static_cast<f32>(by0)) * scale };
	};

	auto addCurve = [&](vec2 p0, vec2 p1, vec2 p2) {
		Math::Bezier::QuadraticBezier curve{ .p0 = p0, .p1 = p1, .p2 = p2 };
		auto split = Math::Bezier::SplitAtYExtrema(curve);
		if (split.wasSplit) {
			out.curves.push_back(split.left);
			out.curves.push_back(split.right);
		} else {
			out.curves.push_back(curve);
		}
	};

	auto addLine = [&](vec2 p0, vec2 p2) { addCurve(p0, (p0 + p2) * 0.5f, p2); };

	stbtt_vertex *verts = nullptr;
	const int numVerts = stbtt_GetGlyphShape(&fontInfo, glyphIndex, &verts);

	vec2 cursor{};
	for (int vert = 0; vert < numVerts; ++vert) {
		switch (verts[vert].type) {
		case STBTT_vmove:
			cursor = toLocal(verts[vert].x, verts[vert].y);
			break;

		case STBTT_vline: {
			vec2 end = toLocal(verts[vert].x, verts[vert].y);
			addLine(cursor, end);
			cursor = end;
			break;
		}

		case STBTT_vcurve: {
			vec2 ctrl = toLocal(verts[vert].cx, verts[vert].cy);
			vec2 end = toLocal(verts[vert].x, verts[vert].y);
			addCurve(cursor, ctrl, end);
			cursor = end;
			break;
		}

		case STBTT_vcubic: {
			vec2 c1 = toLocal(verts[vert].cx, verts[vert].cy);
			vec2 c2 = toLocal(verts[vert].cx1, verts[vert].cy1);
			vec2 end = toLocal(verts[vert].x, verts[vert].y);

			vec2 mid = (cursor + 3.0f * c1 + 3.0f * c2 + end) * 0.125f;
			addCurve(cursor, (cursor + c1) * 0.5f, mid);
			addCurve(mid, (c2 + end) * 0.5f, end);
			cursor = end;
			break;
		}
		}
	}

	if (verts != nullptr) {
		stbtt_FreeShape(&fontInfo, verts);
	}
}

GlyphInfo FontAtlas::BuildGlyphInfo(const stbtt_fontinfo &fontInfo, int glyphIndex, uint32 glyphID, f32 scale) {
	int advanceWidth = 0, leftSideBearing = 0;
	stbtt_GetGlyphHMetrics(&fontInfo, glyphIndex, &advanceWidth, &leftSideBearing);

	int bx0 = 0, by0 = 0, bx1 = 0, by1 = 0;
	stbtt_GetGlyphBox(&fontInfo, glyphIndex, &bx0, &by0, &bx1, &by1);

	GlyphInfo info{};
	info.glyphID = glyphID;
	info.size = { static_cast<f32>(bx1 - bx0) * scale, static_cast<f32>(by1 - by0) * scale };
	info.bearing = { static_cast<f32>(bx0) * scale, -static_cast<f32>(by1) * scale };
	info.advance = static_cast<f32>(advanceWidth) * scale;
	return info;
}

Unique<FontAtlas> FontAtlas::Create(GFX::GfxContext &ctx, const uint8 *ttfData, uint64 dataSize, f32 pixelHeight) {
	AQUILA_UNUSED(dataSize);

	Unique<FontAtlas> atlas(new FontAtlas());
	atlas->m_Ctx = &ctx;
	atlas->m_BakeSize = pixelHeight;

	stbtt_fontinfo fontInfo{};
	stbtt_InitFont(&fontInfo, ttfData, stbtt_GetFontOffsetForIndex(ttfData, 0));

	const f32 scale = stbtt_ScaleForPixelHeight(&fontInfo, pixelHeight);

	int ascent = 0, descent = 0, lineGap = 0;
	stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
	atlas->m_Ascent = static_cast<f32>(ascent) * scale;
	atlas->m_Descent = static_cast<f32>(descent) * scale;
	atlas->m_LineHeight = static_cast<f32>(ascent - descent + lineGap) * scale;

	std::vector<GlyphBuild> builds(SharedConstants::FONT_GLYPH_COUNT);
	for (int i = 0; i < SharedConstants::FONT_GLYPH_COUNT; ++i) {
		const auto codepoint = static_cast<uint32>(SharedConstants::FONT_FIRST_CODEPOINT + i);
		const int glyphIndex = stbtt_FindGlyphIndex(&fontInfo, static_cast<int>(codepoint));
		BuildGlyphCurves(fontInfo, glyphIndex, scale, builds[i]);
	}

	std::vector<uint32> glyphCurveStart(SharedConstants::FONT_GLYPH_COUNT, 0);
	uint32 curveCursor = 0;
	for (int i = 0; i < SharedConstants::FONT_GLYPH_COUNT; ++i) {
		glyphCurveStart[i] = curveCursor;
		curveCursor += static_cast<uint32>(builds[i].curves.size()) * SharedConstants::FONT_TEXELS_PER_CURVE;
	}

	const uint32 curveTexelCount = std::max(curveCursor, 1u);
	auto curveTexData = BuildCurveTextureData(builds, glyphCurveStart, curveTexelCount,
											  static_cast<uint32>(SharedConstants::FONT_GLYPH_COUNT));

	atlas->m_SlugGlyphs.resize(SharedConstants::FONT_GLYPH_COUNT);
	std::vector<std::array<uint32, 4>> bandTexData;
	uint32 bandCursor = 0;

	for (int i = 0; i < SharedConstants::FONT_GLYPH_COUNT; ++i) {
		const auto &build = builds[i];
		const uint32 curveBase = glyphCurveStart[i];

		const f32 emW = build.emMax.x - build.emMin.x;
		const f32 emH = build.emMax.y - build.emMin.y;
		const f32 scaleX = (emW > 0.01f) ? (static_cast<f32>(SharedConstants::FONT_BAND_COUNT) / emW) : 0.f;
		const f32 scaleY = (emH > 0.01f) ? (static_cast<f32>(SharedConstants::FONT_BAND_COUNT) / emH) : 0.f;
		const f32 offsetX = -build.emMin.x * scaleX;
		const f32 offsetY = -build.emMin.y * scaleY;

		{
			const uint32 x = bandCursor % SharedConstants::FONT_TEX_WIDTH;
			if (x + (2 * SharedConstants::FONT_BAND_COUNT) > SharedConstants::FONT_TEX_WIDTH) {
				bandCursor += SharedConstants::FONT_TEX_WIDTH - x;
			}
		}

		const uint32 bandStart = bandCursor;
		const uint32 glyphLocX = bandStart % SharedConstants::FONT_TEX_WIDTH;
		const uint32 glyphLocY = bandStart / SharedConstants::FONT_TEX_WIDTH;

		const auto bands = BucketCurvesIntoBands(build, curveBase, scaleX, scaleY, offsetX, offsetY);
		WriteGlyphBandEntries(bandTexData, bandStart, bands);

		uint32 totalHIdx = 0, totalVIdx = 0;
		for (const auto &hb : bands.horizontal) {
			totalHIdx += static_cast<uint32>(hb.size());
		}
		for (const auto &vb : bands.vertical) {
			totalVIdx += static_cast<uint32>(vb.size());
		}
		bandCursor = bandStart + 2 * SharedConstants::FONT_BAND_COUNT + totalHIdx + totalVIdx;

		atlas->m_SlugGlyphs[i] = SlugGlyphData{
			.glyphLocX = glyphLocX,
			.glyphLocY = glyphLocY,
			.bandMaxX = SharedConstants::FONT_BAND_MAX,
			.bandMaxY = SharedConstants::FONT_BAND_MAX,
			.bandTransform = { scaleX, scaleY, offsetX, offsetY },
			.emMin = build.emMin,
			.emMax = build.emMax,
		};
	}

	for (int i = 0; i < SharedConstants::FONT_GLYPH_COUNT; ++i) {
		const auto codepoint = static_cast<uint32>(SharedConstants::FONT_FIRST_CODEPOINT + i);
		const int glyphIndex = stbtt_FindGlyphIndex(&fontInfo, static_cast<int>(codepoint));
		atlas->m_Glyphs[codepoint] = BuildGlyphInfo(fontInfo, glyphIndex, static_cast<uint32>(i), scale);
	}

	const uint32 curveTexH = (curveTexelCount + SharedConstants::FONT_TEX_WIDTH - 1) / SharedConstants::FONT_TEX_WIDTH;

	atlas->m_CurveTexture = ctx.CreateTexture({
		.width = SharedConstants::FONT_TEX_WIDTH,
		.height = curveTexH,
		.format = RHI::TextureFormat::RGBA32F,
		.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst,
		.debugName = "FontAtlas_Curves",
	});
	ctx.UploadTextureData(*atlas->m_CurveTexture, curveTexData.data(), sizeof(f32) * 4 * curveTexData.size());

	if (bandTexData.empty()) {
		bandTexData.push_back({ 0u, 0u, 0u, 0u });
	}

	const uint32 bandTexelCount = static_cast<uint32>(bandTexData.size());
	const uint32 bandTexH = (bandTexelCount + SharedConstants::FONT_TEX_WIDTH - 1) / SharedConstants::FONT_TEX_WIDTH;

	bandTexData.resize(bandTexH * SharedConstants::FONT_TEX_WIDTH, { 0u, 0u, 0u, 0u });

	atlas->m_BandTexture = ctx.CreateTexture({
		.width = SharedConstants::FONT_TEX_WIDTH,
		.height = bandTexH,
		.format = RHI::TextureFormat::RGBA32U,
		.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst,
		.sampler = RHI::SamplerDesc::PointSample(),
		.debugName = "FontAtlas_Bands",
	});
	ctx.UploadTextureData(*atlas->m_BandTexture, bandTexData.data(), sizeof(uint32) * 4 * bandTexData.size());

	return atlas;
}

Unique<FontAtlas> FontAtlas::CreateFromFile(GFX::GfxContext &ctx, const std::string &path, f32 pixelHeight) {
	auto file = Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(path, AccessMode::Read, OpenMode::Binary);
	if (!file || !file->IsValid()) {
		AQUILA_LOG_ERROR("FontAtlas: cannot open '{}'", path);
		return nullptr;
	}

	const int64 size = file->Size();
	if (size <= 0) {
		return nullptr;
	}
	std::vector<uint8> data(static_cast<usize>(size));
	file->Read(data.data(), static_cast<usize>(size));
	return Create(ctx, data.data(), static_cast<uint64>(size), pixelHeight);
}

const GlyphInfo *FontAtlas::GetGlyph(uint32 codepoint) const {
	auto it = m_Glyphs.find(codepoint);
	return it != m_Glyphs.end() ? &it->second : nullptr;
}

const SlugGlyphData *FontAtlas::GetSlugData(uint32 glyphID) const {
	if (glyphID < static_cast<uint32>(m_SlugGlyphs.size())) {
		return &m_SlugGlyphs[glyphID];
	}
	return nullptr;
}

} // namespace Aquila::UI::Text
