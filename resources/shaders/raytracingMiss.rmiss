#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload {
    vec4 hitColor;
    vec4 normal;
    float distance;
    uint materialIndex;
    float pad1;
    float pad2;
};


layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
    rayPayload.hitColor = vec4(1.0f);
    rayPayload.distance = -1.0f;
    rayPayload.materialIndex = 0;
    rayPayload.normal = vec4(0);
}