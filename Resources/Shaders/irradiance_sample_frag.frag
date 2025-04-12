#version 450

layout(location = 0) in vec3 worldPos;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main() {
    vec3 N = normalize(worldPos);

    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));

    float sampleDelta = 0.05;
    float nrSamples = 0.0;
    vec3 irradiance = vec3(0.0);

    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            vec3 sampleColor = texture(environmentMap, sampleVec).rgb;

            irradiance += sampleColor * cos(theta) * sin(theta);

            nrSamples += 1.0;
        }
    }

    if (nrSamples > 0.0) {
        irradiance = PI * irradiance / nrSamples;
    }

    outColor = vec4(irradiance, 1.0);
}
