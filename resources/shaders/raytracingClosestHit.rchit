#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload {
    vec3 hitColor;
    float distance;
    uint materialIndex;
};

layout(shaderRecordEXT, std430) buffer Record {
    uint materialIndex;
} sbtRecord;

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
    rayPayload.distance = gl_HitTEXT;
    rayPayload.hitColor = vec3(0.0f);
    rayPayload.materialIndex = sbtRecord.materialIndex;
}