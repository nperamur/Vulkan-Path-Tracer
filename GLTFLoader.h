

#ifndef VULKAN_PATHTRACER_GLTFLOADER_H
#define VULKAN_PATHTRACER_GLTFLOADER_H
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include "Entity.h"
#include "Loader.h"
#include <glm/gtc/type_ptr.hpp>

#include "VulkanCommon.h"
#include "image-views/TextureBufferManager.h"

class GLTFLoader {

    TextureBufferManager& textureBufferManager;
    long numBaseColors = 0;

    public:

    GLTFLoader(TextureBufferManager& textureBufferManager);
    std::vector<Entity> load(std::string name, Loader &loader, vk::raii::Device &device, vk::raii::PhysicalDevice &physicalDevice, MVP& mvp, float albedoMultiplier, std::vector<float>& emissiveVertices,
                                std::vector<LightData>& lightData);


    void processNodes(fastgltf::Asset &gltf, std::vector<Entity> &entities,
        const fastgltf::pmr::MaybeSmallVector<unsigned long long> &nodeIndices, glm::mat4 transform, std::string name, Loader &loader, vk::raii::Device &device, vk::raii::PhysicalDevice &physicalDevice,
        MVP& mvp, float albedoMultiplier, std::vector<float>& emissiveVertices, std::vector<LightData>& lightData);


};









#endif //VULKAN_PATHTRACER_GLTFLOADER_H
