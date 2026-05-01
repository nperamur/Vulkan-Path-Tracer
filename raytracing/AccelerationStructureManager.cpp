
#include "AccelerationStructureManager.h"

#include <cstdint>




AccelerationStructureManager::AccelerationStructureManager(vk::raii::Device &device,vk::raii::PhysicalDevice &physicalDevice, VmaAllocator* allocator) {
    this -> device = &device;
    this -> physicalDevice = physicalDevice;
    this -> allocator = allocator;
    this -> geometry = {};
    tlasData.emplace(AccelerationStructureData{.handle = nullptr, .buffer = nullptr, .allocation = nullptr, .deviceAddress = 0});

}




void AccelerationStructureManager::buildBLAS() {
    for (int i = 0; i < geometry.blasGeometry.size(); i++) {
        AccelerationStructureData accelStructureData = {.handle = nullptr, .buffer = nullptr, .allocation = nullptr, .deviceAddress = 0};
        buildAccelerationStructure(
            vk::AccelerationStructureTypeKHR::eBottomLevel,
            &accelStructureData.handle,
            geometry.blasGeometry[i].primitiveCount,
            geometry.blasGeometry[i].geometry,
            &accelStructureData.buffer,
            &accelStructureData.allocation,
            &accelStructureData.deviceAddress

        );
        blasData.push_back(std::move(accelStructureData));
    }
}

void AccelerationStructureManager::buildTLAS() {
    buildAccelerationStructure(
        vk::AccelerationStructureTypeKHR::eTopLevel,
        &tlasData -> handle,
        geometry.tlasGeometry.primitiveCount,
        geometry.tlasGeometry.geometry,
        &tlasData -> buffer,
        &tlasData -> allocation,
        &tlasData -> deviceAddress
    );
}

//TODO: Make storage image

void AccelerationStructureManager::buildAccelerationStructure(vk::AccelerationStructureTypeKHR accelerationStructureType,
    vk::raii::AccelerationStructureKHR* accelStructureHandle, uint32_t primitiveCount, vk::AccelerationStructureGeometryKHR& geometry,  VkBuffer* buffer, VmaAllocation* allocation, vk::DeviceAddress* deviceAddress) {
        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo(
        accelerationStructureType,
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
        vk::BuildAccelerationStructureModeKHR::eBuild,
        nullptr,
        nullptr,
        1,
        &geometry,
        nullptr,
        nullptr,
        nullptr

    );


    vk::AccelerationStructureBuildSizesInfoKHR sizeInfo = device -> getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        buildInfo,
        {primitiveCount}
    );

    //TODO: ALIGN SCRATCH BUFFER WITH minAccelerationStructureScratchOffsetAlignment
    VkPhysicalDeviceAccelerationStructurePropertiesKHR asProps{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR
    };

    VkPhysicalDeviceProperties2 props2{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
    };
    props2.pNext = &asProps;

    vkGetPhysicalDeviceProperties2(**physicalDevice, &props2);

    VkDeviceSize scratchAlign = asProps.minAccelerationStructureScratchOffsetAlignment;
    VkBuffer scratchBuffer;
    VmaAllocation scratchAllocation;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |  VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.minAlignment = scratchAlign;
    bufferInfo.size = sizeInfo.buildScratchSize + scratchAlign;
    vmaCreateBuffer(*allocator, &bufferInfo, &allocInfo, &scratchBuffer, &scratchAllocation, nullptr);
    vk::BufferDeviceAddressInfo deviceAddressInfo(
        scratchBuffer
    );
    vk::DeviceAddress address = device -> getBufferAddress(deviceAddressInfo);
    vk::DeviceAddress alignedAddress = (address + scratchAlign - 1) & ~(scratchAlign - 1);
    buildInfo.setScratchData(alignedAddress);

    VkBuffer asBuffer;
    VmaAllocation asAllocation;
    VkBufferCreateInfo asBufferInfo{};
    asBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

    asBufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VmaAllocationCreateInfo asAllocInfo{};
    asAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    asBufferInfo.size = sizeInfo.accelerationStructureSize;
    vmaCreateBuffer(*allocator, &asBufferInfo, &asAllocInfo, &asBuffer, &asAllocation, nullptr);

    vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo(
        {}, asBuffer,
        0,
        sizeInfo.accelerationStructureSize,
        accelerationStructureType
    );
    auto as = device->createAccelerationStructureKHR(accelerationStructureCreateInfo);

    buildInfo.setDstAccelerationStructure(as);

    vk::AccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo(
        as
    );
    vk::DeviceAddress asDeviceAddress = device -> getAccelerationStructureAddressKHR(asDeviceAddressInfo);


    vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, 0);
    vk::raii::CommandPool commandPool(*device, poolInfo);
    vk::CommandBufferAllocateInfo cmdAllocInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::raii::CommandBuffers commandBuffers = device -> allocateCommandBuffers(cmdAllocInfo);
    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    vk::SubmitInfo submitInfo({}, {}, *commandBuffers[0]);
    vk::Queue queue = device -> getQueue(0, 0);


    vk::AccelerationStructureBuildRangeInfoKHR rangeInfo(
        primitiveCount,
        0,
        0,
        0
    );
    commandBuffers[0].begin(beginInfo);
    commandBuffers[0].buildAccelerationStructuresKHR(buildInfo, &rangeInfo);
    commandBuffers[0].end();
    queue.submit(submitInfo);
    queue.waitIdle();

    *accelStructureHandle = std::move(as);
    vmaDestroyBuffer(*allocator, scratchBuffer, scratchAllocation);
    *deviceAddress = asDeviceAddress;
    *buffer = asBuffer;
    *allocation = asAllocation;
}



