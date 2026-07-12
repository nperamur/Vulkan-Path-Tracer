#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GLTFLoader.h"

#include "VulkanCommon.h"

std::vector<Entity> GLTFLoader::load(std::string name, Loader &loader, vk::raii::Device &device,
                                     vk::raii::PhysicalDevice &physicalDevice, MVP& mvp, float albedoMultiplier, std::vector<float>& emissiveVertices, std::vector<LightData>& lightData) {
    std::string fullPathStr = "resources/GLTFModels/" + name + "/" + name + ".gltf";
    std::filesystem::path gltfPath(fullPathStr);


    auto mappedData = fastgltf::MappedGltfFile::FromPath(gltfPath);
    if (mappedData.error() != fastgltf::Error::None) {
        throw new std::exception("Cannot load gltf file");
    }
    fastgltf::Parser parser;
    auto asset = parser.loadGltf(mappedData.get(), gltfPath.parent_path(), fastgltf::Options::LoadExternalBuffers);
    if (asset.error() != fastgltf::Error::None) {
        std::string msg = std::string(fastgltf::getErrorMessage(asset.error()));
        throw std::runtime_error("Cannot load gltf file: " + msg);
    }
    fastgltf::Asset& gltf = asset.get();


    size_t activeSceneIdx = gltf.defaultScene.value_or(0);

    const auto& scene = gltf.scenes[activeSceneIdx];



    std::vector<Entity> entities;
    glm::mat4 initialTransform = glm::mat4(1.0);
    fastgltf::pmr::MaybeSmallVector<unsigned long long> nodeIndices(scene.nodeIndices.begin(), scene.nodeIndices.end());
    processNodes(gltf, entities, nodeIndices, initialTransform, name, loader, device, physicalDevice, mvp, albedoMultiplier, emissiveVertices, lightData);


    return entities;




}

