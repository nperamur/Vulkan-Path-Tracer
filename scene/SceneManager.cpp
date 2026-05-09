
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

    entities.reserve(10);
    ModelLoader modelLoader;
    Material teapotMaterial = { glm::vec4(0.0f, 1.0f, 0.0f, 1.0) };
    entities.emplace_back("utah_teapot", modelLoader.load("utah_teapot", loader, device, physicalDevice), teapotMaterial, mvp.transformation);
    entities[entities.size() - 1].setScale(glm::vec3(0.1, 0.1, 0.1));

    Material sponzaMaterial = { glm::vec4(1.0f, 0.0f, 0.0f, 1.0) };
    entities.emplace_back("sponza", modelLoader.load("sponza", loader, device, physicalDevice), sponzaMaterial, mvp.transformation);
    entities[entities.size() - 1].setScale(glm::vec3(0.02, 0.02, 0.02));



}

void SceneManager::updateAndDrawEntities(std::function<void()> updateTransform, std::function<void(Entity&)> draw) {
    float time = glfwGetTime();
    for (Entity& entity : entities) {
        if (entity.getIdentifier() == "triangle") {
            entity.setRotation(glm::vec3(0.0f, time, 0.0f));
        } else if (entity.getIdentifier() == "sponza") {
            entity.setScale(glm::vec3(0.02, 0.02, 0.02));
        }
        entity.updateTransformationMatrix();
        updateTransform();
        draw(entity);
    }
}

std::vector<Material> SceneManager::getAllMaterials() {
    std::vector<Material> materials;
    for (Entity& entity : entities) {
        if (entity.hasMaterial()) {
            materials.push_back(entity.getMaterial());
        }
    }
    return std::move(materials);
}


