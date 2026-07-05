#ifndef SKYLIGHT_COMPONENT_H
#define SKYLIGHT_COMPONENT_H

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::SceneManagement::Components {

struct SHCoefficients {
	std::array<Vec3, 9> coeffs{};

	SHCoefficients() {
		for (auto &coefficient : coeffs) {
			coefficient = Vec3(0.0F);
		}
	}
};

struct SkyLightComponent {
	Ref<GFX::GfxTexture> m_hdr_texture;
	SHCoefficients m_irradiance;

	F32 m_intensity = 1.0f;
	F32 m_skybox_lod = 0;
	Vec3 m_tint = Vec3(1.0f);
	bool m_is_active = true;
	bool m_is_dirty = true;
	bool m_render_skybox = true;

  public:
	[[nodiscard]] bool is_active() const noexcept { return m_is_active; }
	[[nodiscard]] F32 get_intensity() const noexcept { return m_intensity; }
	[[nodiscard]] F32 get_skybox_lod() const noexcept { return m_skybox_lod; }
	[[nodiscard]] Vec3 get_tint() const noexcept { return m_tint; }
	[[nodiscard]] bool is_dirty() const noexcept { return m_is_dirty; }
	[[nodiscard]] bool should_render_skybox() const noexcept { return m_render_skybox && m_is_active; }

	[[nodiscard]] const Ref<GFX::GfxTexture> &get_hdr_texture() const noexcept { return m_hdr_texture; }
	[[nodiscard]] const SHCoefficients &get_irradiance() const noexcept { return m_irradiance; }

	void set_active(bool active) noexcept { m_is_active = active; }
	void set_intensity(F32 intensity) noexcept { m_intensity = intensity; }
	void set_skybox_lod(F32 lod) noexcept { m_skybox_lod = lod; }
	void set_tint(const Vec3 &tint) noexcept { m_tint = tint; }
	void set_render_skybox(bool render) noexcept { m_render_skybox = render; }

	void set_hdr_texture(const Ref<GFX::GfxTexture> &texture) {
		m_hdr_texture = texture;
		m_is_dirty = true;
	}

	void set_irradiance(const SHCoefficients &sh) {
		m_irradiance = sh;
		m_is_dirty = false;
	}

	SkyLightComponent() = default;
	explicit SkyLightComponent(const Ref<GFX::GfxTexture> &texture, F32 intensity = 1.0f, int lod = 0.0f)
		: m_hdr_texture(texture), m_intensity(intensity), m_skybox_lod(lod) {}
};

} // namespace Aquila::SceneManagement::Components

#endif // SKYLIGHT_COMPONENT_H
