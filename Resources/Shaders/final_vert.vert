#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;
layout(location = 4) in vec2 uv;


vec2 positions[3] = vec2[](
vec2(-1.0, -1.0),
vec2( 3.0, -1.0),
vec2(-1.0,  3.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
