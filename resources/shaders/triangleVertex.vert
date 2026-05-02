#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(set = 1, binding = 1) uniform transformUBO {
    mat4 transformationMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
} transform;

layout(location = 0) out vec3 passNormal;

void main() {
    gl_Position = transform.projectionMatrix * transform.viewMatrix * transform.transformationMatrix * vec4(inPosition, 1.0);

    passNormal = inNormal.rgb;
}