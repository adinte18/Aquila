#include "Aquila/UI/Rendering/DrawList.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/UI/Rendering/DrawCmd.h"
#include "Aquila/UI/Text/FontAtlas.h"

namespace Aquila::UI::Rendering {

void DrawList::DrawRect(Rect rect, vec4 color, vec4 radius, f32 borderWidth, vec4 borderColor, int32 z) {
	DrawCmd command{};
	command.type = UICommandType::Rect;
	command.rect = rect;
	command.color = color;
	command.radius = radius;
	command.borderWidth = borderWidth;
	command.borderColor = borderColor;
	command.zOrder = z;

	m_Commands.push_back(command);
}

void DrawList::DrawShadow(Rect widgetRect, vec2 offset, float blur, float spread, vec4 color, vec4 radius, int32 z) {
	DrawCmd command{};
	command.type = UICommandType::Shadow;
	command.rect = widgetRect;
	command.color = color;
	command.radius = radius;
	command.borderWidth = blur;

	command.borderColor = { offset.x, offset.y, widgetRect.size.x * 0.5f + spread, widgetRect.size.y * 0.5f + spread };
	command.zOrder = z;
	m_Commands.push_back(command);
}

void DrawList::DrawText(Rect bounds, std::string_view text, Text::FontAtlas *font, vec4 color, float fontSize,
						TextAlign align, int32 z) {
	if ((font == nullptr) || text.empty()) {
		return;
	}

	DrawCmd command{};
	command.type = UICommandType::Text;
	command.rect = bounds;
	command.color = color;
	command.zOrder = z;
	command.text = std::string(text);
	command.font = font;
	command.fontSize = fontSize;

	command.borderWidth = static_cast<float>(static_cast<uint8>(align));

	m_Commands.push_back(command);
}

void DrawList::DrawImage(Rect rect, GFX::GfxTexture *tex, vec4 tint, vec2 uvMin, vec2 uvMax, int32 z) {
	DrawCmd command{};
	command.type = UICommandType::Image;
	command.rect = rect;
	command.texture = tex;
	command.zOrder = z;
	command.textureTint = tint;
	command.uvMin = uvMin;
	command.uvMax = uvMax;

	m_Commands.push_back(command);
}

void DrawList::PushClip(Rect clipRect) {
	m_ClipStack.push_back(clipRect);
	DrawCmd cmd{};
	cmd.type = UICommandType::ClipPush;
	cmd.rect = clipRect;
	m_Commands.push_back(cmd);
}

void DrawList::PopClip() {
	if (!m_ClipStack.empty()) {
		m_ClipStack.pop_back();
	}

	DrawCmd cmd{};
	cmd.type = UICommandType::ClipPop;
	cmd.rect = m_ClipStack.empty() ? Rect{} : m_ClipStack.back();
	m_Commands.push_back(cmd);
}

void DrawList::Sort() {
	std::ranges::stable_sort(m_Commands, {}, &DrawCmd::zOrder);
}

void DrawList::Submit(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd) {
	for (auto &command : m_Commands) {
		switch (command.type) {
		case UICommandType::Rect: {
			Graphics::RectSpec spec{};
			spec.position = command.rect.position;
			spec.size = command.rect.size;
			spec.color = command.color;
			spec.radius = command.radius;
			spec.borderWidth = command.borderWidth;
			spec.borderColor = command.borderColor;
			r2d.DrawRect(spec);
			break;
		}
		case UICommandType::Shadow: {
			Graphics::ShadowSpec spec{};
			const vec2 offset = { command.borderColor.x, command.borderColor.y };
			const vec2 sdfHalfSize = { command.borderColor.z, command.borderColor.w };
			const float blur = command.borderWidth;
			spec.position = command.rect.position + offset - vec2(blur + (sdfHalfSize.x - command.rect.size.x * 0.5f));
			spec.size = command.rect.size +
				vec2(2.f * (blur + (sdfHalfSize.x - command.rect.size.x * 0.5f)),
					 2.f * (blur + (sdfHalfSize.y - command.rect.size.y * 0.5f)));
			spec.color = command.color;
			spec.offset = offset;
			spec.originalHalfSize = sdfHalfSize;
			spec.radius = command.radius;
			spec.blur = blur;
			r2d.DrawShadow(spec);
			break;
		}
		case UICommandType::Image: {
			Graphics::SpriteSpec spec{};
			spec.position = command.rect.position;
			spec.size = command.rect.size;
			spec.tint = command.textureTint;
			spec.texture = command.texture;
			spec.uvMin = command.uvMin;
			spec.uvMax = command.uvMax;
			r2d.DrawSprite(spec);
			break;
		}
		case UICommandType::Text: {
			if ((command.font == nullptr) || command.text.empty()) {
				break;
			}

			Text::FontAtlas *atlas = command.font;
			const auto depth = 0.f;
			const auto align = static_cast<TextAlign>(static_cast<uint8>(command.borderWidth));

			const f32 bakeSize = command.font->GetBakeSize();
			const f32 renderSize = (command.fontSize > 0.f) ? command.fontSize : bakeSize;
			const f32 scale = (bakeSize > 0.f) ? (renderSize / bakeSize) : 1.f;

			struct CharEntry {
				const Text::GlyphInfo *glyph;
				const Text::SlugGlyphData *slug;
			};
			CharEntry glyphCache[512];
			uint32 cacheCount = 0;
			f32 textWidth = 0.f;

			const auto textLen = std::min(command.text.size(), static_cast<size_t>(512));
			for (size_t ci = 0; ci < textLen; ++ci) {
				const auto ch = static_cast<unsigned char>(command.text[ci]);
				const Text::GlyphInfo *g = command.font->GetGlyph(static_cast<uint32>(ch));
				if (!g) {
					continue;
				}
				textWidth += g->advance * scale;
				glyphCache[cacheCount++] = { g, atlas->GetSlugData(g->glyphID) };
			}

			f32 cursorX = command.rect.position.x;
			if (align == TextAlign::Center) {
				cursorX += (command.rect.size.x - textWidth) * 0.5f;
			} else if (align == TextAlign::Right) {
				cursorX += command.rect.size.x - textWidth;
			} else if (cacheCount > 0) {
				cursorX -= glyphCache[0].glyph->bearing.x * scale;
			}
			const f32 baselineY = command.rect.position.y + command.font->GetAscent() * scale;

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
				spec.color = command.color;
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
			break;
		}
		case UICommandType::ClipPush: {
			r2d.Flush();
			cmd.SetScissor(static_cast<int32>(command.rect.Left()), static_cast<int32>(command.rect.Top()),
						   static_cast<uint32>(command.rect.Width()), static_cast<uint32>(command.rect.Height()));
			break;
		}
		case UICommandType::ClipPop: {
			r2d.Flush();
			if (command.rect.IsEmpty()) {
				cmd.SetScissor(0, 0, m_CanvasWidth, m_CanvasHeight);
			} else {
				cmd.SetScissor(static_cast<int32>(command.rect.Left()), static_cast<int32>(command.rect.Top()),
							   static_cast<uint32>(command.rect.Width()), static_cast<uint32>(command.rect.Height()));
			}
			break;
		}
		default:
			break;
		}
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
