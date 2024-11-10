#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 4) in vec2 fragTexCoord;
layout(location = 5) in mat3 TBN;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D albedo;
layout(set = 1, binding = 1) uniform sampler2D normal;
layout(set = 1, binding = 2) uniform sampler2D metallic;

struct uboLight {
    int type;
    vec3 color;
    float intensity;
    vec3 direction;
};

layout(std140, set = 0, binding = 0) uniform UniformData {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    uboLight light;
} ubo;

void main() {
    vec4 albedoColor = texture(albedo, fragTexCoord);
    vec4 normalMap = texture(normal, fragTexCoord);
    vec4 metallicMap = texture(metallic, fragTexCoord);

    // Set final color as default texture
    vec3 finalColor = albedoColor.rgb;

    // Default material properties
    vec3 diffuseColor;
    vec3 specularColor;

    vec3 normal = normalize(fragNormalWorld);


    if (ubo.light.type == 0){
        vec3 lightDir = normalize(ubo.light.direction);
        float diff = max(dot(normal, lightDir), 0.0);
        diffuseColor = diff * ubo.light.color * ubo.light.intensity;

        // Add specular light
        vec3 viewDir = normalize(-fragPosWorld);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        specularColor = spec * ubo.light.color * ubo.light.intensity;

        finalColor *= diffuseColor + specularColor;
    }

    outColor = vec4(finalColor, 1.0);
}