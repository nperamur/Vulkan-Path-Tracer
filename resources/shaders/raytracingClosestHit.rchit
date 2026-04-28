#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload {
    vec3 hitColor;
    float distance;
};
hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
    rayPayload.distance = gl_HitTEXT;
    rayPayload.hitColor = vec3(0.0f);
}