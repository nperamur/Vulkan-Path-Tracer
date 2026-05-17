
#include "SceneManager.h"

#include "../ModelLoader.h"
#include "../VulkanCommon.h"
#include "GLFW/glfw3.h"

#include "glm/vec3.hpp"
#include "glm/ext/matrix_transform.hpp"
// SceneManager::SceneManager(Loader& loader, vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice, MVP& mvp) {
//     // std::vector<float> triangleVertices = {
//     //     0.0f, -1.0f, 0.0f,
//     //     1.0f,  1.0f, 0.0f,
//     //    -1.0f,  1.0f, 0.0f,
//     // };
//     // std::vector<float> triangleNormals = {
//     //     0.0f, 0.0f, 1.0f,
//     //     0.0f, 0.0f, 1.0f,
//     //     0.0f, 0.0f, 1.0f,
//     // };
//     // std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 1};
//     //triangleEntity.emplace(loader.load(triangleVertices, indices, triangleNormals, std::nullopt, device, physicalDevice), mvp.transformation);
//
//     entities.reserve(10);
//     ModelLoader modelLoader;
//     Material teapotMaterial = { .color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0), .diffuseAlbedo = 0.8};
//     entities.emplace_back("utah_teapot", modelLoader.load("utah_teapot", loader, device, physicalDevice), teapotMaterial, mvp.transformation);
//     entities[entities.size() - 1].setScale(glm::vec3(0.1, 0.1, 0.1));
//
//     Material sponzaMaterial = { .color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0), .diffuseAlbedo = 0.4};
//     entities.emplace_back("sponza", modelLoader.load("sponza", loader, device, physicalDevice), sponzaMaterial, mvp.transformation);
//     entities[entities.size() - 1].setScale(glm::vec3(0.02, 0.02, 0.02));
//
//
//
// }


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

    entities.reserve(32);

    ModelLoader modelLoader;

    Material white = { .color = glm::vec4(0.73f, 0.73f, 0.73f, 1.0f), .diffuseAlbedo = 0.8f };
    Material red   = { .color = glm::vec4(0.65f, 0.05f, 0.05f, 1.0f), .diffuseAlbedo = 0.8f };
    Material green = { .color = glm::vec4(0.12f, 0.45f, 0.12f, 1.0f), .diffuseAlbedo = 0.8f };
    Material light = { .color = glm::vec4(2.0f, 2.0f, 2.0f, 1.0f), .diffuseAlbedo = 1.0f };
    Material gray  = { .color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), .diffuseAlbedo = 0.8f };


    std::vector<float> quadVertices = {
        // Front-facing side (Indices 0-3)
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,

        // Back-facing side (Indices 4-7)
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };

    std::vector<float> quadNormals = {
        // Front-facing normals (+Z)
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,

        // Back-facing normals (-Z)
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f
    };

    // Both orientations packed together ensures coverage from any approach direction
    std::vector<uint32_t> indices = {
        0, 1, 2, 0, 2, 3, // Front side triangles
        4, 6, 5, 4, 7, 6  // Back side triangles (flipped winding order)
    };

    // floor
    auto floorModel = loader.load(quadVertices, indices, quadNormals, std::nullopt, device, physicalDevice);
    entities.emplace_back("floor", std::move(floorModel), white, mvp.transformation);
    entities.back().setRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
    entities.back().setPosition(glm::vec3(0.0f, -1.0f, 0.0f));

    // ceiling
    auto ceilingModel = loader.load(quadVertices, indices, quadNormals, std::nullopt, device, physicalDevice);
    entities.emplace_back("ceiling", std::move(ceilingModel), light, mvp.transformation);
    entities.back().setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
    entities.back().setPosition(glm::vec3(0.0f, 1.0f, 0.0f));

    // back wall
    auto backModel = loader.load(quadVertices, indices, quadNormals, std::nullopt, device, physicalDevice);
    entities.emplace_back("back", std::move(backModel), white, mvp.transformation);
    entities.back().setPosition(glm::vec3(0.0f, 0.0f, -1.0f));

    // left wall
    auto leftModel = loader.load(quadVertices, indices, quadNormals, std::nullopt, device, physicalDevice);
    entities.emplace_back("left", std::move(leftModel), red, mvp.transformation);
    entities.back().setRotation(glm::vec3(0.0f, 90.0f, 0.0f));
    entities.back().setPosition(glm::vec3(-1.0f, 0.0f, 0.0f));

    // right wall
    auto rightModel = loader.load(quadVertices, indices, quadNormals, std::nullopt, device, physicalDevice);
    entities.emplace_back("right", std::move(rightModel), green, mvp.transformation);
    entities.back().setRotation(glm::vec3(0.0f, -90.0f, 0.0f));
    entities.back().setPosition(glm::vec3(1.0f, 0.0f, 0.0f));


    // fake visible ceiling (non-emissive shell)
    std::vector<float> visibleCeilingVertices = {
        -1.0f,  1.001f,  1.0f,
        -1.0f,  1.001f, -1.0f,
         1.0f,  1.001f, -1.0f,
         1.0f,  1.001f,  1.0f
    };

    std::vector<float> visibleCeilingNormals = {
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f
    };

    std::vector<uint32_t> visibleCeilingIndices = {
        0, 1, 2,
        0, 2, 3
    };

    auto visibleCeilingModel =
        loader.load(visibleCeilingVertices, visibleCeilingIndices, visibleCeilingNormals,
                    std::nullopt, device, physicalDevice);

    entities.emplace_back("ceiling_vis",
        std::move(visibleCeilingModel),
        white,
        mvp.transformation);

    auto teapot = modelLoader.load("utah_teapot", loader, device, physicalDevice);
    entities.emplace_back("teapot", std::move(teapot), gray, mvp.transformation);
    entities.back().setScale(glm::vec3(0.1f));

    std::vector<float> cubeVertices = {
        // front
        -0.5f,-0.5f, 0.5f,
         0.5f,-0.5f, 0.5f,
         0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,

        // back
        -0.5f,-0.5f,-0.5f,
         0.5f,-0.5f,-0.5f,
         0.5f, 0.5f,-0.5f,
        -0.5f, 0.5f,-0.5f,
    };
    std::vector<uint32_t> cubeIndices = {
        0,1,2, 0,2,3,
        1,5,6, 1,6,2,
        5,4,7, 5,7,6,
        4,0,3, 4,3,7,
        3,2,6, 3,6,7,
        4,5,1, 4,1,0
    };
    std::vector<float> cubeNormals = {
        // front
        0,0,1, 0,0,1, 0,0,1, 0,0,1,

        // back
        0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1
    };
    auto cube = loader.load(cubeVertices, cubeIndices, cubeNormals,
                            std::nullopt, device, physicalDevice);

    Material matteBlue = {
        .color = glm::vec4(0.2f, 0.3f, 0.9f, 1.0f),
        .diffuseAlbedo = 0.9f
    };

    entities.emplace_back("cube_test", std::move(cube), matteBlue, mvp.transformation);
    entities.back().setPosition(glm::vec3(0.4f, -0.7f, 0.2f));

    entities.back().setScale(glm::vec3(0.3f));
    entities.back().setPosition(glm::vec3(0.4f, -0.7f, 0.2f));


    std::vector<float> sphereVertices;
    std::vector<float> sphereNormals;
    std::vector<uint32_t> sphereIndices;

    unsigned int rings = 128;
    unsigned int segments = 128;

    const float PI = 3.14159265359f;

    for (unsigned int r = 0; r <= rings; ++r) {
        float phi = PI * static_cast<float>(r) / static_cast<float>(rings);
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (unsigned int s = 0; s <= segments; ++s) {
            float theta = 2.0f * PI * static_cast<float>(s) / static_cast<float>(segments);
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            float x = cosTheta * sinPhi;
            float y = cosPhi;
            float z = sinTheta * sinPhi;

            // Positions
            sphereVertices.push_back(x);
            sphereVertices.push_back(y);
            sphereVertices.push_back(z);

            // Vertex Normals
            sphereNormals.push_back(x);
            sphereNormals.push_back(y);
            sphereNormals.push_back(z);
        }
    }

    for (unsigned int r = 0; r < rings; ++r) {
        for (unsigned int s = 0; s < segments; ++s) {
            uint32_t first = (r * (segments + 1)) + s;
            uint32_t second = first + segments + 1;

            sphereIndices.push_back(first);
            sphereIndices.push_back(second);
            sphereIndices.push_back(first + 1);

            sphereIndices.push_back(second);
            sphereIndices.push_back(second + 1);
            sphereIndices.push_back(first + 1);
        }
    }

    Material shinySpecular = {
        .color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
        .diffuseAlbedo = 0.2f
    };

    auto sphereModel = loader.load(sphereVertices, sphereIndices, sphereNormals,
                                  std::nullopt, device, physicalDevice);

    entities.emplace_back("stress_sphere", std::move(sphereModel), shinySpecular, mvp.transformation);

    entities.back().setScale(glm::vec3(0.25f));
    entities.back().setPosition(glm::vec3(-0.4f, -0.7f, 0.2f));



    // auto sponza = modelLoader.load("sponza", loader, device, physicalDevice);
    // entities.emplace_back("sponza", std::move(sponza), white, mvp.transformation);
    // entities.back().setScale(glm::vec3(0.02f));
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


