
#include "SceneManager.h"

#include "../ModelLoader.h"
#include "../VulkanCommon.h"
#include "GLFW/glfw3.h"

#include "glm/vec3.hpp"
#include "glm/ext/matrix_transform.hpp"
SceneManager::SceneManager(Loader& loader, vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice, MVP& mvp) {
    // std::vector<float> triangleVertices = {
    //     0.0f, -1.0f, 0.0f,
    //     1.0f,  1.0f, 0.0f,
    //    -1.0f,  1.0f, 0.0f,
    // };
    // std::vector<float> triangleNormals = {
    //     0.0f, 0.0f, 1.0f,
    //     0.0f, 0.0f, 1.0f,
    //     0.0f, 0.0f, 1.0f,
    // };
    // std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 1};
    //triangleEntity.emplace(loader.load(triangleVertices, indices, triangleNormals, std::nullopt, device, physicalDevice), mvp.transformation);


    ModelLoader modelLoader;
    entities.emplace_back("sponza", modelLoader.load("sponza", loader, device, physicalDevice), mvp.transformation);
    entities[entities.size() - 1].setScale(glm::vec3(0.02, 0.02, 0.02));

}

void SceneManager::updateAndDrawEntities(std::function<void()> updateTransformUniform, std::function<void(Entity&)> draw) {
    float time = glfwGetTime();
    for (Entity& entity : entities) {
        if (entity.getIdentifier() == "triangle") {
            entity.setRotation(glm::vec3(0.0f, time, 0.0f));
        } else if (entity.getIdentifier() == "sponza") {
            entity.setScale(glm::vec3(0.02, 0.02, 0.02));
        }
        entity.updateTransformationMatrix();
        updateTransformUniform();
        draw(entity);
    }
}


