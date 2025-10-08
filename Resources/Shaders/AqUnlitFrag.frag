#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTangent;
layout(location = 4) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D mainTexture;
layout(set = 1, binding = 1) uniform MaterialUBO {
    vec4 color;
} material;

void main() {
    vec4 texColor = texture(mainTexture, fragUV);
    outColor = texColor * material.color;
}