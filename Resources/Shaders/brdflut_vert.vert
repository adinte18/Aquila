#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec2 texCoords;

vec2 positions[3] = vec2[](
vec2(-1.0, -1.0),  // bottom-left
vec2( 3.0, -1.0),  // bottom-right (overshoots to cover top-right)
vec2(-1.0,  3.0)   // top-left (overshoots to cover top-right)
);

vec2 uvs[3] = vec2[](
vec2(0.0, 0.0),
vec2(2.0, 0.0),
vec2(0.0, 2.0)
);

void main()
{
    texCoords = uvs[gl_VertexIndex];
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
