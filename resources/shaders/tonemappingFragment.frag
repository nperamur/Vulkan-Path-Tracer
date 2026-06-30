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

vec3 agxDefaultContrastApprox(vec3 x) {
    vec3 x2 = x * x;
    vec3 x4 = x2 * x2;
    return + 15.5     * x4 * x2
           - 40.14    * x4 * x
           + 31.96    * x4
           - 6.868    * x2 * x
           + 0.4298   * x2
           + 0.1191   * x
           - 0.00232;
}

vec3 agx(vec3 val) {
    const mat3 agx_mat = mat3(
        0.842479062253094,  0.0423282422610123, 0.0423756549057051,
        0.0784335999999992, 0.878468636469772,  0.0784336,
        0.0792237451477643, 0.0791661274605434, 0.879142973793104
    );

    const float min_ev = -12.47393;
    const float max_ev =  4.026069;

    val = agx_mat * val;
    val = clamp(log2(val), min_ev, max_ev);
    val = (val - min_ev) / (max_ev - min_ev);
    val = agxDefaultContrastApprox(val);
    return val;
}

vec3 agxEotf(vec3 val) {
    const mat3 agx_mat_inv = mat3(
         1.19687900512017,   -0.0528968517574562, -0.0529716355144438,
        -0.0980208811401368,  1.15190312990417,   -0.0980434501171241,
        -0.0990297440797205, -0.0989611768448433,  1.15107367264116
    );
    val = agx_mat_inv * val;
    return val;
}

vec3 agxLook(vec3 val) {
    const vec3 lw = vec3(0.2126, 0.7152, 0.0722);
    float luma = dot(val, lw);

    vec3 offset = vec3(0.0);
    vec3 slope  = vec3(1.0);
    float power = 1.5;
    float sat   = 1.2;

    val = pow(val * slope + offset, vec3(power));
    return luma + sat * (val - luma);
}

vec3 toneMapAgX(vec3 color) {
    color = agx(color);
    color = agxLook(color);
    color = agxEotf(color);
    return color;
}


void main() {
    vec3 baseColor = texture(baseImage, passTextureCoords).rgb;
    outColor = vec4(toneMapAgX(baseColor * 0.4), 1.0);

}