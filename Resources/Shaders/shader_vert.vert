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
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    uboLight light;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projection * ubo.view * positionWorld;
    gl_Position.y = -gl_Position.y;

    fragTexCoord = uv;

    vec3 T = normalize(mat3(push.normalMatrix) * tangent.xyz);
    vec3 N = normalize(mat3(push.normalMatrix) * normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN = transpose(mat3(T, B, N));

    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;
}