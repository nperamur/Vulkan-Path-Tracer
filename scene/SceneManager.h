
#ifndef VULKAN_TEST_SCENEMANAGER_H
#define VULKAN_TEST_SCENEMANAGER_H
#include <vulkan/vulkan_raii.hpp>
#include "../Entity.h"
#include "glm/fwd.hpp"
#include "glm/vec3.hpp"
#include "glm/glm.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
class Entity;
struct MVP;
class Loader;

class SceneManager {

    std::vector<Entity> entities;
    public:
        SceneManager(Loader& loader, vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice, MVP& mvp);

        void updateAndDrawEntities(std::function<void()> updateTransformUniform,  std::function<void(Entity&)> draw);

        std::vector<Entity>& getEntities() {
            return entities;
        }
};



#endif //VULKAN_TEST_SCENEMANAGER_H
