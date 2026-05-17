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



class VulkanCommon {

};



#endif //VULKAN_TEST_COMMONUTILS_H
