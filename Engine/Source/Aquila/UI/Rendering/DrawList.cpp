#include "Aquila/UI/Rendering/DrawList.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/UI/Rendering/DrawCmd.h"

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
void DrawList::DrawText(Rect bounds, std::string_view text, Text::FontAtlas *font, vec4 color, int32 z) {
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

	m_Commands.push_back(command);
}

void DrawList::DrawImage(Rect rect, GFX::GfxTexture *tex, vec4 tint, int32 z) {
	DrawCmd command{};
	command.type = UICommandType::Image;
	command.rect = rect;
	command.texture = tex;
	command.zOrder = z;
	command.textureTint = tint;

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
			spec.depth = static_cast<f32>(command.zOrder);
			r2d.DrawRect(spec);
			break;
		}
		case UICommandType::Image: {
			Graphics::SpriteSpec spec{};
			spec.position = command.rect.position;
			spec.size = command.rect.size;
			spec.tint = command.textureTint;
			spec.texture = command.texture;
			spec.depth = static_cast<f32>(command.zOrder);
			r2d.DrawSprite(spec);
			break;
		}
		case UICommandType::Text: {
			if ((command.font == nullptr) || command.text.empty()) {
				break;
			}

			GFX::GfxTexture *atlas = command.font->GetTexture();
			const auto depth = static_cast<f32>(command.zOrder);
			f32 cursorX = command.rect.position.x;
			const f32 baselineY = command.rect.position.y + command.font->GetAscent();

			for (unsigned char ch : command.text) {
				const Text::GlyphInfo *glyph = command.font->GetGlyph(static_cast<uint32>(ch));
				if (glyph == nullptr) {
					continue;
				}

				const f32 glyphX = cursorX + glyph->bearing.x;
				const f32 glyphY = baselineY + glyph->bearing.y;

				Graphics::GlyphSpec spec{};
				spec.position = { glyphX, glyphY };
				spec.size = glyph->size;
				spec.color = command.color;
				spec.depth = depth;
				spec.atlasTexture = atlas;
				spec.uvMin = glyph->uvMin;
				spec.uvMax = glyph->uvMax;

				r2d.DrawGlyph(spec);
				cursorX += glyph->advance;
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
