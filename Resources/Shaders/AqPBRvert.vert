#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragTangent;
layout(location = 4) out vec2 fragUV;

layout(std140, set = 0, binding = 0) uniform SceneUniformData {
    mat4 cameraProjection;
    mat4 cameraView;
    mat4 inverseView;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

    fragUV = uv;
    fragNormal = inverse(transpose(mat3(push.modelMatrix))) * normal;
    fragPosition = positionWorld.xyz;
    fragColor = color;

    gl_Position = ubo.cameraProjection * ubo.cameraView * positionWorld;
}
