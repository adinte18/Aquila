#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 4) out vec2 fragTexCoord;
layout(location = 5) out mat3 TBN;

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
    mat4 lightView;
    mat4 lightProjection;
    vec3 cameraPosition;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    // Compute world position normally
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

    positionWorld.xz += ubo.cameraPosition.xz;

    // Transform into clip space
    gl_Position = ubo.cameraProjection * ubo.cameraView * positionWorld;

    fragTexCoord = uv;
    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;
}
