#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload {
    float distance;
    uint materialIndex;
    uint primitiveIndex;
    vec2 uv;
};


layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
    rayPayload.distance = -1.0f;
    rayPayload.materialIndex = 0;
    rayPayload.primitiveIndex = 0;
    rayPayload.uv = vec2(0.0f);
}