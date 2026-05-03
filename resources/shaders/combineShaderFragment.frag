#version 450
layout(set = 0, binding = 0) uniform sampler2D firstColor;
layout(set = 0, binding = 1) uniform sampler2D secondColor;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 passTextureCoords;

void main() {
    vec4 first = texture(firstColor, passTextureCoords);
    vec4 second = texture(secondColor, passTextureCoords);

    vec4 lightFactor = vec4(0.50) + 0.5 * second;
    outColor = first * lightFactor;
}