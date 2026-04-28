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

struct MVP {
    glm::mat4 transformation;
    glm::mat4 view;
    glm::mat4 projection;
};

class VulkanCommon {

};



#endif //VULKAN_TEST_COMMONUTILS_H
