#version 450 core

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTangent;
layout(location = 4) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

struct LightData {
    vec3 color;
    float intensity;

    vec3 direction;
    float range;

    vec3 position;
    int type;
    float innerCone;
    float outerCone;
    int isActive;
};

layout(std140, set = 0, binding = 1) uniform Lights {
    LightData lights[32];
};

const vec3 baseColor = vec3(1.0, 0.8, 0.6);

void main() {
    vec3 N = normalize(fragNormal);
    vec3 result = vec3(0.0); // ambient term

    for (int i = 0; i < 32; ++i) {
        LightData light = lights[i];

        if (light.isActive == 0) continue; // still dont get it why this does not kill the light and stop its contribution

        if (light.type == 1) { // directional
            vec3 L = normalize(-light.direction);
            float NdotL = max(dot(N, L), 0.0);
            result += light.color * light.intensity * NdotL;

        } else if (light.type == 0) { // point
            vec3 L = light.position - fragPosition;
            float dist = length(L);
            L = normalize(L);
            float NdotL = max(dot(N, L), 0.0);
            float attenuation = 1.0 / (1.0 + (dist / light.range) * (dist / light.range));
            result += light.color * light.intensity * NdotL * attenuation;

        } else if (light.type == 2) { // spot
            vec3 L = light.position - fragPosition;
            float dist = length(L);
            L = normalize(L);
            float NdotL = max(dot(N, L), 0.0);

            float theta = dot(L, normalize(-light.direction));
            float epsilon = cos(radians(light.innerCone)) - cos(radians(light.outerCone));
            float spotFactor = clamp((theta - cos(radians(light.outerCone))) / epsilon, 0.0, 1.0);

            float attenuation = 1.0 / (1.0 + (dist / light.range) * (dist / light.range));
            result += light.color * light.intensity * NdotL * attenuation * spotFactor;
        }
    }

    outColor = vec4(baseColor * result, 1.0);
}
