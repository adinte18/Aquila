#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec3 fragTangentWorld;
layout(location = 4) out vec2 fragTexCoord;
layout(location = 5) out vec4 fragPosLightSpace;
layout(location = 6) out mat3 TBN;

struct uboLight {
    int type;
    vec3 color;
    float intensity;
    vec3 position;
};

layout(std140, set = 0, binding = 0) uniform UniformData {
    mat4 cameraProjection;
    mat4 cameraView;
    mat4 inverseView;
    uboLight light;
    mat4 lightSpaceMatrix;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

    fragTexCoord = uv;
    fragNormalWorld = inverse(transpose(mat3(push.modelMatrix))) * normal;
    fragPosWorld = positionWorld.xyz;
    fragColor = color;

    fragPosLightSpace = ubo.lightSpaceMatrix * vec4(fragPosWorld, 1.0);

    mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)));

    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * normal);

    T = normalize(T - dot(T, N) * N);

    vec3 B = cross(N, T);
    TBN = transpose(mat3(T, B, N));

    gl_Position = ubo.cameraProjection * ubo.cameraView * positionWorld;
}
