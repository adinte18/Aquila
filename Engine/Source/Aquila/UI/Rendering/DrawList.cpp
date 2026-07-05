#include "Aquila/UI/Rendering/DrawList.h"
#include "Aquila/Foundation/Text/Utf8.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/UI/Rendering/DrawCmd.h"
#include "Aquila/UI/Text/FontAtlas.h"

#include <type_traits>

namespace Aquila::UI::Rendering {

void DrawList::DrawRect(Rect rect, vec4 color, vec4 radius, f32 borderWidth, vec4 borderColor, int32 z) {
	RectCmd command;
	command.rect = rect;
	command.color = color;
	command.radius = radius;
	command.borderWidth = borderWidth;
	command.borderColor = borderColor;
	command.zOrder = z;

	m_Commands.push_back(command);
}

void DrawList::DrawLine(vec2 from, vec2 to, float width, vec4 color, int32 z) {
	vec2 delta = to - from;
	float length = glm::length(delta);
	if (length < 0.5f) {
		return;
	}

	vec2 center = (from + to) * 0.5f;
	float angle = std::atan2(delta.y, delta.x);

	RectCmd command;
	command.rect = { center - vec2(length * 0.5f, width * 0.5f), { length, width } };
	command.color = color;
	command.rotation = angle;
	command.zOrder = z;

	m_Commands.push_back(command);
}

void DrawList::DrawShadow(Rect widgetRect, vec2 offset, float blur, float spread, vec4 color, vec4 radius, int32 z) {
	ShadowCmd command;
	command.rect = widgetRect;
	command.color = color;
	command.radius = radius;
	command.offset = offset;
	command.originalHalfSize = { widgetRect.size.x * 0.5f + spread, widgetRect.size.y * 0.5f + spread };
	command.blur = blur;
	command.zOrder = z;

	m_Commands.push_back(command);
}

void DrawList::DrawText(Rect bounds, std::string_view text, Text::FontAtlas *font, vec4 color, float fontSize,
						TextAlign align, int32 z) {
	if ((font == nullptr) || text.empty()) {
		return;
	}

	font->EnsureGlyphs(text);

	TextCmd command;
	command.rect = bounds;
	command.color = color;
	command.zOrder = z;
	command.text = std::string(text);
	command.font = font;
	command.fontSize = fontSize;
	command.align = align;

	m_Commands.push_back(std::move(command));
}

void DrawList::DrawImage(Rect rect, GFX::GfxTexture *tex, vec4 tint, vec2 uvMin, vec2 uvMax, int32 z) {
	ImageCmd command;
	command.rect = rect;
	command.texture = tex;
	command.zOrder = z;
	command.tint = tint;
	command.uvMin = uvMin;
	command.uvMax = uvMax;

	m_Commands.push_back(command);
}

void DrawList::PushClip(Rect clipRect) {
	m_ClipStack.push_back(clipRect);
	ClipPushCmd cmd;
	cmd.rect = clipRect;
	m_Commands.push_back(cmd);
}

void DrawList::PopClip() {
	if (!m_ClipStack.empty()) {
		m_ClipStack.pop_back();
	}

	ClipPopCmd cmd;
	cmd.rect = m_ClipStack.empty() ? Rect{} : m_ClipStack.back();
	m_Commands.push_back(cmd);
}

void DrawList::Sort() {
	std::ranges::stable_sort(m_Commands, {}, [](const DrawCmd &c) { return DrawCmdZOrder(c); });
}

void DrawList::Submit(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd) {
	for (auto &command : m_Commands) {
		std::visit(
			[&](auto &c) {
				using T = std::decay_t<decltype(c)>;

				if constexpr (std::is_same_v<T, RectCmd>) {
					Graphics::RectSpec spec{};
					spec.position = c.rect.position;
					spec.size = c.rect.size;
					spec.color = c.color;
					spec.radius = c.radius;
					spec.borderWidth = c.borderWidth;
					spec.borderColor = c.borderColor;
					spec.rotation = c.rotation;
					r2d.DrawRect(spec);
				} else if constexpr (std::is_same_v<T, ShadowCmd>) {
					Graphics::ShadowSpec spec{};
					const vec2 offset = c.offset;
					const vec2 sdfHalfSize = c.originalHalfSize;
					const float blur = c.blur;
					spec.position = c.rect.position + offset - vec2(blur + (sdfHalfSize.x - c.rect.size.x * 0.5f));
					spec.size = c.rect.size +
						vec2(2.f * (blur + (sdfHalfSize.x - c.rect.size.x * 0.5f)),
							 2.f * (blur + (sdfHalfSize.y - c.rect.size.y * 0.5f)));
					spec.color = c.color;
					spec.offset = offset;
					spec.originalHalfSize = sdfHalfSize;
					spec.radius = c.radius;
					spec.blur = blur;
					r2d.DrawShadow(spec);
				} else if constexpr (std::is_same_v<T, ImageCmd>) {
					Graphics::SpriteSpec spec{};
					spec.position = c.rect.position;
					spec.size = c.rect.size;
					spec.tint = c.tint;
					spec.texture = c.texture;
					spec.uvMin = c.uvMin;
					spec.uvMax = c.uvMax;
					r2d.DrawSprite(spec);
				} else if constexpr (std::is_same_v<T, TextCmd>) {
					if ((c.font == nullptr) || c.text.empty()) {
						return;
					}

					Text::FontAtlas *atlas = c.font;
					const auto depth = 0.f;
					const auto align = c.align;

					const f32 bakeSize = c.font->GetBakeSize();
					const f32 renderSize = (c.fontSize > 0.f) ? c.fontSize : bakeSize;
					const f32 scale = (bakeSize > 0.f) ? (renderSize / bakeSize) : 1.f;

					struct CharEntry {
						const Text::GlyphInfo *glyph;
						const Text::SlugGlyphData *slug;
					};
					CharEntry glyphCache[512];
					uint32 cacheCount = 0;
					f32 textWidth = 0.f;

					const auto textLen = std::min(c.text.size(), static_cast<size_t>(512));
					for (size_t ci = 0; ci < textLen && cacheCount < 512;) {
						const Foundation::Utf8::Decoded d = Foundation::Utf8::Decode(c.text, ci);
						ci += (d.size > 0 ? d.size : 1u);
						const Text::GlyphInfo *g = c.font->GetGlyph(d.codepoint);
						if (!g) {
							continue;
						}
						textWidth += g->advance * scale;
						glyphCache[cacheCount++] = { g, atlas->GetSlugData(g->glyphID) };
					}

					f32 cursorX = c.rect.position.x;
					if (align == TextAlign::Center) {
						cursorX += (c.rect.size.x - textWidth) * 0.5f;
					} else if (align == TextAlign::Right) {
						cursorX += c.rect.size.x - textWidth;
					} else if (cacheCount > 0) {
						cursorX -= glyphCache[0].glyph->bearing.x * scale;
					}
					const f32 baselineY = c.rect.position.y + c.font->GetAscent() * scale;

					GFX::GfxTexture *curveTexture = atlas->GetCurveTexture();
					GFX::GfxTexture *bandTexture = atlas->GetBandTexture();

					for (uint32 ci = 0; ci < cacheCount; ++ci) {
						const Text::GlyphInfo *glyph = glyphCache[ci].glyph;
						const Text::SlugGlyphData *slug = glyphCache[ci].slug;

						if (slug == nullptr) {
							cursorX += glyph->advance * scale;
							continue;
						}

						const f32 glyphX = cursorX + glyph->bearing.x * scale;
						const f32 glyphY = baselineY + glyph->bearing.y * scale;

						Graphics::GlyphSpec spec{};
						spec.position = { glyphX, glyphY };
						spec.size = glyph->size * scale;
						spec.color = c.color;
						spec.depth = depth;
						spec.glyphLocX = slug->glyphLocX;
						spec.glyphLocY = slug->glyphLocY;
						spec.bandMaxX = slug->bandMaxX;
						spec.bandMaxY = slug->bandMaxY;
						spec.banding = slug->bandTransform;
						spec.emMin = slug->emMin;
						spec.emMax = slug->emMax;
						spec.curveTexture = curveTexture;
						spec.bandTexture = bandTexture;

						r2d.DrawGlyph(spec);
						cursorX += glyph->advance * scale;
					}
				} else if constexpr (std::is_same_v<T, ClipPushCmd>) {
					r2d.Flush();
					r2d.SetScissor(cmd, static_cast<int32>(c.rect.Left()), static_cast<int32>(c.rect.Top()),
								   static_cast<uint32>(c.rect.Width()), static_cast<uint32>(c.rect.Height()));
				} else if constexpr (std::is_same_v<T, ClipPopCmd>) {
					r2d.Flush();
					if (c.rect.IsEmpty()) {
						r2d.SetScissor(cmd, 0, 0, m_CanvasWidth, m_CanvasHeight);
					} else {
						r2d.SetScissor(cmd, static_cast<int32>(c.rect.Left()), static_cast<int32>(c.rect.Top()),
									   static_cast<uint32>(c.rect.Width()), static_cast<uint32>(c.rect.Height()));
					}
				}
			},
			command);
	}
}

void DrawList::AppendCmd(const DrawCmd &cmd) {
	m_Commands.push_back(cmd);
}

std::vector<DrawCmd> DrawList::TakeCommands() {
	m_ClipStack.clear();
	return std::move(m_Commands);
}

void DrawList::Clear() {
	m_ClipStack.clear();
	m_Commands.clear();
}

bool DrawList::IsEmpty() const {
	return m_Commands.empty();
}

void DrawList::SetCanvasSize(uint32 width, uint32 height) {
	m_CanvasWidth = width;
	m_CanvasHeight = height;
}

Option<Rect> DrawList::ActiveClip() const {
	if (m_ClipStack.empty()) {
		return std::nullopt;
	}
	return m_ClipStack.back();
}
} // namespace Aquila::UI::Rendering
