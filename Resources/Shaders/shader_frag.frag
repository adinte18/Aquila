#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 4) in vec2 fragTexCoord;
layout(location = 5) in vec4 fragPosLightSpace; // Light space position for shadow mapping

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D albedo;
layout(set = 1, binding = 1) uniform sampler2D normal;
layout(set = 1, binding = 2) uniform sampler2D metallic;

layout(set = 0, binding = 1) uniform sampler2D shadowMap; // Shadow map texture

struct uboLight {
    int type;
    vec3 color;
    float intensity;
    vec3 direction; // Direction of light (for directional lights)
};

layout(std140, set = 0, binding = 0) uniform UniformData {
    mat4 cameraProjection;
    mat4 cameraView;
    mat4 inverseView;
    uboLight light;
    mat4 lightSpaceMatrix; // Light's space matrix for shadow mapping
} ubo;

// Function to calculate shadow (depth comparison in light's space)
float calculateShadow(vec4 fragPosLightSpace) {
    vec3 ProjCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    vec2 UVCoords = ProjCoords.xy * 0.5 + 0.5;
    float z = ProjCoords.z;

    if (UVCoords.x < 0.0 || UVCoords.x > 1.0 || UVCoords.y < 0.0 || UVCoords.y > 1.0)
    return 1.0; // oob, assume fully lit

    float bias = max(0.002 * (1.0 - dot(normalize(fragNormalWorld), normalize(-ubo.light.direction))), 0.0005);

    // PCF 5x5 filtering
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for(int x = -2; x <= 2; x++) {
        for(int y = -2; y <= 2; y++) {
            float pcfDepth = texture(shadowMap, UVCoords + vec2(x, y) * texelSize).r;
            shadow += (pcfDepth + bias > z) ? 1.0 : 0.0;
        }
    }
    return shadow / 25.0;
}


void main() {
    vec4 albedoColor = texture(albedo, fragTexCoord);
    vec4 normalMap = texture(normal, fragTexCoord);
    vec4 metallicMap = texture(metallic, fragTexCoord);

    // Set default color to albedo
    vec3 finalColor = albedoColor.rgb;

    // Material properties
    vec3 diffuseColor;
    vec3 specularColor;

    // normal vector
    vec3 normal = normalize(fragNormalWorld);

    // lighting based on the light type
    if (ubo.light.type == 0) { // Directional light
       vec3 lightDir = normalize(-ubo.light.direction); // Ensure direction is correct
       float diff = max(dot(normal, lightDir), 0.0);
       vec3 diffuse = diff * ubo.light.intensity * ubo.light.color;

       // Specular shading (Phong reflection model)
       vec3 viewDir = normalize(vec3(ubo.inverseView[3]) - fragPosWorld);
       vec3 reflectDir = reflect(-lightDir, normal);
       float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
       vec3 specular = spec * ubo.light.color * ubo.light.intensity;

       float shadow = calculateShadow(fragPosLightSpace);

       vec3 ambient = vec3(0.1) * ubo.light.color; // Soft global ambient light
       finalColor *= ambient + (diffuse + specular) * shadow;
    }

    outColor = vec4(finalColor, 1.0); // Output the final shaded color
}
