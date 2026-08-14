#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GLTFLoader.h"

#include "VulkanCommon.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Application.h"
#include "stb_image.h"

GLTFLoader::GLTFLoader(TextureBufferManager &textureBufferManager) : textureBufferManager(textureBufferManager) {}

std::vector<Entity> GLTFLoader::load(std::string name, Loader &loader, vk::raii::Device &device,
                                     vk::raii::PhysicalDevice &physicalDevice, MVP& mvp, float albedoMultiplier, float lightMultiplier, std::vector<float>& emissiveVertices, std::vector<LightData>& lightData) {
    std::string fullPathStr = "resources/GLTFModels/" + name + "/" + name + ".gltf";
    std::filesystem::path gltfPath(fullPathStr);


    auto mappedData = fastgltf::MappedGltfFile::FromPath(gltfPath);
    if (mappedData.error() != fastgltf::Error::None) {
        throw new std::exception("Cannot load gltf file");
    }
    fastgltf::Parser parser(fastgltf::Extensions::KHR_materials_specular | fastgltf::Extensions::KHR_materials_ior | fastgltf::Extensions::KHR_materials_emissive_strength | fastgltf::Extensions::KHR_materials_transmission);
    auto asset = parser.loadGltf(mappedData.get(), gltfPath.parent_path(), fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages);
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
    processNodes(gltf, entities, nodeIndices, initialTransform, name, loader, device, physicalDevice, mvp, albedoMultiplier, lightMultiplier, emissiveVertices, lightData);


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
    MVP& mvp, float albedoMultiplier, float lightMultiplier, std::vector<float>& emissiveVertices, std::vector<LightData>& lightData) {

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
        processNodes(gltf, entities, node.children, totalTransform, name, loader, device, physicalDevice, mvp, albedoMultiplier, lightMultiplier, emissiveVertices, lightData);

        if (node.meshIndex.has_value()) {
            const fastgltf::Mesh& mesh = gltf.meshes[node.meshIndex.value()];
            int i = 0;
            for (auto& primitive : mesh.primitives) {
                std::string primitiveName = nodeName + "_primitive_" + std::to_string(i);

                long materialIndex = *primitive.materialIndex;
                Material material;
                material.roughness = gltf.materials[materialIndex].pbrData.roughnessFactor;
                material.metalness = gltf.materials[materialIndex].pbrData.metallicFactor;
                material.color = glm::make_vec4(gltf.materials[materialIndex].pbrData.baseColorFactor.data());
                material.albedoFactor = albedoMultiplier;
                //TODO: combine specular color factor with specular factor

                float ior = gltf.materials[materialIndex].ior;
                float f0Base = std::pow((ior - 1.0f) / (ior + 1.0f), 2.0f);
                float specularFactor = 1.0f;
                float specularColorAvg = 1.0f;
                if (gltf.materials[materialIndex].specular) {
                    specularFactor = gltf.materials[materialIndex].specular->specularFactor;
                    if (auto scfPtr = &gltf.materials[materialIndex].specular->specularColorFactor) {
                        const auto& scf = *scfPtr;
                        specularColorAvg = (scf[0] + scf[1] + scf[2]) / 3.0f;
                    }
                }
                material.reflectivity = f0Base * specularFactor * specularColorAvg;
                material.ior = ior;
                if (gltf.materials[materialIndex].transmission) {
                    material.transmissionFactor = gltf.materials[materialIndex].transmission -> transmissionFactor;
                }

                if (gltf.materials[materialIndex].pbrData.baseColorTexture.has_value()) {
                    auto& texture = gltf.textures[gltf.materials[materialIndex].pbrData.baseColorTexture -> textureIndex];
                    if (texture.imageIndex.has_value()) {
                        auto& image = gltf.images[texture.imageIndex.value()];
                        uploadTexture(image, TextureBuffers::baseColor,gltf, name, device, &material.baseColorTextureIndex);
                    }
                }

                if (gltf.materials[materialIndex].normalTexture.has_value()) {
                    auto& texture = gltf.textures[gltf.materials[materialIndex].normalTexture -> textureIndex];
                    if (texture.imageIndex.has_value()) {
                        auto& image = gltf.images[texture.imageIndex.value()];
                        uploadTexture(image, TextureBuffers::normalMap,gltf, name, device, &material.normalMapTextureIndex);
                    }
                }
                imageCounter++;


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
                entity.updateTransformationMatrix();
                i++;

                if (isEmissive) {
                    LightData currLightData;
                    currLightData.emissionFactor = glm::make_vec3(gltf.materials[materialIndex].emissiveFactor.data()) * lightMultiplier * gltf.materials[materialIndex].emissiveStrength;
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

void GLTFLoader::uploadTexture(fastgltf::Image image, std::string textureBuffer, fastgltf::Asset &gltf, std::string name, vk::raii::Device &device, int* textureIndexPtr) {
    std::filesystem::path gltfDir = "resources/GLTFModels/" + name + "/";
    void* imageBytes;
    size_t imageSize;
    std::vector<std::byte> imageVec;
    std::visit(fastgltf::visitor {
        [&](auto&) {},
        [&](const fastgltf::sources::URI& uri) {
            std::filesystem::path fullPath = gltfDir / std::filesystem::path(uri.uri.path());
            std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                size_t size = file.tellg();
                file.seekg(0);
                imageVec.resize(size);
                file.read(reinterpret_cast<char*>(imageVec.data()), size);
                imageSize = size;
                imageBytes = imageVec.data();
            } else {
                std::string filename(uri.uri.path());
                throw std::runtime_error("Cannot load file: " + filename);
            }
        }, [&](const fastgltf::sources::BufferView bufferView) {
            auto& view = gltf.bufferViews[bufferView.bufferViewIndex];
            auto& buffer = gltf.buffers[view.bufferIndex];
            imageSize = buffer.byteLength;
            imageVec.resize(imageSize);
            std::visit(fastgltf::visitor {
                [&](auto&) {},
                    [&](fastgltf::sources::Array& bufArr) {
                        memcpy(imageVec.data(), bufArr.bytes.data() + view.byteOffset, imageSize);
                        imageBytes = bufArr.bytes.data() + view.byteOffset;
                    },
                    [&](fastgltf::sources::Vector& bufVec) {
                        memcpy(imageVec.data(), bufVec.bytes.data() + view.byteOffset, imageSize);
                        imageBytes = bufVec.bytes.data() + view.byteOffset;
                    }
                }, buffer.data
            );
        }, [&](fastgltf::sources::Vector vector) {
            imageVec.resize(imageSize);
            memcpy(imageVec.data(), vector.bytes.data(), imageSize);
            imageBytes = imageVec.data();
            imageSize = vector.bytes.size();
        },
        [&](fastgltf::sources::Array array) {
            imageSize = array.bytes.size();
            imageVec.resize(imageSize);
            memcpy(imageVec.data(), array.bytes.data(), imageSize);
            imageBytes = imageVec.data();
        }

    }, image.data);

    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(imageBytes),
        static_cast<int>(imageSize),
        &width, &height, &channels,
        4 //RGBA8
    );
    size_t pixelsSize = static_cast<size_t>(width) * height * 4;

    if (!pixels) {
        throw std::runtime_error("failed to parse texture: " + std::string(stbi_failure_reason()));
        return;
    }

    std::string imageId = textureBuffer + std::to_string(imageCounter);
    int textureIndex = textureBufferManager.getTextureViews(textureBuffer).size();
    *textureIndexPtr = textureIndex;
    textureBufferManager.registerImage(imageId, textureBuffer, VK_FORMAT_R8G8B8A8_SRGB, width, height);
    VmaAllocator allocator = Application::get() -> getMemoryAllocator();

    VkBuffer buffer;
    VmaAllocation allocation;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = pixelsSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo resultInfo;
    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &resultInfo);

    memcpy(resultInfo.pMappedData, pixels, pixelsSize);
    vmaFlushAllocation(allocator, allocation, 0, VK_WHOLE_SIZE);
    stbi_image_free(pixels);
    vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, 0);
    vk::raii::CommandPool commandPool(device, poolInfo);
    vk::CommandBufferAllocateInfo cmdAllocInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::raii::CommandBuffers commandBuffers = device.allocateCommandBuffers(cmdAllocInfo);
    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    vk::SubmitInfo submitInfo({}, {}, *commandBuffers[0]);
    vk::Queue queue = device.getQueue(0, 0);
    queue.waitIdle();
    commandBuffers[0].begin(beginInfo);
    Image* textureImage = &textureBufferManager.getImage(imageId);
    vk::BufferImageCopy2 copyInfo{};
    copyInfo.bufferOffset = 0;
    copyInfo.bufferRowLength = 0;
    copyInfo.bufferImageHeight = 0;

    copyInfo.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    copyInfo.imageSubresource.mipLevel = 0;
    copyInfo.imageSubresource.baseArrayLayer = 0;
    copyInfo.imageSubresource.layerCount = 1;

    copyInfo.imageOffset = vk::Offset3D{0, 0, 0};
    copyInfo.imageExtent = vk::Extent3D{
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        1
    };
    vk::ImageSubresourceRange subresourceRange(
        vk::ImageAspectFlagBits::eColor,
        0, 1, 0, 1
    );

    vk::ImageMemoryBarrier2 barrier(
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        0,
        0,
        textureImage->getImage(),
        subresourceRange
    );

    vk::DependencyInfo depInfo(
        {},
        {},
        {},
        barrier
    );

    commandBuffers[0].pipelineBarrier2(depInfo);
    vk::CopyBufferToImageInfo2 copyBufferToImageInfo = vk::CopyBufferToImageInfo2(
        buffer,
        textureImage -> getImage(),
        vk::ImageLayout::eTransferDstOptimal,
        1,
        &copyInfo
    );
    commandBuffers[0].copyBufferToImage2(copyBufferToImageInfo);
    commandBuffers[0].end();
    queue.submit(submitInfo);
    queue.waitIdle();
    vmaDestroyBuffer(allocator, buffer, allocation);
}

