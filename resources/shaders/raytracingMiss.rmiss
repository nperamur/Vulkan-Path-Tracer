#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#include "raytracingUtils.glsl"

struct RayPayload {
    vec3 throughput;
    uint normal;
    vec3 accumulation;
    uint pathInfo;
    float distance;
    float mediumDist;
    float bsdfPDF;
    float lobePDF;
    uint randState;
    uint coneData;
    uint nextRayDir;
    int16_t currentMedium;
    int16_t outerIorId;
};



layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
    rayPayload.distance = -1.0f;
}