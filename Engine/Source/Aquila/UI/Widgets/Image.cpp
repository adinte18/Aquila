#include "Aquila/UI/Widgets/Image.h"

namespace Aquila::UI::Core {

Image::Image(GFX::GfxTexture *texture, vec4 tint) : m_Texture(texture), m_Tint(tint) {}

void Image::OnDrawSelf(Rendering::DrawList &drawList) {
	View::OnDrawSelf(drawList);

	if (m_Texture == nullptr) {
		return;
	}

	const Rect worldRect = { .position = GetAbsolutePosition(), .size = GetLayoutRect().size };
	const vec4 tint = m_Tint * GetDisplayStyle().color;
	drawList.DrawImage(worldRect, m_Texture, tint, m_UVMin, m_UVMax, GetDisplayStyle().zIndex * 4 + 2);
}

void Image::OnDraw(Rendering::DrawList &drawList, vec2 origin) {
	View::OnDraw(drawList, origin); // background + children

	if (m_Texture == nullptr) {
		return;
	}

	const Rect &layout = GetLayoutRect();
	const Rect worldRect{ .position = layout.position + origin, .size = layout.size };

	const vec4 tint = m_Tint * GetDisplayStyle().color;
	drawList.DrawImage(worldRect, m_Texture, tint, m_UVMin, m_UVMax, GetDisplayStyle().zIndex * 4 + 2);
}

} // namespace Aquila::UI::Core
