#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 4) in vec2 fragTexCoord;
layout(location = 5) in mat3 TBN;

layout(location = 0) out vec4 outColor;

struct uboLight {
    int type;
    vec3 color;
    float intensity;
    vec3 direction;
};

layout(std140, set = 0, binding = 0) uniform UniformData {
    mat4 cameraProjection;
    mat4 cameraView;
    mat4 inverseView;
    uboLight light;
    mat4 lightView;
    mat4 lightProjection;
} ubo;

const float gGridSize = 100.0;
const float gGridMinPixelsBetweenCells = 2.0;
const float gGridCellSize = 0.025;
const vec4 gGridColorThin = vec4(0.7, 0.7, 0.7, 1.0);
const vec4 gGridColorThick = vec4(1.0, 1.0, 1.0, 1.0);

float log10(float x) {
    return log(x) / log(10.0);
}

float satf(float x) {
    return clamp(x, 0.0, 1.0);
}

vec2 satv(vec2 x) {
    return clamp(x, vec2(0.0), vec2(1.0));
}

float max2(vec2 v) {
    return max(v.x, v.y);
}

void main() {
    vec3 cameraPos = ubo.inverseView[3].xyz;

    vec2 dvx = vec2(dFdx(fragPosWorld.x), dFdy(fragPosWorld.x));
    vec2 dvy = vec2(dFdx(fragPosWorld.z), dFdy(fragPosWorld.z));

    float lx = length(dvx);
    float ly = length(dvy);

    vec2 dudv = vec2(lx, ly);
    float l = length(dudv);

    float LOD = max(0.0, log10(l * gGridMinPixelsBetweenCells / gGridCellSize) + 1.0);

    float GridCellSizeLod0 = gGridCellSize * pow(10.0, floor(LOD));
    float GridCellSizeLod1 = GridCellSizeLod0 * 10.0;
    float GridCellSizeLod2 = GridCellSizeLod1 * 10.0;

    dudv *= 4.0;

    vec2 mod_div_dudv = mod(fragPosWorld.xz, GridCellSizeLod0) / dudv;
    float Lod0a = max2(vec2(1.0) - abs(satv(mod_div_dudv) * 2.0 - vec2(1.0)));

    mod_div_dudv = mod(fragPosWorld.xz, GridCellSizeLod1) / dudv;
    float Lod1a = max2(vec2(1.0) - abs(satv(mod_div_dudv) * 2.0 - vec2(1.0)));

    mod_div_dudv = mod(fragPosWorld.xz, GridCellSizeLod2) / dudv;
    float Lod2a = max2(vec2(1.0) - abs(satv(mod_div_dudv) * 2.0 - vec2(1.0)));

    float LOD_fade = fract(LOD);
    vec4 Color;

    if (Lod2a > 0.0) {
        Color = gGridColorThick;
        Color.a *= Lod2a;
    } else {
        if (Lod1a > 0.0) {
            Color = mix(gGridColorThick, gGridColorThin, LOD_fade);
            Color.a *= Lod1a;
        } else {
            Color = gGridColorThin;
            Color.a *= (Lod0a * (1.0 - LOD_fade));
        }
    }

    float distance = length(fragPosWorld.xz - cameraPos.xz);
    float OpacityFalloff = 1.0 - satf(distance / gGridSize);
    Color.a *= OpacityFalloff;

    outColor = Color;
}
