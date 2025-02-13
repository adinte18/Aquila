#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec3 fragPosition;

struct uboLight {
    int type;         // Light type (directional, point, etc.)
    vec3 color;       // Light color
    float intensity;  // Light intensity
    vec3 direction;   // Light direction
};

layout(std140, set = 0, binding = 0) uniform UniformData {
    mat4 cameraProjection;
    mat4 cameraView;
    mat4 inverseView;
    uboLight light;
    mat4 lightView;
    mat4 lightProjection;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main(){
    mat4 viewProjection = ubo.lightProjection * ubo.lightView;

    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

    gl_Position = viewProjection * positionWorld;
    gl_Position.y = -gl_Position.y;
}