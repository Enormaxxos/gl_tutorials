#version 430 core

in vec2 texCoords;

layout(binding = 0) uniform sampler2D u_sceneTexture;
layout(binding = 1) uniform sampler2D u_depthTexture;

layout(location = 15) uniform float u_focusDistance;
layout(location = 20) uniform float u_focusRange;

out vec4 FragColor;

void main() {
    float depth = texture(u_depthTexture, texCoords).r;

    float focusDistance = u_focusDistance;
    float focusRange = u_focusRange;

    float blurFactor = clamp(abs(depth - focusDistance) / focusRange, 0.0, 1.0);

    // simple box blur for demonstration (can be better, but im no artist)
    vec4 color = vec4(0.0);
    int kernelSize = int(blurFactor * 10.0);
    for (int x = -kernelSize; x <= kernelSize; ++x) {
        for (int y = -kernelSize; y <= kernelSize; ++y) {
            vec2 offset = vec2(x, y) / vec2(textureSize(u_sceneTexture, 0));
            color += texture(u_sceneTexture, texCoords + offset);
        }
    }
    color /= pow(2 * kernelSize + 1, 2);

    FragColor = color;
}