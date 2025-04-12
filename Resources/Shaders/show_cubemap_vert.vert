#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec3 WorldPos;

layout(push_constant) uniform Push {
    mat4 viewMatrix;
    mat4 projectionMatrix;
} push;


void main() {
    WorldPos = position;
    vec4 pos = push.projectionMatrix * push.viewMatrix * vec4(position, 1.0);
    gl_Position = pos.xyww;
}