void GLTFLoader::processNodes(
    fastgltf::Asset &gltf,
    std::vector<Entity> &entities,
    const fastgltf::pmr::MaybeSmallVector<unsigned long long> &nodeIndices,
    glm::mat4 parentTransform,
    std::string name,
    Loader &loader,
    vk::raii::Device &device,
    vk::raii::PhysicalDevice &physicalDevice,
    MVP& mvp, float albedoMultiplier, std::vector<float>& emissiveVertices, std::vector<LightData>& lightData) {
    for (long nodeIndex : nodeIndices) {
        const fastgltf::Node& node = gltf.nodes[nodeIndex];

        std::string nodeName;
        if (!node.name.empty()) {
            nodeName = node.name.c_str();
        } else {
            nodeName = name + "_Node_" + std::to_string(nodeIndex);
        }

        fastgltf::math::fmat4x4 fastMat = fastgltf::getTransformMatrix(node);
        glm::mat4 nodeTransform = glm::make_mat4(fastMat.data());
        glm::mat4 totalTransform = parentTransform * nodeTransform;
        processNodes(gltf, entities, node.children, totalTransform, name, loader, device, physicalDevice, mvp, albedoMultiplier, emissiveVertices, lightData);

        if (node.meshIndex.has_value()) {
            const fastgltf::Mesh& mesh = gltf.meshes[node.meshIndex.value()];
            int i = 0;
            for (auto& primitive : mesh.primitives) {
                std::string primitiveName = nodeName + "_primitive_" + std::to_string(i);

                long materialIndex = *primitive.materialIndex;
                Material material;
                material.roughness = gltf.materials[materialIndex].pbrData.roughnessFactor;
                material.metalness = gltf.materials[materialIndex].pbrData.metallicFactor;
                material.color = glm::make_vec4(gltf.materials[materialIndex].pbrData.baseColorFactor.data()) * albedoMultiplier;
                //TODO: combine specular color factor with specular factor

                if (gltf.materials[materialIndex].specular) {
                    material.reflectivity = gltf.materials[materialIndex].specular->specularFactor;
                }


                //glm::vec3 specColorFactor = glm::make_vec3(gltf.materials[materialIndex].specular.get() -> specularColorFactor.data());



                if (!primitive.indicesAccessor.has_value()) {
                    continue;
                }
                auto& indexAccessor = gltf.accessors[primitive.indicesAccessor.value()];
                std::vector<uint32_t> indices;
                indices.resize(indexAccessor.count);
                fastgltf::copyFromAccessor<uint32_t>(gltf, indexAccessor, indices.data());

                std::vector<float> flatPositions;
                std::vector<float> flatNormals;
                std::vector<float> flatUvs;
                auto posAttr = primitive.findAttribute("POSITION");
                auto normalAttr = primitive.findAttribute("NORMAL");
                auto uvAttr  = primitive.findAttribute("TEXCOORD_0");

                if (posAttr != nullptr && posAttr != primitive.attributes.end()) {
                    auto& accessor = gltf.accessors[posAttr->accessorIndex];
                    flatPositions.resize(accessor.count * 3);
                    auto* destPtr = reinterpret_cast<fastgltf::math::fvec3*>(flatPositions.data());

                    fastgltf::copyFromAccessor<fastgltf::math::fvec3>(gltf, accessor, destPtr);
                }

                if (normalAttr != nullptr && normalAttr != primitive.attributes.end()) {
                    auto& accessor = gltf.accessors[normalAttr->accessorIndex];
                    flatNormals.resize(accessor.count * 3);
                    auto* destPtr = reinterpret_cast<fastgltf::math::fvec3*>(flatNormals.data());

                    fastgltf::copyFromAccessor<fastgltf::math::fvec3>(gltf, accessor, destPtr);
                }

                if (uvAttr != nullptr && uvAttr != primitive.attributes.end()) {
                    auto& accessor = gltf.accessors[uvAttr->accessorIndex];
                    flatUvs.resize(accessor.count * 2);
                    auto* destPtr = reinterpret_cast<fastgltf::math::fvec2*>(flatUvs.data());

                    fastgltf::copyFromAccessor<fastgltf::math::fvec2>(gltf, accessor, destPtr);
                }


                // Entity entity = Entity(
                //     primitiveName,
                //     ,
                //     material,
                //
                // );
                Model model = loader.load(
                    flatPositions,
                    indices,
                    flatNormals.empty() ? std::nullopt : std::optional(flatNormals),
                    flatUvs.empty() ? std::nullopt : std::optional(flatUvs),
                    device, physicalDevice
                );
                //TODO: Later, make a World Object Class that contains all these entities for better single transform stuff

                bool isEmissive = (gltf.materials[materialIndex].emissiveFactor[0] > 0.0f ||
                   gltf.materials[materialIndex].emissiveFactor[1] > 0.0f ||
                   gltf.materials[materialIndex].emissiveFactor[2] > 0.0f);

                if (isEmissive) {
                    material.lightIndex = lightData.size();
                }
                Entity entity = Entity(primitiveName, std::move(model), material, mvp.transformation);
                entity.setTransform(totalTransform);
                i++;

                if (isEmissive) {
                    LightData currLightData;
                    currLightData.emissionFactor = glm::make_vec3(gltf.materials[materialIndex].emissiveFactor.data());
                    currLightData.materialIndex = static_cast<int>(entities.size());
                    currLightData.triangleCDFStartIndex = static_cast<int>(emissiveVertices.size() / 9);
                    currLightData.triangleCDFStride = static_cast<int>(indices.size() / 3);
                    currLightData.position = glm::vec3(0.0f);
                    entity.setEmission(&emissiveVertices, emissiveVertices.size(), indices.size() * 3);
                    for (uint32_t index : indices) {
                        emissiveVertices.push_back(flatPositions[index * 3]);
                        emissiveVertices.push_back(flatPositions[index * 3 + 1]);
                        emissiveVertices.push_back(flatPositions[index * 3 + 2]);
                    }

                    lightData.push_back(currLightData);

                    //emissiveVertices.insert(emissiveVertices.end(), flatPositions.begin(), flatPositions.end());
                }

                entities.push_back(std::move(entity));

            }
        }

    }

}
