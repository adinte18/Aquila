#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H

#include "AquilaCore.h"

struct LightComponent {
    enum class Type {
        Point,
        Directional,
        Spot
    };

    Type m_Type = Type::Directional;

    // general light properties
    glm::vec3 m_Color = glm::vec3(1.0f);
    float m_Intensity = 1.0f;
    float m_Range = 100.0f;
    
    // spot light
    float m_InnerConeAngle = 0.0f;
    float m_OuterConeAngle = 45.0f;
    
    // directional light
    glm::vec3 m_Direction = glm::vec3(0.0f, -1.0f, 0.0f);

    bool m_IsActive = true;

    LightComponent() = default;

    LightComponent(Type lightType, const glm::vec3& lightColor, float lightIntensity)
        : m_Type(lightType), m_Color(lightColor), m_Intensity(lightIntensity) {}
};

#endif // LIGHT_COMPONENT_H