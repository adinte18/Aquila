#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H

#include "Aquila/Core/AquilaCore.h"

namespace Aquila::SceneManagement::Components {

struct ShadowQualitySettings {
	f32 lightSize = 8.0f;
	f32 shadowBias = 0.0035f;
	f32 normalBias = 1.0f;
	int pcfSamples = 9;
	f32 cascadeSplitLambda = 0.95f;
	int blockerSearchSamples = 16;
};

struct LightComponent {
	enum class Type { Point, Directional, Spot };

	Type m_Type = Type::Directional;

	vec3 m_Color = vec3(1.0f);
	f32 m_Intensity = 1.0f;
	f32 m_Range = 100.0f;

	f32 m_InnerConeAngle = 0.0f;
	f32 m_OuterConeAngle = 45.0f;

	vec3 m_Direction = vec3(0.0f, -1.0f, 0.0f);

	ShadowQualitySettings m_ShadowSettings;

	bool m_IsActive = true;

  public:
	bool IsActive() const { return m_IsActive; }
	void SetActive(bool active) { m_IsActive = active; }

	Type GetType() const noexcept { return m_Type; }
	void SetType(Type type) noexcept { m_Type = type; }

	const vec3 &GetColor() const noexcept { return m_Color; }
	void SetColor(const vec3 &color) noexcept { m_Color = color; }

	f32 GetIntensity() const noexcept { return m_Intensity; }
	void SetIntensity(f32 intensity) noexcept { m_Intensity = intensity; }

	f32 GetRange() const noexcept { return m_Range; }
	void SetRange(f32 range) noexcept { m_Range = range; }

	f32 GetInnerConeAngle() const noexcept { return m_InnerConeAngle; }
	void SetInnerConeAngle(f32 angle) noexcept { m_InnerConeAngle = angle; }

	f32 GetOuterConeAngle() const noexcept { return m_OuterConeAngle; }
	void SetOuterConeAngle(f32 angle) noexcept { m_OuterConeAngle = angle; }

	const vec3 &GetDirection() const noexcept { return m_Direction; }
	void SetDirection(const vec3 &direction) noexcept { m_Direction = direction; }

	ShadowQualitySettings &GetShadowSettings() noexcept { return m_ShadowSettings; }
	const ShadowQualitySettings &GetShadowSettings() const noexcept { return m_ShadowSettings; }
	void SetShadowSettings(const ShadowQualitySettings &settings) noexcept { m_ShadowSettings = settings; }

	LightComponent() = default;

	LightComponent(Type lightType, const vec3 &lightColor, f32 lightIntensity)
		: m_Type(lightType), m_Color(lightColor), m_Intensity(lightIntensity) {}
};
} // namespace Aquila::SceneManagement::Components
#endif
