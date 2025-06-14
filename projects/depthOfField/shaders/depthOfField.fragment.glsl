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

    FragColor = vec4(blurFactor, blurFactor, blurFactor, 1);
}