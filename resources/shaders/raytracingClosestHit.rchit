#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_tracing_position_fetch : require
struct RayPayload {
    vec3 normal;
    float distance;
    uint materialIndex;
    uint primitiveIndex;
    uint pad1;
    uint pad2;
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

    vec3 v0 = gl_HitTriangleVertexPositionsEXT[0];
    vec3 v1 = gl_HitTriangleVertexPositionsEXT[1];
    vec3 v2 = gl_HitTriangleVertexPositionsEXT[2];
    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;
    vec3 objNormal = normalize(cross(e1, e2));
    if (gl_HitKindEXT == -gl_HitKindBackFacingTriangleEXT) {
        objNormal = -objNormal;
    }
    vec3 worldNormal = normalize(transpose(mat3(gl_WorldToObjectEXT)) * objNormal);
    rayPayload.normal = worldNormal;
    rayPayload.primitiveIndex = gl_PrimitiveID;
}