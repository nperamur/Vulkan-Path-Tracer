#include "Loader.h"

#include <iostream>
#include <optional>

Model Loader::load(std::vector<float> vertices, std::optional<std::vector<uint32_t>> indices, std::optional<std::vector<float>> normals, std::optional<std::vector<float>> textureCoords, vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice) {
    Model model;
    numVertices = vertices.size() / 3;

    size_t maxBytes = vertices.size() * sizeof(float);
    if (indices.has_value()) maxBytes = std::max(maxBytes, indices->size() * sizeof(uint32_t));
    if (normals.has_value()) maxBytes = std::max(maxBytes, normals->size() * sizeof(float));
    if (textureCoords.has_value()) maxBytes = std::max(maxBytes, textureCoords->size() * sizeof(float));

    vk::BufferCreateInfo stagingBufferCreateInfo(
        {},
        maxBytes,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive
    );

    stagingBuffer.emplace(device, stagingBufferCreateInfo);
    auto stagingMemReqs = stagingBuffer->getMemoryRequirements();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    for (int i = 0; i < memProps.memoryTypeCount; i++) {
        if ((stagingMemReqs.memoryTypeBits & (1 << i)) != 0 && (memProps.memoryTypes[i].propertyFlags &
            (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent))) {
            vk::MemoryAllocateInfo memoryInfo(stagingMemReqs.size, i);
            stagingMemory.emplace(device, memoryInfo);
            break;
        }
    }

    stagingBuffer->bindMemory(**stagingMemory, 0);

    vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, 0);
    vk::raii::CommandPool commandPool(device, poolInfo);
    vk::CommandBufferAllocateInfo allocInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::raii::CommandBuffers commandBuffers = device.allocateCommandBuffers(allocInfo);
    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    vk::SubmitInfo submitInfo({}, {}, *commandBuffers[0]);
    vk::Queue queue = device.getQueue(0, 0);

    // vertex buffer
    vk::MemoryAllocateFlagsInfo memFlagsInfo(
        vk::MemoryAllocateFlagBits::eDeviceAddress
    );
    vk::BufferCreateInfo vertexBufferCreateInfo({}, vertices.size() * sizeof(float),
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, vk::SharingMode::eExclusive);
    vertexBuffer.emplace(device, vertexBufferCreateInfo);
    auto vertexMemReqs = vertexBuffer->getMemoryRequirements();
    for (int i = 0; i < memProps.memoryTypeCount; i++) {
        if ((vertexMemReqs.memoryTypeBits & (1 << i)) != 0 && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) {
            vertexMemory.emplace(device, vk::MemoryAllocateInfo(vertexMemReqs.size, i, &memFlagsInfo));
            break;
        }
    }
    model.vertexMemory = std::move(vertexMemory);
    vertexBuffer->bindMemory(**model.vertexMemory, 0);
    void* data = stagingMemory->mapMemory(0, stagingMemReqs.size);
    memcpy(data, vertices.data(), vertices.size() * sizeof(float));
    stagingMemory->unmapMemory();
    commandBuffers[0].begin(beginInfo);
    commandBuffers[0].copyBuffer(*stagingBuffer, *vertexBuffer, vk::BufferCopy(0, 0, vertices.size() * sizeof(float)));
    commandBuffers[0].end();
    queue.submit(submitInfo);
    queue.waitIdle();

    // index buffer
    int numIndices = indices.has_value() ? indices->size() : 0;
    if (indices.has_value()) {
        vk::BufferCreateInfo indexBufferCreateInfo({}, indices->size() * sizeof(uint32_t),
            vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, vk::SharingMode::eExclusive);
        indexBuffer.emplace(device, indexBufferCreateInfo);
        auto indexMemReqs = indexBuffer->getMemoryRequirements();
        for (int i = 0; i < memProps.memoryTypeCount; i++) {
            if ((indexMemReqs.memoryTypeBits & (1 << i)) != 0 && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) {
                indexMemory.emplace(device, vk::MemoryAllocateInfo(indexMemReqs.size, i, &memFlagsInfo));
                break;
            }
        }
        model.indexMemory = std::move(indexMemory);
        indexBuffer->bindMemory(**model.indexMemory, 0);
        data = stagingMemory->mapMemory(0, stagingMemReqs.size);
        memcpy(data, indices->data(), indices->size() * sizeof(uint32_t));
        stagingMemory->unmapMemory();
        commandBuffers[0].begin(beginInfo);
        commandBuffers[0].copyBuffer(*stagingBuffer, *indexBuffer, vk::BufferCopy(0, 0, indices->size() * sizeof(uint32_t)));
        commandBuffers[0].end();
        queue.submit(submitInfo);
        queue.waitIdle();
    }

    // normal buffer
    if (normals.has_value()) {
        vk::BufferCreateInfo normalBufferCreateInfo({}, normals->size() * sizeof(float),
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress, vk::SharingMode::eExclusive);
        normalBuffer.emplace(device, normalBufferCreateInfo);
        auto normalMemReqs = normalBuffer->getMemoryRequirements();
        for (int i = 0; i < memProps.memoryTypeCount; i++) {
            if ((normalMemReqs.memoryTypeBits & (1 << i)) != 0 && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) {
                normalMemory.emplace(device, vk::MemoryAllocateInfo(normalMemReqs.size, i, memFlagsInfo));
                break;
            }
        }
        model.normalMemory = std::move(normalMemory);
        normalBuffer->bindMemory(**model.normalMemory, 0);
        data = stagingMemory->mapMemory(0, stagingMemReqs.size);
        memcpy(data, normals->data(), normals->size() * sizeof(float));
        stagingMemory->unmapMemory();
        commandBuffers[0].begin(beginInfo);
        commandBuffers[0].copyBuffer(*stagingBuffer, *normalBuffer, vk::BufferCopy(0, 0, normals->size() * sizeof(float)));
        commandBuffers[0].end();
        queue.submit(submitInfo);
        queue.waitIdle();
    }



    if (textureCoords.has_value()) {
        vk::BufferCreateInfo textureCoordCreateInfo({}, textureCoords->size() * sizeof(float),
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress, vk::SharingMode::eExclusive);
        textureCoordBuffer.emplace(device, textureCoordCreateInfo);
        auto textureCoordMemReqs = textureCoordBuffer->getMemoryRequirements();
        for (int i = 0; i < memProps.memoryTypeCount; i++) {
            if ((textureCoordMemReqs.memoryTypeBits & (1 << i)) != 0 && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) {
                textureCoordMemory.emplace(device, vk::MemoryAllocateInfo(textureCoordMemReqs.size, i, memFlagsInfo));
                break;
            }
        }
        model.textureCoordMemory = std::move(textureCoordMemory);
        textureCoordBuffer->bindMemory(**model.textureCoordMemory, 0);
        data = stagingMemory->mapMemory(0, stagingMemReqs.size);
        memcpy(data, textureCoords->data(), textureCoords->size() * sizeof(float));
        stagingMemory->unmapMemory();
        commandBuffers[0].begin(beginInfo);
        commandBuffers[0].copyBuffer(*stagingBuffer, *textureCoordBuffer, vk::BufferCopy(0, 0, textureCoords->size() * sizeof(float)));
        commandBuffers[0].end();
        queue.submit(submitInfo);
        queue.waitIdle();
    }

    model.vertexBuffer = std::move(vertexBuffer);
    model.numVertices = numVertices;
    model.indexBuffer = std::move(indexBuffer);
    model.numIndices = numIndices;
    model.normalBuffer = std::move(normalBuffer);
    model.textureCoordBuffer = std::move(textureCoordBuffer);

    return model;
}