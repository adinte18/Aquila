#version 450

layout(location = 0) in vec3 worldPos;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183); // = 1 / vec2(PI, PI / 2)
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    vec3 dir = normalize(worldPos);
    vec2 uv = SampleSphericalMap(dir);
    vec3 color = texture(equirectangularMap, uv).rgb;
    outColor = vec4(color, 1.0);
}
