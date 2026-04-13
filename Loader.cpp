#include "Loader.h"

#include <iostream>
#include <optional>

Model Loader::load(std::vector<float> vertices, std::optional<std::vector<uint32_t>> indices, std::optional<std::vector<float>> normals, vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice) {
    numVertices = vertices.size();

    size_t maxSize = vertices.size();
    if (indices.has_value()) maxSize = std::max(maxSize, indices->size());
    if (normals.has_value()) maxSize = std::max(maxSize, normals->size());

    vk::BufferCreateInfo stagingBufferCreateInfo(
        {},
        maxSize * sizeof(float),
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
    vk::BufferCreateInfo vertexBufferCreateInfo({}, vertices.size() * sizeof(float),
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive);
    vertexBuffer.emplace(device, vertexBufferCreateInfo);
    auto vertexMemReqs = vertexBuffer->getMemoryRequirements();
    for (int i = 0; i < memProps.memoryTypeCount; i++) {
        if ((vertexMemReqs.memoryTypeBits & (1 << i)) != 0 && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) {
            vertexMemory.emplace(device, vk::MemoryAllocateInfo(vertexMemReqs.size, i));
            break;
        }
    }
    vertexBuffer->bindMemory(**vertexMemory, 0);
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
            vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive);
        indexBuffer.emplace(device, indexBufferCreateInfo);
        auto indexMemReqs = indexBuffer->getMemoryRequirements();
        for (int i = 0; i < memProps.memoryTypeCount; i++) {
            if ((indexMemReqs.memoryTypeBits & (1 << i)) != 0 && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) {
                indexMemory.emplace(device, vk::MemoryAllocateInfo(indexMemReqs.size, i));
                break;
            }
        }
        indexBuffer->bindMemory(**indexMemory, 0);
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
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive);
        normalBuffer.emplace(device, normalBufferCreateInfo);
        auto normalMemReqs = normalBuffer->getMemoryRequirements();
        for (int i = 0; i < memProps.memoryTypeCount; i++) {
            if ((normalMemReqs.memoryTypeBits & (1 << i)) != 0 && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) {
                normalMemory.emplace(device, vk::MemoryAllocateInfo(normalMemReqs.size, i));
                break;
            }
        }
        normalBuffer->bindMemory(**normalMemory, 0);
        data = stagingMemory->mapMemory(0, stagingMemReqs.size);
        memcpy(data, normals->data(), normals->size() * sizeof(float));
        stagingMemory->unmapMemory();
        commandBuffers[0].begin(beginInfo);
        commandBuffers[0].copyBuffer(*stagingBuffer, *normalBuffer, vk::BufferCopy(0, 0, normals->size() * sizeof(float)));
        commandBuffers[0].end();
        queue.submit(submitInfo);
        queue.waitIdle();
    }

    return Model {
        .vertexBuffer = std::move(vertexBuffer),
        .numVertices = numVertices,
        .indexBuffer = std::move(indexBuffer),
        .numIndices = numIndices,
        .normalBuffer = std::move(normalBuffer)
    };
}