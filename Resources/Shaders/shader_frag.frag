#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec3 fragTangentWorld;
layout(location = 4) in vec2 fragTexCoord;
layout(location = 5) in vec4 fragPosLightSpace;
layout(location = 6) in mat3 TBN;

layout(location = 0) out vec4 outColor;

struct uboLight {
    int type;
    vec3 color;
    float intensity;
    vec3 direction;
};

layout(std140, set = 0, binding = 0) uniform UniformData {
    mat4 cameraProjection;
    mat4 cameraView;
    mat4 inverseView;
    uboLight light;
    mat4 lightSpaceMatrix;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D shadowMap;
layout(set = 0, binding = 2) uniform samplerCube irradianceMap;
layout(set = 0, binding = 3) uniform samplerCube prefilterMap;
layout(set = 0, binding = 4) uniform sampler2D brdfLUT;

layout(set = 1, binding = 0) uniform sampler2D albedo;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughness;
layout(set = 1, binding = 3) uniform sampler2D emissive;
layout(set = 1, binding = 4) uniform sampler2D ao;

layout(set = 1, binding = 6) uniform MaterialProperties {
    vec3 albedoColor;
    float metallic;
    float roughness;
    vec3 emissionColor;
    float emissiveIntensity;
    float aoIntensity;
    vec2 tiling;
    vec2 offset;
    int invertNormalMap;
} material;

const float PI = 3.14159265359;

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

float calculateShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    vec2 UVCoords = projCoords.xy * 0.5 + 0.5;
    float z = projCoords.z;

    if (UVCoords.x < 0.0 || UVCoords.x > 1.0 || UVCoords.y < 0.0 || UVCoords.y > 1.0)
    return 1.0;

    float bias = max(0.005, 0.02 * (1.0 - dot(normalize(fragNormalWorld), normalize(-ubo.light.direction))));
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int x = -3; x <= 3; x++) {
        for (int y = -3; y <= 3; y++) {
            float pcfDepth = texture(shadowMap, UVCoords + vec2(x, y) * texelSize).r;
            shadow += (pcfDepth + bias > z) ? 1.0 : 0.0;
        }
    }
    return shadow / 25.0;
}

void main() {
    vec3 texNormal = texture(normalMap, fragTexCoord).xyz;

    // dummy normal = 0.5, 0.5, 1.0, 1.0
    bool isDummyNormal = all(lessThan(abs(texNormal - vec3(0.5, 0.5, 1.0)), vec3(0.01)));

    vec3 localNormal = isDummyNormal
    ? normalize(fragNormalWorld)                  // Use geometry normal
    : normalize(TBN * (texNormal * 2.0 - 1.0));   // Convert and transform from tangent space

    if (material.invertNormalMap == 1) {
        localNormal = -localNormal;
    }

    vec3 N = normalize(localNormal);
    vec3 L = normalize(-ubo.light.direction);
    vec3 V = normalize(vec3(ubo.inverseView[3]) - fragPosWorld);
    vec3 H = normalize(V + L);
    vec3 R = reflect(-V, N);

    vec3 irradiance = texture(irradianceMap, fragNormalWorld).rgb;

    // Shadow calculation
    float shadow = calculateShadow(fragPosLightSpace);

    vec3 albedoColor = pow(texture(albedo, fragTexCoord).rgb, vec3(2.2)) * material.albedoColor;
    float metallic = texture(metallicRoughness, fragTexCoord).b * material.metallic;
    float roughness = texture(metallicRoughness, fragTexCoord).g * material.roughness;
    vec3 emissive = texture(emissive, fragTexCoord).rgb;
    float ao = texture(ao, fragTexCoord).r;

    // Fresnel
    vec3 F0 = mix(vec3(0.04), albedoColor, metallic);

    const float MAX_REFLECTION_LOD = 8.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlickRoughness(max(dot(N,V), 0.0), F0, roughness);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;

    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedoColor / PI + specular) * ubo.light.color * ubo.light.intensity * NdotL * shadow;

    // Diffuse light from irradiance (ambient lighting)
    vec3 diffuseIrradiance = irradiance * albedoColor;

    // Specular
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuseIrradiance + specularIBL) * ao;
    vec3 color = (ambient + Lo);

    // HDR tonemapping (Reinhard tone mapping)
    color = color / (color + vec3(1.0));

    // Gamma correction (sRGB to linear space)
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color , 1.0);
}
