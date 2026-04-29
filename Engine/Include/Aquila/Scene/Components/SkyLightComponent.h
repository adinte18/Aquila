#ifndef SKYLIGHT_COMPONENT_H
#define SKYLIGHT_COMPONENT_H

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::SceneManagement::Components {

struct SHCoefficients {
	std::array<vec3, 9> coeffs{};

	SHCoefficients() {
		for (auto &coefficient : coeffs) {
			coefficient = vec3(0.0F);
		}
	}
};

struct SkyLightComponent {
	Ref<GFX::GfxTexture> m_HDRTexture;
	SHCoefficients m_Irradiance;

	f32 m_Intensity = 1.0f;
	f32 m_SkyboxLOD = 0;
	vec3 m_Tint = vec3(1.0f);
	bool m_IsActive = true;
	bool m_IsDirty = true;
	bool m_RenderSkybox = true;

  public:
	[[nodiscard]] bool IsActive() const noexcept { return m_IsActive; }
	[[nodiscard]] f32 GetIntensity() const noexcept { return m_Intensity; }
	[[nodiscard]] f32 GetSkyboxLOD() const noexcept { return m_SkyboxLOD; }
	[[nodiscard]] vec3 GetTint() const noexcept { return m_Tint; }
	[[nodiscard]] bool IsDirty() const noexcept { return m_IsDirty; }
	[[nodiscard]] bool ShouldRenderSkybox() const noexcept { return m_RenderSkybox && m_IsActive; }

	[[nodiscard]] const Ref<GFX::GfxTexture> &GetHDRTexture() const noexcept { return m_HDRTexture; }
	[[nodiscard]] const SHCoefficients &GetIrradiance() const noexcept { return m_Irradiance; }

	void SetActive(bool active) noexcept { m_IsActive = active; }
	void SetIntensity(f32 intensity) noexcept { m_Intensity = intensity; }
	void SetSkyboxLOD(f32 lod) noexcept { m_SkyboxLOD = lod; }
	void SetTint(const vec3 &tint) noexcept { m_Tint = tint; }
	void SetRenderSkybox(bool render) noexcept { m_RenderSkybox = render; }

	void SetHDRTexture(const Ref<GFX::GfxTexture> &texture) {
		m_HDRTexture = texture;
		m_IsDirty = true;
	}

	void SetIrradiance(const SHCoefficients &sh) {
		m_Irradiance = sh;
		m_IsDirty = false;
	}

	SkyLightComponent() = default;
	explicit SkyLightComponent(const Ref<GFX::GfxTexture> &texture, f32 intensity = 1.0f, int lod = 0.0f)
		: m_HDRTexture(texture), m_Intensity(intensity), m_SkyboxLOD(lod) {}
};

} // namespace Aquila::SceneManagement::Components

#endif // SKYLIGHT_COMPONENT_H
