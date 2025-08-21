#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H

#include "AquilaCore.h"

struct LightComponent {
    enum class Type {
        Point,
        Directional,
        Spot
    };

    Type type = Type::Directional;

    // general light properties
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float range = 100.0f;
    
    // spot light
    float innerConeAngle = 0.0f;
    float outerConeAngle = 45.0f;
    
    // directional light
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);

    LightComponent() = default;

    LightComponent(Type lightType, const glm::vec3& lightColor, float lightIntensity)
        : type(lightType), color(lightColor), intensity(lightIntensity) {}
};

#endif // LIGHT_COMPONENT_H