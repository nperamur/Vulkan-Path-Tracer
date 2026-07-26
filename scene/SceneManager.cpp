
#include "SceneManager.h"

#include "../GLTFLoader.h"
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
//     Material teapotMaterial = { .color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0), .roughness = 0.4f, .metalness = 1.0f, .reflectivity = 0.7};
//     entities.emplace_back("utah_teapot", modelLoader.load("utah_teapot", loader, device, physicalDevice), teapotMaterial, mvp.transformation);
//     entities[entities.size() - 1].setScale(glm::vec3(0.1, 0.1, 0.1));
//
//     //Material sponzaMaterial = { .color = glm::vec4(0.7f, 0.0f, 0.0f, 1.0), .roughness = 0.5f, .metalness = 0.05f, .reflectivity = 0.04};
//     Material sponzaMaterial = { .color = glm::vec4(0.7f, 0.0f, 0.0f, 1.0), .roughness = 0.5f, .metalness = 1.0f, .reflectivity = 0.7};
//     entities.emplace_back("sponza", modelLoader.load("sponza", loader, device, physicalDevice), sponzaMaterial, mvp.transformation);
//     entities[entities.size() - 1].setScale(glm::vec3(0.02, 0.02, 0.02));
//
//     for (Entity& entity : entities) {
//         entity.setIndexAddress(device.getBufferAddress(vk::BufferDeviceAddressInfo{*entity.getModel().indexBuffer}));
//         entity.setVertexAddress(device.getBufferAddress(vk::BufferDeviceAddressInfo{*entity.getModel().vertexBuffer}));
//         entity.setNormalAddress(device.getBufferAddress(vk::BufferDeviceAddressInfo{*entity.getModel().normalBuffer}));
//         entity.updateTransformationMatrix();
//     }
//



