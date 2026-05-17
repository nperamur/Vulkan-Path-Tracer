#version 450
layout(set = 1, binding = 0) uniform LightUBO {
    vec4 position;
    vec4 color;
    vec4 playerPos;
    int frameCount;
} lightData;
layout(set = 0, binding = 0) uniform sampler2D firstColor;
layout(set = 0, binding = 1) uniform sampler2D secondColor;
layout(set = 0, binding = 2) uniform sampler2D historyBuffer;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 passTextureCoords;

void main() {
    vec4 first = texture(firstColor, passTextureCoords);
    vec4 second = texture(secondColor, passTextureCoords);
    vec4 history = texture(historyBuffer, passTextureCoords);

    vec4 lightFactor = vec4(0.50) + 0.5 * second;
    //outColor = history + (first * lightFactor - history) * 0.1;
    //outColor = first * lightFactor;
    //outColor = history + (second - history) * 0.1;
    if (lightData.frameCount < 3) {
        outColor = second;
    } else {
        outColor = history + (second - history) / float(lightData.frameCount);
    }
}