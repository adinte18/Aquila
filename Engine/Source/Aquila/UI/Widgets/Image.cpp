#include "Aquila/UI/Widgets/Image.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/UI/Style/StyleParserHelper.h"

namespace Aquila::UI::Core {

Image::Image(GFX::GfxTexture *texture, vec4 tint) : m_Texture(texture), m_Tint(tint) {}

void Image::SetTexture(GFX::GfxTexture *texture) {
	if (texture == m_Texture) {
		return;
	}
	m_Texture = texture;
	InvalidateLayout();
}

void Image::SetTint(vec4 tint) {
	if (tint == m_Tint) {
		return;
	}
	m_Tint = tint;
	QueueRedraw();
}

void Image::SetUVRegion(vec2 uvMin, vec2 uvMax) {
	if (uvMin == m_UVMin && uvMax == m_UVMax) {
		return;
	}
	m_UVMin = uvMin;
	m_UVMax = uvMax;
	InvalidateLayout();
}

void Image::OnDrawSelf(Rendering::DrawList &drawList) {
	View::OnDrawSelf(drawList);

	if (m_Texture == nullptr) {
		return;
	}

	const Rect worldRect = GetAbsoluteRect();
	const vec4 tint = m_Tint * GetDisplayStyle().color;
	drawList.DrawImage(worldRect, m_Texture, tint, m_UVMin, m_UVMax, GetStackingZ() * 4 + 2);
}

void Image::ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx) {
	if (name == "tint") {
		if (auto c = UI::ParserHelper::ParseColor(value)) {
			SetTint(*c);
		}
		return;
	}
	if (name == "src") {
		auto *loader = static_cast<Core::LayoutLoader *>(loaderCtx);
		if (loader) {
			if (GFX::GfxTexture *tex = loader->ResolveTexture(std::string(value))) {
				SetTexture(tex);
			} else {
				AQUILA_LOG_WARNING("Image: could not load texture '{}'", value);
			}
		}
		return;
	}
	// icon/bank/uv: the LayoutLoader handles these together (they require cross-attribute
	// coordination) — ignore them here so we don't double-apply them.
	if (name == "icon" || name == "bank" || name == "uv") {
		return;
	}
	View::ApplyXmlAttribute(name, value, loaderCtx);
}

} // namespace Aquila::UI::Core
