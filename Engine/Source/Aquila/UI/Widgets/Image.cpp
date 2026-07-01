#include "Aquila/UI/Widgets/Image.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/UI/Style/StyleParserHelper.h"
#include <sstream>

namespace Aquila::UI::Core {

Image::Image() {
	m_ShouldSkipHitTest = true;
}

Image::Image(GFX::GfxTexture *texture, vec4 tint) : m_Texture(texture), m_Tint(tint) {
	m_ShouldSkipHitTest = true;
}

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
	drawList.DrawImage(worldRect, m_Texture, tint, m_UVMin, m_UVMax, 2);
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
	if (name == "bank") {
		m_IconBank = std::string(value);
		return;
	}
	if (name == "icon") {
		auto *loader = static_cast<Core::LayoutLoader *>(loaderCtx);
		if (loader == nullptr) {
			return;
		}
		const std::string &bankName = m_IconBank.empty() ? std::string("default") : m_IconBank;
		TextureIconBank *bank = loader->ResolveTextureIconBank(m_IconBank);
		if (bank == nullptr) {
			AQUILA_LOG_WARNING("Image: no TextureIconBank registered as '{}'", bankName);
			return;
		}
		if (const IconEntry *entry = bank->GetIcon(std::string(value))) {
			SetTexture(entry->texture);
			SetUVRegion(entry->uvMin, entry->uvMax);
		} else {
			AQUILA_LOG_WARNING("Image: icon '{}' not found in bank '{}'", value, bankName);
		}
		return;
	}
	if (name == "uv") {
		float u0 = 0.f, v0 = 0.f, u1 = 1.f, v1 = 1.f;
		std::istringstream ss{ std::string(value) };
		if (ss >> u0 >> v0 >> u1 >> v1) {
			SetUVRegion({ u0, v0 }, { u1, v1 });
		}
		return;
	}
	View::ApplyXmlAttribute(name, value, loaderCtx);
}

} // namespace Aquila::UI::Core