SceneManager::SceneManager(Loader& loader, vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice, MVP& mvp, DirectionalLight& directionalLight, GLTFLoader& gltfLoader) {

    // WorldObject sponza;
    // sponza.entities = std::move(gltfLoader.load("Sponza", loader, device, physicalDevice, mvp, 15.0f, emissiveVertices, lightData));
    // sponza.setScale(glm::vec3(4.0f));
    // sponza.setPosition(glm::vec3(10, -3, 0));
    // entities.emplace_back(std::move(sponza));
    //

    // WorldObject cornellBox;
    // cornellBox.entities = std::move(gltfLoader.load("Cornell-Box", loader, device, physicalDevice, mvp, 1.0f, emissiveVertices, lightData));
    // cornellBox.setRotation(-90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    // cornellBox.setPosition(glm::vec3(0.0, -1, 0.0));
    // entities.emplace_back(std::move(cornellBox));

    WorldObject cornellBoxSpheres;
    cornellBoxSpheres.entities = std::move(gltfLoader.load("Cornell-Box-Spheres", loader, device, physicalDevice, mvp, 1.0f, emissiveVertices, lightData));
    cornellBoxSpheres.setRotation(90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    cornellBoxSpheres.setScale(glm::vec3(1.2f, 1.2f, 1.2f));
    cornellBoxSpheres.setPosition(glm::vec3(0.0, -1, 0.0));
    entities.emplace_back(std::move(cornellBoxSpheres));


    // WorldObject blocks;
    // blocks.entities = std::move(gltfLoader.load("Blocks-2", loader, device, physicalDevice, mvp, 1.0f, emissiveVertices, lightData));
    // blocks.setRotation(90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    // blocks.setPosition(glm::vec3(-250.0f, -90.0f, 150.0f));
    // entities.emplace_back(std::move(blocks));

    // WorldObject cornellBoxMirror;
    // cornellBoxMirror.entities = std::move(gltfLoader.load("Cornell-Box-Mirror", loader, device, physicalDevice, mvp, 1.0f, emissiveVertices, lightData));
    // cornellBoxMirror.setRotation(90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    // cornellBoxMirror.setScale(glm::vec3(1.2f, 1.2f, 1.2f));
    // cornellBoxMirror.setPosition(glm::vec3(0.0, -1, 0.0));
    // entities.emplace_back(std::move(cornellBoxMirror));

    directionalLight.color = LIGHT_DISABLED;
    // directionalLight.color = glm::vec4(1.0, 0.95, 0.8, 1.0);

    for (Entity* entity : getEntities()) {
        entity -> setIndexAddress(entity -> getModel().indexBuffer ? device.getBufferAddress(vk::BufferDeviceAddressInfo{*entity -> getModel().indexBuffer}) : 0);
        entity -> setVertexAddress(entity -> getModel().vertexBuffer ? device.getBufferAddress(vk::BufferDeviceAddressInfo{*entity -> getModel().vertexBuffer}) : 0);
        entity -> setNormalAddress(entity -> getModel().normalBuffer ? device.getBufferAddress(vk::BufferDeviceAddressInfo{*entity -> getModel().normalBuffer}) : 0);
        entity -> setTextureCoordsAddress(entity -> getModel().textureCoordBuffer ? device.getBufferAddress(vk::BufferDeviceAddressInfo{*entity -> getModel().textureCoordBuffer}) : 0);
        entity -> updateTransformationMatrix();
        entity -> updateEmissiveVertices();
    }
    buildCDF(emissiveVertices, triangleCDFBuffer, lightData, lightCDFBuffer);

}

// SceneManager::SceneManager(Loader& loader, vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice, MVP& mvp, DirectionalLight& directionalLight) {
//
//      entities.reserve(32);
//
//      ModelLoader modelLoader;
//
//      Material white = { .color = glm::vec4(0.73f, 0.73f, 0.73f, 1.0f), .roughness = 0.8f, .metalness = 1.0f, .reflectivity = 0.04f };
//      Material red   = { .color = glm::vec4(0.65f, 0.05f, 0.05f, 1.0f), .roughness = 0.8f, .metalness = 0.0f, .reflectivity = 0.04f };
//      Material green = { .color = glm::vec4(0.12f, 0.45f, 0.12f, 1.0f), .roughness = 0.8f, .metalness = 0.0f, .reflectivity = 0.04f };
//      Material light = { .color = glm::vec4(2.0f, 2.0f, 2.0f, 1.0f), .roughness = 0.5f, .metalness = 0.0f, .reflectivity = 0.04f };
//      Material gray  = { .color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), .roughness = 0.5f, .metalness = 1.0f, .reflectivity = 0.7f };
//
//      std::vector<float> quadVertices = {
//          -1.0f, -1.0f, 0.0f,
//           1.0f, -1.0f, 0.0f,
//           1.0f,  1.0f, 0.0f,
//          -1.0f,  1.0f, 0.0f,
//          -1.0f, -1.0f, 0.0f,
//           1.0f, -1.0f, 0.0f,
//           1.0f,  1.0f, 0.0f,
//          -1.0f,  1.0f, 0.0f
//      };
//
//      std::vector<uint32_t> indices = {
//          0, 1, 2, 0, 2, 3,
//          4, 6, 5, 4, 7, 6
//      };
//
//      std::vector<float> quadNormals = {
//          0.0f, 0.0f, 1.0f,
//          0.0f, 0.0f, 1.0f,
//          0.0f, 0.0f, 1.0f,
//          0.0f, 0.0f, 1.0f,
//          0.0f, 0.0f, -1.0f,
//          0.0f, 0.0f, -1.0f,
//          0.0f, 0.0f, -1.0f,
//          0.0f, 0.0f, -1.0f
//      };
//
//      auto floorModel = loader.load(quadVertices, indices, quadNormals, std::nullopt, device, physicalDevice);
//      entities.emplace_back(Entity("floor", std::move(floorModel), white, mvp.transformation));
//      std::get<Entity>(entities.back()).setRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
//      std::get<Entity>(entities.back()).setPosition(glm::vec3(0.0f, -1.0f, 0.0f));
//
//      auto ceilingModel = loader.load(quadVertices, indices, quadNormals, std::nullopt, device, physicalDevice);
//      entities.emplace_back(Entity("ceiling", std::move(ceilingModel), light, mvp.transformation));
//      std::get<Entity>(entities.back()).setRotation(glm::vec3(90.0f, 0.0f, 0.0f));
//      std::get<Entity>(entities.back()).setPosition(glm::vec3(0.0f, 1.0f, 0.0f));
//
//      auto backModel = loader.load(quadVertices, indices, quadNormals, std::nullopt, device, physicalDevice);
//      entities.emplace_back(Entity("back", std::move(backModel), white, mvp.transformation));
//      std::get<Entity>(entities.back()).setPosition(glm::vec3(0.0f, 0.0f, -1.0f));
//
//      auto leftModel = loader.load(quadVertices, indices, quadNormals, std::nullopt, device, physicalDevice);
//      entities.emplace_back(Entity("left", std::move(leftModel), red, mvp.transformation));
//      std::get<Entity>(entities.back()).setRotation(glm::vec3(0.0f, 90.0f, 0.0f));
//      std::get<Entity>(entities.back()).setPosition(glm::vec3(-1.0f, 0.0f, 0.0f));
//
//      auto rightModel = loader.load(quadVertices, indices, quadNormals, std::nullopt, device, physicalDevice);
//      entities.emplace_back(Entity("right", std::move(rightModel), green, mvp.transformation));
//      std::get<Entity>(entities.back()).setRotation(glm::vec3(0.0f, -90.0f, 0.0f));
//      std::get<Entity>(entities.back()).setPosition(glm::vec3(1.0f, 0.0f, 0.0f));
//
//      std::vector<float> visibleCeilingVertices = {
//          -1.0f,  1.001f,  1.0f,
//          -1.0f,  1.001f, -1.0f,
//           1.0f,  1.001f, -1.0f,
//           1.0f,  1.001f,  1.0f
//      };
//
//      std::vector<float> visibleCeilingNormals = {
//          0.0f, -1.0f, 0.0f,
//          0.0f, -1.0f, 0.0f,
//          0.0f, -1.0f, 0.0f,
//          0.0f, -1.0f, 0.0f
//      };
//
//      std::vector<uint32_t> visibleCeilingIndices = {
//          0, 1, 2,
//          0, 2, 3
//      };
//
//      auto visibleCeilingModel = loader.load(visibleCeilingVertices, visibleCeilingIndices, visibleCeilingNormals, std::nullopt, device, physicalDevice);
//      entities.emplace_back(Entity("ceiling_vis", std::move(visibleCeilingModel), white, mvp.transformation));
//
//      auto teapot = modelLoader.load("utah_teapot", loader, device, physicalDevice);
//      entities.emplace_back(Entity("teapot", std::move(teapot), gray, mvp.transformation));
//      std::get<Entity>(entities.back()).setRotation(glm::vec3(0.0f, 0, 0.0f));
//      std::get<Entity>(entities.back()).setScale(glm::vec3(0.1f));
//
//      std::vector<float> cubeVertices = {
//          -0.5f,-0.5f, 0.5f,
//           0.5f,-0.5f, 0.5f,
//           0.5f, 0.5f, 0.5f,
//          -0.5f, 0.5f, 0.5f,
//           0.5f,-0.5f,-0.5f,
//          -0.5f,-0.5f,-0.5f,
//          -0.5f, 0.5f,-0.5f,
//           0.5f, 0.5f,-0.5f,
//          -0.5f,-0.5f,-0.5f,
//          -0.5f,-0.5f, 0.5f,
//          -0.5f, 0.5f, 0.5f,
//          -0.5f, 0.5f,-0.5f,
//           0.5f,-0.5f, 0.5f,
//           0.5f,-0.5f,-0.5f,
//           0.5f, 0.5f,-0.5f,
//           0.5f, 0.5f, 0.5f,
//          -0.5f, 0.5f, 0.5f,
//           0.5f, 0.5f, 0.5f,
//           0.5f, 0.5f,-0.5f,
//          -0.5f, 0.5f,-0.5f,
//          -0.5f,-0.5f,-0.5f,
//           0.5f,-0.5f,-0.5f,
//           0.5f,-0.5f, 0.5f,
//          -0.5f,-0.5f, 0.5f,
//      };
//
//      std::vector<uint32_t> cubeIndices = {
//          0,1,2,  0,2,3,
//          4,5,6,  4,6,7,
//          8,9,10, 8,10,11,
//          12,13,14, 12,14,15,
//          16,17,18, 16,18,19,
//          20,21,22, 20,22,23
//      };
//
//      std::vector<float> cubeNormals = {
//          0,0,1,  0,0,1,  0,0,1,  0,0,1,
//          0,0,-1, 0,0,-1, 0,0,-1, 0,0,-1,
//         -1,0,0, -1,0,0, -1,0,0, -1,0,0,
//          1,0,0,  1,0,0,  1,0,0,  1,0,0,
//          0,1,0,  0,1,0,  0,1,0,  0,1,0,
//          0,-1,0, 0,-1,0, 0,-1,0, 0,-1,0
//      };
//
//      auto cube = loader.load(cubeVertices, cubeIndices, cubeNormals, std::nullopt, device, physicalDevice);
//      Material matteBlue = { .color = glm::vec4(0.2f, 0.3f, 0.9f, 1.0f), .roughness = 0.85f, .metalness = 0.0f, .reflectivity = 0.1f };
//      entities.emplace_back(Entity("cube_test", std::move(cube), matteBlue, mvp.transformation));
//      std::get<Entity>(entities.back()).setRotation(glm::vec3(0.0f, -15, 0.0f));
//      std::get<Entity>(entities.back()).setScale(glm::vec3(0.3f));
//      std::get<Entity>(entities.back()).setPosition(glm::vec3(0.4f, -0.7f, 0.2f));
//      directionalLight.color = glm::vec4(1.0, 0.95, 0.8, 1.0);
//      std::vector<float> sphereVertices;
//      std::vector<float> sphereNormals;
//      std::vector<uint32_t> sphereIndices;
//      unsigned int rings = 128;
//      unsigned int segments = 128;
//      const float PI = 3.14159265359f;
//
//      for (unsigned int r = 0; r <= rings; ++r) {
//          float phi = PI * static_cast<float>(r) / static_cast<float>(rings);
//          float sinPhi = std::sin(phi);
//          float cosPhi = std::cos(phi);
//
//          for (unsigned int s = 0; s <= segments; ++s) {
//              float theta = 2.0f * PI * static_cast<float>(s) / static_cast<float>(segments);
//              float sinTheta = std::sin(theta);
//              float cosTheta = std::cos(theta);
//
//              float x = cosTheta * sinPhi;
//              float y = cosPhi;
//              float z = sinTheta * sinPhi;
//
//              sphereVertices.push_back(x);
//              sphereVertices.push_back(y);
//              sphereVertices.push_back(z);
//              sphereNormals.push_back(x);
//              sphereNormals.push_back(y);
//              sphereNormals.push_back(z);
//          }
//      }
//
//      for (unsigned int r = 0; r < rings; ++r) {
//          for (unsigned int s = 0; s < segments; ++s) {
//              uint32_t first  = (r * (segments + 1)) + s;
//              uint32_t second = first + segments + 1;
//
//              sphereIndices.push_back(first);
//              sphereIndices.push_back(first + 1);
//              sphereIndices.push_back(second);
//
//              sphereIndices.push_back(first + 1);
//              sphereIndices.push_back(second + 1);
//              sphereIndices.push_back(second);
//          }
//      }
//
//      Material shinySpecular = { .color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), .roughness = 0.2f, .metalness = 1.0f, .reflectivity = 0.8f };
//      auto sphereModel = loader.load(sphereVertices, sphereIndices, sphereNormals, std::nullopt, device, physicalDevice);
//      entities.emplace_back(Entity("stress_sphere", std::move(sphereModel), shinySpecular, mvp.transformation));
//      std::get<Entity>(entities.back()).setScale(glm::vec3(0.25f));
//      std::get<Entity>(entities.back()).setPosition(glm::vec3(-0.4f, -0.7f, 0.1f));
//
//
//     for (Entity* entity : getEntities()) {
//         entity -> setIndexAddress(device.getBufferAddress(vk::BufferDeviceAddressInfo{*entity -> getModel().indexBuffer}));
//         entity -> setVertexAddress(device.getBufferAddress(vk::BufferDeviceAddressInfo{*entity -> getModel().vertexBuffer}));
//         entity -> setNormalAddress(device.getBufferAddress(vk::BufferDeviceAddressInfo{*entity -> getModel().normalBuffer}));
//         entity -> setTextureCoordsAddress(entity -> getModel().textureCoordBuffer ? device.getBufferAddress(vk::BufferDeviceAddressInfo{*entity -> getModel().textureCoordBuffer}) : 0);
//         entity -> updateTransformationMatrix();
//         entity -> updateEmissiveVertices();
//     }
//     buildCDF(emissiveVertices, triangleCDFBuffer, lightData, lightCDFBuffer);
//
// }


void SceneManager::updateAndDrawEntities(std::function<void()> updateTransform, std::function<void(Entity&)> draw) {
    float time = glfwGetTime();
    // for (auto& variantEntity : entities) {
    //     if (auto* o = std::get_if<WorldObject>(&variantEntity)) {
    //
    //     }
    // }
    for (Entity* entity : getEntities()) {
        if (entity -> getIdentifier() == "triangle") {
            entity -> setRotation(glm::vec3(0.0f, time, 0.0f));
        } else if (entity -> getIdentifier() == "sponza") {
            entity -> setScale(glm::vec3(0.02, 0.02, 0.02));
        }
        entity -> updateTransformationMatrix();
        updateTransform();
        draw(*entity);
    }
}

std::vector<Material> SceneManager::getAllMaterials() {
    std::vector<Material> materials;
    for (Entity* entity : getEntities()) {
        if (entity -> hasMaterial()) {
            materials.push_back(entity -> getMaterial());
        }
    }
    return materials;
}

std::vector<float> & SceneManager::getTriangleCDFBuffer() {
    return triangleCDFBuffer;
}

std::vector<float>& SceneManager::getLightCDFBuffer() {
    return lightCDFBuffer;
}

std::vector<float>& SceneManager::getEmissiveVertices() {
    return emissiveVertices;
}

std::vector<LightData>& SceneManager::getLightData() {
    return lightData;
}


void SceneManager::buildCDF(std::vector<float> &emissiveVertices, std::vector<float>& triangleCDFBuffer, std::vector<LightData>& lightData, std::vector<float>& lightCDFBuffer) {
    if (emissiveVertices.empty() || emissiveVertices.size() % 9 != 0) {
        return;
    }
    std::vector<std::array<glm::vec3, 3>> emissiveTriangles;
    emissiveTriangles.resize(emissiveVertices.size() / 9);
    memcpy(emissiveTriangles.data(), emissiveVertices.data(), emissiveVertices.size() * sizeof(float));

    float prevWeight = 0.0f;
    float prevLightWeight = 0.0f;

    float currLightArea = 0;
    int lightIndex = 0;
    int remainingStride = 0;
    std::optional<LightData> currLight = std::nullopt;
    AABB currAABB = AABB{glm::vec3(std::numeric_limits<float>::infinity()), glm::vec3(-std::numeric_limits<float>::infinity())};
    if (!lightData.empty()) {
        currLight = lightData.at(0);
        remainingStride = currLight->triangleCDFStride;
    }
    int i = 0;
    for (std::array<glm::vec3, 3>& triangle : emissiveTriangles) {
        float area = getTriangleSurfaceArea(triangle);
        float cdfWeight = prevWeight + area;
        triangleCDFBuffer.push_back(cdfWeight);
        prevWeight = cdfWeight;

        currAABB.min = glm::min(glm::min(triangle[0], glm::min(triangle[1], triangle[2])), currAABB.min);
        currAABB.max = glm::max(glm::max(triangle[0], glm::max(triangle[1], triangle[2])), currAABB.max);


        if (currLight != std::nullopt && i >= currLight->triangleCDFStartIndex) {
            currLightArea += area;
            remainingStride--;
            //handle next light
            if (remainingStride <= 0) {
                lightData[lightIndex].lightArea = currLightArea;
                lightData[lightIndex].position = (currAABB.max + currAABB.min) / 2.0f;
                lightData[lightIndex].radius = glm::length(currAABB.max - currAABB.min) / 2.0f;
                currAABB = AABB{glm::vec3(std::numeric_limits<float>::infinity()), glm::vec3(-std::numeric_limits<float>::infinity())};
                lightIndex++;
                currLight = std::nullopt;
                float lightCDFWeight = prevLightWeight + currLightArea;
                lightCDFBuffer.push_back(lightCDFWeight);
                prevLightWeight = lightCDFWeight;
                currLightArea = 0;
                if (lightIndex < lightData.size()) {
                    currLight = lightData[lightIndex];
                    remainingStride = currLight->triangleCDFStride;
                }
            }
        }
        i++;

    }

    auto triangleCDFBack = triangleCDFBuffer.back();
    for (i = 0; i < triangleCDFBuffer.size(); i++) {
        triangleCDFBuffer[i] /= triangleCDFBack;
    }

    auto lightCDFBack = lightCDFBuffer.back();
    for (i = 0; i < lightCDFBuffer.size(); i++) {
        lightCDFBuffer[i] /= lightCDFBack;
    }
}

float SceneManager::getTriangleSurfaceArea(std::array<glm::vec3, 3> vertices) {
    glm::vec3 AB = vertices[1] - vertices[0];
    glm::vec3 AC = vertices[2] - vertices[0];

    glm::vec3 crossProduct = glm::cross(AB, AC);
    return glm::length(crossProduct) / 2;
}
