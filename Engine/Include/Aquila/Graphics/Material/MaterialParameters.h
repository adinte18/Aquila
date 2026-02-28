#ifndef AQUILA_MATERIAL_PROPERTY_H
#define AQUILA_MATERIAL_PROPERTY_H

#include "Aquila/Graphics/Resources/Texture2D.h"

namespace Aquila::Graphics::Material {
enum class ParameterType { Float, Int, Bool, Vec2, Vec3, Vec4, Texture2D, Color };

using ParameterValue = std::variant<f32, int, bool, vec2, vec3, vec4, Ref<Resources::Texture2D>>;
struct MaterialParameter {
	std::string m_Name;
	ParameterType m_Type;
	ParameterValue m_Value;
	ParameterValue m_DefaultValue;

	// stuff for editor to eat
	f32 m_MinValue = 0.0f;
	f32 m_MaxValue = 1.0f;
	bool m_IsEditable = true;
	std::string m_DisplayName;

	uint32 m_BindingIndex = -1;

	MaterialParameter() = default;

	MaterialParameter(const std::string &n, const ParameterType t, const ParameterValue &v)
		: m_Name(n), m_Type(t), m_Value(v), m_DefaultValue(v), m_DisplayName(n) {}

	void ResetToDefault() { m_Value = m_DefaultValue; }
};
} // namespace Aquila::Graphics::Material

#endif
