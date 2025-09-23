#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H

#include "AquilaCore.h"

struct LightComponent {
  enum class Type { Point, Directional, Spot };

  Type m_Type = Type::Directional;

  // general light properties
  vec3 m_Color = vec3(1.0f);
  f32 m_Intensity = 1.0f;
  f32 m_Range = 100.0f;

  // spot light
  f32 m_InnerConeAngle = 0.0f;
  f32 m_OuterConeAngle = 45.0f;

  // directional light
  vec3 m_Direction = vec3(0.0f, -1.0f, 0.0f);

  bool m_IsActive = true;

  LightComponent() = default;

  LightComponent(Type lightType, const vec3 &lightColor, f32 lightIntensity)
      : m_Type(lightType), m_Color(lightColor), m_Intensity(lightIntensity) {}
};

#endif // LIGHT_COMPONENT_H