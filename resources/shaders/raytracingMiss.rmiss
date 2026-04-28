#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload {
    vec3 hitColor;
    float distance;
};


layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
    rayPayload.hitColor = vec3(1.0f);
    rayPayload.distance = -1.0f;
}