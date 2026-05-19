#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_tracing_position_fetch : require

struct RayPayload {
    vec4 hitColor;
    vec4 normal;
    float distance;
    uint materialIndex;
    float pad1;
    float pad2;
};

layout(shaderRecordEXT, std430) buffer Record {
    uint materialIndex;
} sbtRecord;

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
    rayPayload.distance = gl_HitTEXT;
    rayPayload.hitColor = vec4(0.0f);
    //rayPayload.materialIndex = sbtRecord.materialIndex;
    rayPayload.materialIndex = gl_InstanceCustomIndexEXT;

    vec3 v0 = gl_HitTriangleVertexPositionsEXT[0];
    vec3 v1 = gl_HitTriangleVertexPositionsEXT[1];
    vec3 v2 = gl_HitTriangleVertexPositionsEXT[2];
    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;
    vec3 objNormal = normalize(cross(e1, e2));
    vec3 worldNormal = normalize(transpose(mat3(gl_WorldToObjectEXT)) * objNormal);
    rayPayload.normal = vec4(worldNormal, 1);
}