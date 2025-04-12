#version 450

layout(location = 0) in vec3 WorldPos;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform samplerCube envMap;

void main() {
    vec3 color = texture(envMap, WorldPos).rgb;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    fragColor = vec4(color, 1.0);
}
