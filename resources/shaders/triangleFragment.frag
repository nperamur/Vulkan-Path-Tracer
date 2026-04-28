#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 passNormal;

layout(set = 1, binding = 0) uniform TriangleUBO {
    vec4 color;
} colors;

layout(set = 0, binding = 0) uniform LightUBO {
    vec4 position;
    vec4 color;
} lightData;

void main() {
    float diffuse = max(dot(normalize(passNormal), normalize(lightData.position.rgb)), 0);
    vec4 ambient = vec4(vec3(0.2), 1.0);
    outColor = min(colors.color * diffuse * lightData.color + ambient * colors.color * lightData.color, 1.0);
}