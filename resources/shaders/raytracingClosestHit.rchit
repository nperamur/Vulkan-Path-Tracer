#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_tracing_position_fetch : require
struct RayPayload {
    float distance;
    uint materialIndex;
    uint primitiveIndex;
    vec2 uv;
};


layout(shaderRecordEXT, std430) buffer Record {
    uint materialIndex;
} sbtRecord;

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
    rayPayload.distance = gl_HitTEXT;
    //rayPayload.materialIndex = sbtRecord.materialIndex;
    rayPayload.materialIndex = gl_InstanceCustomIndexEXT;
    rayPayload.primitiveIndex = gl_PrimitiveID;
    rayPayload.uv = attribs;
}