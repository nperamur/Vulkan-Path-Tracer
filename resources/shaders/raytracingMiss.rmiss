#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload {
    vec3 normal;
    float distance;
    uint materialIndex;
    uint primitiveIndex;
    uint pad1;
    uint pad2;
};


layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
    rayPayload.distance = -1.0f;
    rayPayload.materialIndex = 0;
    rayPayload.normal = vec3(0);
    rayPayload.primitiveIndex = 0;
}