void AccelerationStructureManager::buildTLASGeometry(std::vector<AccelerationStructureData>& blasData, std::vector<Entity>& entities, MVP* mvp) {
    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    int i = 0;
    for (Entity &entity : entities) {
        entity.updateTransformationMatrix();
        glm::mat4 transposed = glm::transpose(mvp -> transformation);
        vk::TransformMatrixKHR transformMatrix;
        memcpy(&transformMatrix, &transposed, sizeof(vk::TransformMatrixKHR));
        vk::AccelerationStructureInstanceKHR asInstance(
            transformMatrix,
            i,
            0xFF,
            0,
            {},
            blasData[i].deviceAddress
        );
        instances.push_back(asInstance);
        i++;
    }


    VkBufferCreateInfo instanceBufferInfo{};
    instanceBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

    instanceBufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VmaAllocationCreateInfo instanceBufferAllocInfo{};
    instanceBufferAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    instanceBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    instanceBufferInfo.size = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
    vmaCreateBuffer(*allocator, &instanceBufferInfo, &instanceBufferAllocInfo, &instanceBuffer, &instanceAllocation, nullptr);

    VmaAllocationInfo instanceAllocInfo;
    vmaGetAllocationInfo(*allocator, instanceAllocation, &instanceAllocInfo);
    uint8_t* mapped = (uint8_t*)instanceAllocInfo.pMappedData;

    memcpy(mapped, instances.data(), instances.size() * sizeof(VkAccelerationStructureInstanceKHR));

    vk::BufferDeviceAddressInfo addressInfo(
        instanceBuffer
    );
    vk::DeviceAddress instanceAddress = device -> getBufferAddress(addressInfo);
    vk::DeviceOrHostAddressConstKHR address(instanceAddress);
    vk::AccelerationStructureGeometryInstancesDataKHR instanceGeometry(
        VK_FALSE,
         address

    );
    vk::AccelerationStructureGeometryDataKHR asGeometryData(instanceGeometry);

    vk::AccelerationStructureGeometryKHR tlasGeometry(
        vk::GeometryTypeKHR::eInstances,
        asGeometryData,
        {}

    );

    this -> geometry.tlasGeometry.geometry = tlasGeometry;
}



void AccelerationStructureManager::buildBLASGeometry(std::vector<Entity>& entities) {
    geometry.tlasGeometry.primitiveCount = entities.size();

    int i = 0;
    for (Entity& entity : entities) {
        vk::BufferDeviceAddressInfo vertexBufferDeviceAddressInfo(
            *entity.getModel().vertexBuffer
        );
        vk::DeviceAddress vertexAddress = device -> getBufferAddress(vertexBufferDeviceAddressInfo);
        vk::BufferDeviceAddressInfo indexBufferDeviceAddressInfo(
            *entity.getModel().indexBuffer
        );
        vk::DeviceAddress indexAddress = device -> getBufferAddress(indexBufferDeviceAddressInfo);

        vk::DeviceOrHostAddressConstKHR vertexData(vertexAddress);
        vk::DeviceOrHostAddressConstKHR indexData(indexAddress);
        vk::AccelerationStructureGeometryTrianglesDataKHR triangleData(
            vk::Format::eR32G32B32Sfloat,
            vertexData,
            sizeof(float) * 3,
            entity.getModel().numVertices - 1,
            vk::IndexType::eUint32,
            indexData,
            {}

        );
        vk::AccelerationStructureGeometryDataKHR asGeometryData(triangleData);
        vk::AccelerationStructureGeometryKHR triangleGeometry(
            vk::GeometryTypeKHR::eTriangles,
            asGeometryData,
            vk::GeometryFlagBitsKHR::eOpaque

        );


        GeometryData geometryData = {.geometry = triangleGeometry, .primitiveCount = (uint32_t)(entity.getModel().numIndices / 3)};
        geometry.blasGeometry.push_back(geometryData);

    }

}

void AccelerationStructureManager::cleanUp() {
    vmaDestroyBuffer(*allocator, tlasData ->buffer, tlasData -> allocation);
    for (auto& blas : blasData) {
        vmaDestroyBuffer(*allocator, blas.buffer, blas.allocation);
    }
    vmaDestroyBuffer(*allocator, instanceBuffer, instanceAllocation);
}

void AccelerationStructureManager::build(std::vector<Entity> &entities, MVP& mvp) {
    buildBLASGeometry(entities);
    buildBLAS();
    buildTLASGeometry(blasData, entities, &mvp);
    buildTLAS();

}

