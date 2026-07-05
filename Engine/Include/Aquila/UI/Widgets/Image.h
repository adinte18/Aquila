#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::UI::Core {

class Image : public View {
  public:
	Image();
	Image(GFX::GfxTexture *texture, Vec4 tint = Vec4(1.F));

	[[nodiscard]] std::string_view get_type_name() const override { return "Image"; }

	void set_texture(GFX::GfxTexture *texture);
	void set_tint(Vec4 tint);
	void set_uv_region(Vec2 uv_min, Vec2 uv_max);

	[[nodiscard]] GFX::GfxTexture *get_texture() const { return m_texture; }
	[[nodiscard]] Vec4 get_tint() const { return m_tint; }
	[[nodiscard]] Vec2 get_uv_min() const { return m_uv_min; }
	[[nodiscard]] Vec2 get_uv_max() const { return m_uv_max; }

	// Returns the texture dimensions so Clay auto-sizes the widget when no CSS size is set.
	[[nodiscard]] Vec2 get_intrinsic_size() const override {
		if (m_texture == nullptr) {
			return { -1.F, -1.F };
		}
		const Vec2 tex_size = { static_cast<float>(m_texture->get_width()),
								static_cast<float>(m_texture->get_height()) };
		const Vec2 uv_span = m_uv_max - m_uv_min;
		return tex_size * uv_span;
	}

	void on_draw_self(Rendering::DrawList &draw_list) override;
	void apply_xml_attribute(std::string_view name, std::string_view value, void *loader_ctx = nullptr) override;

  private:
	GFX::GfxTexture *m_texture = nullptr;
	Vec4 m_tint = Vec4(1.F);
	Vec2 m_uv_min = { 0.F, 0.F };
	Vec2 m_uv_max = { 1.F, 1.F };
	std::string m_icon_bank; // remembers the "bank" attribute so a later "icon" resolves against it
};

} // namespace Aquila::UI::Core
