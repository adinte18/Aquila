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

void main() {

    vec4 albedoColor = texture(albedo, fragTexCoord);
    vec4 normalMap = texture(normal, fragTexCoord);
    vec4 metallicMap = texture(metallic, fragTexCoord);

    vec3 finalColor = albedoColor.rgb;

    outColor = vec4(finalColor, 1.0);
}