#version 450


layout(location = 0) out vec4 fragColor;
void main() {
    float depth = gl_FragCoord.z;

    fragColor = vec4(vec3(depth), 1.0); // Visualize depth
}
