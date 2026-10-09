#extension GL_EXT_buffer_reference : require
uint nextRand(inout uint randState) {
    randState ^= randState << 13;
    randState ^= randState >> 17;
    randState ^= randState << 5;
    return randState;
}
float randFloat(inout uint randState) {
    return float(nextRand(randState) & 0x007fffffU) / float(0x00800000U);
}
vec2 randFloat2(inout uint randState) {
    float r1 = randFloat(randState);
    float r2 = randFloat(randState);
    return vec2(r1, r2);
}

const float PI = 3.14159265359;
const float directionalLightAngularRadius = 0.00465;

//TODO: give sun ratio relative to other weights
const float SUN_ANGLE_WEIGHT_MULTIPLIER = 10000;
const float SUN_RADIANCE = 3.33e5;




const int NUM_SAMPLES_PER_PIXEL = 1;
const bool INTERPOLATED_NORMALS = true;
const int NUM_MAX_BOUNCES = 12;
const float lightIntensity = 45;

layout(push_constant) uniform PushConstants {
    uint frameCount;
} pushConstants;

layout(set = 1, binding = 0) uniform LightUBO {
    vec4 position;
    vec4 color;
    vec4 playerPos;
    int frameCount;
} lightData;


layout(buffer_reference, std430) buffer VertexBuffer {
    float data[];
};

layout(buffer_reference, std430) buffer IndexBuffer {
    uint data[];
};

layout(buffer_reference, std430) buffer NormalBuffer {
    float data[];
};


layout(std430, set = 0, binding = 9) buffer TriangleCDFBuffer {
    float data[];
} triangleCDF;

struct Light {
    vec3 emissionFactor;
    int triangleCDFStartIndex;
    vec3 position;
    int triangleCDFStride;
    int materialIndex;
    float lightArea;
    float radius;
};
layout(std430, set = 0, binding = 10) buffer LightCDFBuffer {
    float data[];
} lightCDF;

layout(std430, set = 0, binding = 11) buffer LightDataBuffer {
    uint lightCount;
    Light data[];
} lightDataBuffer;

layout(std430, set = 0, binding = 12) buffer EmissiveVerticesBuffer {
    float data[];
} emissiveVertices;

layout(set = 1, binding = 1) uniform InverseViewProj {
    mat4 invViewMatrix;
    mat4 invProjMatrix;
} inverseViewProj;

//TODO: if ref address is 0 it doesnt exist otherwise it exists
layout(buffer_reference, std430) buffer TextureCoordsBuffer {
    vec2 data[];
};

//layout(set = 0, binding = 1) uniform sampler2D depthBuffer;
//layout(set = 0, binding = 2) uniform sampler2D visibilityBuffer;
layout(set = 0, binding = 4) uniform sampler2D[] baseColorTextures;
layout(set = 0, binding = 5) uniform sampler2D[] normalMapTextures;
layout(set = 0, binding = 6) uniform sampler2D[] metallicRoughnessMapTextures;
layout(set = 0, binding = 7, rgba32f) uniform image2D storageImage;
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
const vec3 skyColor = vec3(0.4, 0.45, 0.9);
//const vec3 skyColor = vec3(0, 0, 0);
const float SKY_RADIANCE = 40.0f;
const float SKY_MULTIPLIER = 1.0f;
#define OP_PUSH 1
#define OP_DELETE 2
#define OP_TRANSMISSION_NONE 3
#define OP_NONE = 0
float calculateSolidAngleFromAngularRadius(float radius) {
    float cosAlpha = cos(radius);
    float solidAngle = 2.0 * PI * (1.0 - cosAlpha);
    return solidAngle;
}
//returns the solid angle.
float sampleBoundingSphere(float boundingRadius, float distance) {
    if (distance <= boundingRadius) {
        return 4.0 * PI;
    }
    return 2 * PI * (1 - sqrt(1 - pow(boundingRadius, 2) / pow(distance, 2)));
}

float getLightSelectionWeightSum(vec3 surfacePos, vec3 normal, bool isTransmissive) {
    float lightWeightSum = 0.0;
    for (int i = 0; i < lightDataBuffer.lightCount; i++) {
        if ((dot(normal, lightDataBuffer.data[i].position - surfacePos) + lightDataBuffer.data[i].radius < 0) && !isTransmissive) {
            continue;
        }
        float dist = length(surfacePos - lightDataBuffer.data[i].position);
        lightWeightSum += sampleBoundingSphere(lightDataBuffer.data[i].radius, dist) * length(lightDataBuffer.data[i].emissionFactor);
    }
    if (length(lightData.color) > 0 && dot(normal, normalize(lightData.position.xyz)) >= -sin(directionalLightAngularRadius) || isTransmissive) {
        lightWeightSum += SUN_RADIANCE * calculateSolidAngleFromAngularRadius(directionalLightAngularRadius);
    }
    return lightWeightSum;
}

vec2 octEncode(vec3 n) {
    vec2 p = n.xy / (abs(n.x) + abs(n.y) + abs(n.z));
    if (n.z < 0.0) {
        p = (1.0 - abs(p.yx)) * (vec2(p.x >= 0.0 ? 1.0 : -1.0, p.y >= 0.0 ? 1.0 : -1.0));
    }
    return p;
}

vec3 octDecode(vec2 p) {
    vec3 n = vec3(p.xy, 1.0 - abs(p.x) - abs(p.y));
    if (n.z < 0.0) {
        vec2 sgn = vec2(n.x >= 0.0 ? 1.0 : -1.0, n.y >= 0.0 ? 1.0 : -1.0);
        n.xy = (1.0 - abs(n.yx)) * sgn;
    }
    return normalize(n);
}

const uint STACK_OP_MASK = 0xFFFFFFFC;
const uint BOUNCE_COUNT_MASK = 0XFFFFFC03;
const uint PREV_BOUNCE_MIS_MASK = 0xFFFFFBFF;
const uint PREV_BOUNCE_DIFFUSE_MASK = 0xFFFFF7FF;

//note: stack op must be valid uint 0-3
uint setStackOp(uint data, uint stackOp) {
   return (data & STACK_OP_MASK) | stackOp;
}

//note: bounceCount is reserved 8 bits for valid number 0-255
uint setBounceCount(uint data, uint bounceCount) {
    return (data & BOUNCE_COUNT_MASK) | (bounceCount << 2);
}

uint setPrevBounceMis(uint data, bool mis) {
    return (data & PREV_BOUNCE_MIS_MASK) | ((mis ? 1 : 0) << 10);
}

uint setPrevBounceDiffuse(uint data, bool diffuse) {
    return (data & PREV_BOUNCE_DIFFUSE_MASK) | ((diffuse ? 1 : 0) << 11);
}

bool getPrevBounceDiffuse(uint data) {
    return (data & (~PREV_BOUNCE_DIFFUSE_MASK)) > 0;
}

uint getStackOp(uint data) {
    return (data & (~STACK_OP_MASK));
}

uint getBounceCount(uint data) {
    return (data & (~BOUNCE_COUNT_MASK)) >> 2;
}

bool getPrevBounceMis(uint data) {
    return (data & (~PREV_BOUNCE_MIS_MASK)) > 0;
}







//layout(set = 0, binding = 3) uniform sampler2D normalBuffer;