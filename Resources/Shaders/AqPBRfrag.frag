#version 450 core

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec3 fragTangentWorld;
layout(location = 4) in vec2 fragTexCoord;
layout(location = 5) in vec4 fragPosLightSpace;
layout(location = 6) in mat3 TBN;

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
    int lightCount;
};

layout(std140, set = 0, binding = 0) uniform SceneUniformData {
    mat4 cameraProjection;
    mat4 cameraView;
    mat4 inverseView;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D aoMap;
layout(set = 1, binding = 3) uniform sampler2D emissiveMap;

// Set 1, binding 4: Material properties UBO
layout(set = 1, binding = 4) uniform MaterialUBO {
    vec4 albedoColor;
    vec4 emissiveColor;
    float metallic;
    float roughness;
} material;

const float PI = 3.14159265359;
const vec3 baseColor = vec3(1.0, 0.8, 0.6);

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


void main() {

    vec3 texNormal = texture(normalMap, fragTexCoord).xyz;

    // dummy normal = 0.5, 0.5, 1.0, 1.0
    bool isDummyNormal = all(lessThan(abs(texNormal - vec3(0.5, 0.5, 1.0)), vec3(0.01)));

    vec3 localNormal = isDummyNormal
    ? normalize(fragNormalWorld)
    : normalize(TBN * (texNormal * 2.0 - 1.0));


    vec3 N = normalize(localNormal);
    vec3 V = normalize(vec3(ubo.inverseView[3]) - fragPosWorld);
    vec3 R = reflect(-V, N);
    vec3 result = vec3(0.0);

    for (int i = 0; i < lightCount; ++i) {
        LightData light = lights[i];

        if (light.type == 1) { // directional
            vec3 L = normalize(-light.direction);
            float NdotL = max(dot(N, L), 0.0);
            result += material.albedoColor.rgb * light.color * light.intensity * NdotL;

        } else if (light.type == 0) { // point
            vec3 L = light.position - fragPosWorld;
            float dist = length(L);
            L = normalize(L);
            float NdotL = max(dot(N, L), 0.0);
            float attenuation = 1.0 / (1.0 + (dist / light.range) * (dist / light.range));
            result += material.albedoColor.rgb *light.color * light.intensity * NdotL * attenuation;

        } else if (light.type == 2) { // spot
            vec3 L = light.position - fragPosWorld;
            float dist = length(L);
            L = normalize(L);
            float NdotL = max(dot(N, L), 0.0);

            float theta = dot(L, normalize(-light.direction));
            float epsilon = cos(radians(light.innerCone)) - cos(radians(light.outerCone));
            float spotFactor = clamp((theta - cos(radians(light.outerCone))) / epsilon, 0.0, 1.0);

            float attenuation = 1.0 / (1.0 + (dist / light.range) * (dist / light.range));
            result += material.albedoColor.rgb * light.color * light.intensity * NdotL * attenuation * spotFactor;
        }
    }

    outColor = vec4(baseColor * result, 1.0);
}