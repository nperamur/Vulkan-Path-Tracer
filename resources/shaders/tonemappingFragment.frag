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

// ---------------------------------------------------------------------
// AgX tonemapping — from dmnsgn/glsl-tone-map (MIT License)
// https://github.com/dmnsgn/glsl-tone-map/blob/main/agx.glsl
//
// MIT License
//
// Copyright (c) 2022 dmnsgn
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Original references credited by dmnsgn:
// Missing Deadlines (Benjamin Wrensch): https://iolite-engine.com/blog_posts/minimal_agx_implementation
// Filament: https://github.com/google/filament/blob/main/filament/src/ToneMapper.cpp#L263
// https://github.com/EaryChow/AgX_LUT_Gen/blob/main/AgXBaseRec2020.py
// Three.js: https://github.com/mrdoob/three.js/blob/4993e3af579a27cec950401b523b6e796eab93ec/src/renderers/shaders/ShaderChunk/tonemapping_pars_fragment.glsl.js#L79-L89

const mat3 LINEAR_REC2020_TO_LINEAR_SRGB = mat3(
  1.6605, -0.1246, -0.0182,
  -0.5876, 1.1329, -0.1006,
  -0.0728, -0.0083, 1.1187
);
const mat3 LINEAR_SRGB_TO_LINEAR_REC2020 = mat3(
  0.6274, 0.0691, 0.0164,
  0.3293, 0.9195, 0.0880,
  0.0433, 0.0113, 0.8956
);
const mat3 AgXInsetMatrix = mat3(
  0.856627153315983, 0.137318972929847, 0.11189821299995,
  0.0951212405381588, 0.761241990602591, 0.0767994186031903,
  0.0482516061458583, 0.101439036467562, 0.811302368396859
);
const mat3 AgXOutsetMatrix = mat3(
  1.1271005818144368, -0.1413297634984383, -0.14132976349843826,
  -0.11060664309660323, 1.157823702216272, -0.11060664309660294,
  -0.016493938717834573, -0.016493938717834257, 1.2519364065950405
);
const float AgxMinEv = -12.47393;
const float AgxMaxEv = 4.026069;

vec3 agxCdl(vec3 color, vec3 slope, vec3 offset, vec3 power, float saturation) {
  color = LINEAR_SRGB_TO_LINEAR_REC2020 * color;

  color = AgXInsetMatrix * color;
  color = max(color, 1e-10);
  color = clamp(log2(color), AgxMinEv, AgxMaxEv);
  color = (color - AgxMinEv) / (AgxMaxEv - AgxMinEv);
  color = clamp(color, 0.0, 1.0);

  vec3 x2 = color * color;
  vec3 x4 = x2 * x2;
  color = + 15.5     * x4 * x2
          - 40.14    * x4 * color
          + 31.96    * x4
          - 6.868    * x2 * color
          + 0.4298   * x2
          + 0.1191   * color
          - 0.00232;

  color = max(color, 0.0);
  color = pow(color * slope + offset, power);
  const vec3 lw = vec3(0.2126, 0.7152, 0.0722);
  float luma = dot(color, lw);
  color = luma + saturation * (color - luma);
  color = pow(max(vec3(0.0), color), vec3(2.2));
  color = AgXOutsetMatrix * color;
  color = LINEAR_REC2020_TO_LINEAR_SRGB * color;

  color = clamp(color, 0.0, 1.0);

  return color;
}

vec3 agx(vec3 color) {
  return agxCdl(color, vec3(1.0), vec3(0.0), vec3(1.0), 1.0);
}

vec3 agxPunchy(vec3 color) {
  return agxCdl(color, vec3(1.0), vec3(0.0), vec3(1.35), 1.4);
}

vec3 agxCustom(vec3 color) {
  return agxCdl(color, vec3(1.0), vec3(0.0), vec3(0.8), 1.6);
}


// ---------------------------------------------------------------------

void main() {
    vec3 baseColor = texture(baseImage, passTextureCoords).rgb;
    outColor = vec4(agxCustom(baseColor * 0.1), 1.0);
    //outColor = vec4(agx(baseColor * 0.25), 1.0);
    //outColor = vec4(ACESFilmicToneMapping(baseColor * 0.25), 1.0);

}