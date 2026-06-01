#version 450

layout(set = 0, binding = 0) uniform sampler2D baseImage;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 passTextureCoords;


//  Tonemapping curve sourced from: Baking Lab https://github.com/TheRealMJP/BakingLab?tab=MIT-1-ov-file
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//  The tonemapping curve in this file was originally written by Stephen Hill (@self_shadow)
//  Tonemapping curve licensed under the MIT license
//
const mat3 ACESInputMat = mat3(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
);

const mat3 ACESOutputMat = mat3(
    1.60475, -0.53108, -0.07367,
    -0.10210,  1.10813, -0.00603,
    -0.00327, -0.07276,  1.07602
);

vec3 RRTAndODTFit(vec3 v) {
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 ACESFilmicToneMapping(vec3 color) {
    vec3 focusedColor = color * ACESInputMat;
    vec3 fittedColor = RRTAndODTFit(focusedColor);
    vec3 outputColor = fittedColor * ACESOutputMat;
    return clamp(outputColor, 0.0, 1.0);
}



void main() {
    vec3 baseColor = texture(baseImage, passTextureCoords).rgb;
    outColor = vec4(ACESFilmicToneMapping(baseColor * 1.65), 1.0);
}