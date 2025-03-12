#version 450
layout(set = 0, binding = 0) uniform sampler2D sceneTexture;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

// --- Settings ---
// TODO : make these dynamically adjustable
const float exposure = 0.8;              // adjust scene brightness (lower = darker, higher = brighter)
const float aberrationStrength = 0.0015; // chromatic aberration intensity

// --- ACES Film Tonemapping ---
vec3 ACESFilm(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// --- Chromatic Aberration ---
vec3 ApplyChromaticAberration() {
    vec2 texelOffset = aberrationStrength * (uv - 0.5); // Offset based on distance from center
    float r = texture(sceneTexture, uv + texelOffset).r;
    float g = texture(sceneTexture, uv).g;
    float b = texture(sceneTexture, uv - texelOffset).b;
    return vec3(r, g, b);
}


// --- Color Grading ---
vec3 SubtleCoolness(vec3 color) {
    vec3 coolTint = vec3(0.95, 1.0, 1.2); // blue boost, keeping balance
    color = mix(color, color * coolTint, 0.3); // shift while keeping vibrance
    return color;
}


void main() {
    vec3 color = ApplyChromaticAberration();
    color *= exposure;                   // Exposure control
    color = SubtleCoolness(color);     // Film grading
    color = ACESFilm(color);              // Final ACES tonemapping
    outColor = vec4(color, 1.0);
}

