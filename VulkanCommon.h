//
// Created by neele on 4/23/2026.
//

#ifndef VULKAN_TEST_COMMONUTILS_H
#define VULKAN_TEST_COMMONUTILS_H
#include "glm/fwd.hpp"

#include "glm/fwd.hpp"
#include "glm/vec4.hpp"
#include "glm/glm.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "shader-pipeline/ShaderPipeline.h"

struct MVP {
    glm::mat4 transformation;
    glm::mat4 view;
    glm::mat4 projection;
};
struct alignas(16) DirectionalLight {
    glm::vec4 position = glm::vec4(0.0f);
    glm::vec4 color = glm::vec4(0.0f);
    glm::vec4 playerPos = glm::vec4(0.0f);
    int frameCount = 0;
};
struct alignas(16) LightData {
    glm::vec3 emissionFactor;
    int triangleCDFStartIndex;
    glm::vec3 position;
    int triangleCDFStride;
    int materialIndex = -1; //use material index -1 for directional light
    float lightArea = 0;
    float radius = 0;
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

namespace Config {
    inline constexpr int maxFramesInFlight = 3;
}



//Shader slots
namespace RTShaderSlots {
    inline constexpr DescriptorBinding accelerationStructure = {.set=0, .binding=0};
    inline constexpr DescriptorBinding lightUBO = {.set=1, .binding=0};
    inline constexpr DescriptorBinding inverseViewProj = {.set=1, .binding=1};
    inline constexpr DescriptorBinding depthBuffer = {.set = 0, .binding = 1};
    inline constexpr DescriptorBinding rtOutput = {.set = 0, .binding = 4};
    inline constexpr DescriptorBinding materialsBuffer = {.set = 0, .binding = 5};
    inline constexpr DescriptorBinding triangleCdfBuffer = {.set = 0, .binding = 6};
    inline constexpr DescriptorBinding lightCdfBuffer = {.set = 0, .binding = 7};
    inline constexpr DescriptorBinding lightDataBuffer = {.set = 0, .binding = 8};
    inline constexpr DescriptorBinding emissiveVerticesBuffer = {.set = 0, .binding = 9};
    inline constexpr DescriptorBinding visibilityBuffer = {.set = 0, .binding = 2};
    inline constexpr DescriptorBinding normalBuffer = {.set = 0, .binding = 3};
}

namespace ForwardPassShaderSlots {
    inline constexpr DescriptorBinding lightUBO = {.set = 0,.binding = 0};
    inline constexpr DescriptorBinding triangleUBO = {.set = 1,.binding = 0};
    inline constexpr DescriptorBinding mvp = {.set = 1,.binding = 1};
}


namespace CombineShaderSlots {
    inline constexpr DescriptorBinding lightUBO = {.set = 1,.binding = 0};
    inline constexpr DescriptorBinding firstImage = {.set = 0,.binding = 0};
    inline constexpr DescriptorBinding secondImage = {.set = 0,.binding = 1};
    inline constexpr DescriptorBinding historyBuffer = {.set = 0,.binding = 2};

}

namespace ToneMappingShaderSlots {
    inline constexpr DescriptorBinding baseImage = {.set = 0,.binding = 0};
}



class VulkanCommon {

};



#endif //VULKAN_TEST_COMMONUTILS_H
