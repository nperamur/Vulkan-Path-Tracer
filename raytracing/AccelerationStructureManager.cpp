
#include "AccelerationStructureManager.h"
#include <deque>


#include <cstdint>
/**
 * @Author Neelesh Peramur
 * This class owns and manages the lifecycle of ray-tracing acceleration structures in the engine
 */



AccelerationStructureManager::AccelerationStructureManager(vk::raii::Device &device,vk::raii::PhysicalDevice &physicalDevice, VmaAllocator* allocator) {
    this -> device = &device;
    this -> physicalDevice = physicalDevice;
    this -> allocator = allocator;
    this -> geometry = {};
    tlasData.emplace(AccelerationStructureData{.handle = nullptr, .buffer = nullptr, .allocation = nullptr, .deviceAddress = 0});

}


void AccelerationStructureManager::createScratchBuffers(
    std::vector<vk::AccelerationStructureBuildGeometryInfoKHR>& buildInfos,
    std::vector<VkBuffer>& scratchBuffers,
    std::vector<VmaAllocation>& scratchAllocations, bool isTlas)
{
    int i = 0;
    for (auto& buildInfo : buildInfos) {
        VkPhysicalDeviceAccelerationStructurePropertiesKHR asProps{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR
        };

        VkPhysicalDeviceProperties2 props2{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
        };

        uint32_t primitiveCount;
        if (isTlas) {
            primitiveCount = geometry.tlasGeometry.primitiveCount;
        } else {
            primitiveCount = geometry.blasGeometry[i].primitiveCount;
        }
        vk::AccelerationStructureBuildSizesInfoKHR sizeInfo = device -> getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice,
            buildInfo,
            primitiveCount
        );
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
        scratchBuffers[i] = scratchBuffer;
        scratchAllocations[i] = scratchAllocation;
        i++;
    }
}

/**
 * Given the entities, this method builds the acceleration structure used for ray-tracing
 */
void AccelerationStructureManager::build(std::vector<Entity*> entities, MVP& mvp) {
    buildBLASGeometry(entities);
    vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, 0);
    vk::raii::CommandPool commandPool(*device, poolInfo);
    vk::CommandBufferAllocateInfo cmdAllocInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::raii::CommandBuffers commandBuffers = device -> allocateCommandBuffers(cmdAllocInfo);
    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    vk::SubmitInfo submitInfo({}, {}, *commandBuffers[0]);
    vk::Queue queue = device -> getQueue(0, 0);
    queue.waitIdle();
    commandBuffers[0].begin(beginInfo);
    std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildInfos;
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> rangeInfos;
    buildBLAS(commandBuffers[0], &buildInfos, rangeInfos);

    std::vector<VkBuffer> scratchBuffers(buildInfos.size());
    std::vector<VmaAllocation> scratchAllocations(buildInfos.size());
    createScratchBuffers(
        buildInfos,
        scratchBuffers,
        scratchAllocations,
        false
    );

    std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> rangeInfoPointers(rangeInfos.size());

    for (size_t j = 0; j < rangeInfos.size(); ++j) {
        rangeInfoPointers[j] = &rangeInfos[j];
    }

    commandBuffers[0].buildAccelerationStructuresKHR(buildInfos, rangeInfoPointers);
    commandBuffers[0].end();
    queue.submit(submitInfo);
    queue.waitIdle();

    for (int j = 0; j < buildInfos.size(); j++) {
        vmaDestroyBuffer(*allocator, scratchBuffers[j], scratchAllocations[j]);
    }

    buildTLASGeometry(blasData, entities, &mvp);
    std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> tlasBuildInfos;
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> tlasRangeInfos;
    commandBuffers[0].begin(beginInfo);

    buildTLAS(commandBuffers[0], &tlasBuildInfos, tlasRangeInfos);

    std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> tlasRangeInfoPointers(tlasRangeInfos.size());

    std::vector<VkBuffer> tlasScratchBuffers(tlasBuildInfos.size());
    std::vector<VmaAllocation> tlasScratchAllocations(tlasBuildInfos.size());
    createScratchBuffers(
        tlasBuildInfos,
        tlasScratchBuffers,
        tlasScratchAllocations,
        true
    );

    for (size_t j = 0; j < tlasRangeInfos.size(); ++j) {
        tlasRangeInfoPointers[j] = &tlasRangeInfos[j];
    }

    commandBuffers[0].buildAccelerationStructuresKHR(tlasBuildInfos, tlasRangeInfoPointers);
    commandBuffers[0].end();
    queue.submit(submitInfo);
    queue.waitIdle();

    for (int j = 0; j < tlasBuildInfos.size(); j++) {
        vmaDestroyBuffer(*allocator, tlasScratchBuffers[j], tlasScratchAllocations[j]);
    }
}



/**
 * Builds the bottom-level acceleration structure
 */
void AccelerationStructureManager::buildBLAS(vk::raii::CommandBuffer& commandBuffer, std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> *buildInfos, std::vector<vk::AccelerationStructureBuildRangeInfoKHR>& rangeInfos) {
    for (int i = 0; i < geometry.blasGeometry.size(); i++) {
        AccelerationStructureData accelStructureData = {.handle = nullptr, .buffer = nullptr, .allocation = nullptr, .deviceAddress = 0};
        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo;
        vk::AccelerationStructureBuildRangeInfoKHR rangeInfo;
        buildAccelerationStructure(
            vk::AccelerationStructureTypeKHR::eBottomLevel,
            &accelStructureData.handle,
            geometry.blasGeometry[i].primitiveCount,
            geometry.blasGeometry[i].geometry,
            commandBuffer,
            &accelStructureData.buffer,
            &accelStructureData.allocation,
            &accelStructureData.deviceAddress,
            &buildInfo,
            &rangeInfo
        );
        blasData.push_back(std::move(accelStructureData));
        buildInfos->push_back(buildInfo);
        rangeInfos.push_back(rangeInfo);
    }
}

/**
 * Builds the top-level acceleration structure
 */
void AccelerationStructureManager::buildTLAS(vk::raii::CommandBuffer& commandBuffer, std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> *buildInfos, std::vector<vk::AccelerationStructureBuildRangeInfoKHR>& rangeInfos) {

    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo;
    vk::AccelerationStructureBuildRangeInfoKHR rangeInfo;
    buildAccelerationStructure(
        vk::AccelerationStructureTypeKHR::eTopLevel,
        &tlasData -> handle,
        geometry.tlasGeometry.primitiveCount,
        geometry.tlasGeometry.geometry,
        commandBuffer,
        &tlasData -> buffer,
        &tlasData -> allocation,
        &tlasData -> deviceAddress,
        &buildInfo,
        &rangeInfo
    );

    buildInfos->push_back(buildInfo);
    rangeInfos.push_back(rangeInfo);
}


/**
 * A helper for building the acceleration structure that configures build info, manages the scratch allocation
 * and sends the acceleration structure data to the gpu using the command buffer, and finally
 * updating passed-in acceleration structure allocation pointers when done
 */
void AccelerationStructureManager::buildAccelerationStructure(vk::AccelerationStructureTypeKHR accelerationStructureType,
    vk::raii::AccelerationStructureKHR* accelStructureHandle, uint32_t primitiveCount, vk::AccelerationStructureGeometryKHR& geometry, vk::raii::CommandBuffer& commandBuffer,  VkBuffer* buffer, VmaAllocation* allocation, vk::DeviceAddress* deviceAddress, vk::AccelerationStructureBuildGeometryInfoKHR* buildInfo, vk::AccelerationStructureBuildRangeInfoKHR* rangeInfo) {
    *buildInfo = vk::AccelerationStructureBuildGeometryInfoKHR(
        accelerationStructureType,
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowDataAccess,
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
        *buildInfo,
        {primitiveCount}
    );

    VkPhysicalDeviceAccelerationStructurePropertiesKHR asProps{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR
    };

    VkPhysicalDeviceProperties2 props2{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
    };
    props2.pNext = &asProps;

    vkGetPhysicalDeviceProperties2(**physicalDevice, &props2);


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

    buildInfo -> setDstAccelerationStructure(as);

    vk::AccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo(
        as
    );
    vk::DeviceAddress asDeviceAddress = device -> getAccelerationStructureAddressKHR(asDeviceAddressInfo);


    vk::MemoryBarrier2 asBarrier(
            vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
            vk::AccessFlagBits2::eAccelerationStructureWriteKHR,
            vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
            vk::AccessFlagBits2::eAccelerationStructureReadKHR
        );

    vk::DependencyInfo dependencyInfo(
            vk::DependencyFlags(),
            asBarrier,
            nullptr,
            nullptr
        );

    commandBuffer.pipelineBarrier2(dependencyInfo);
    *accelStructureHandle = std::move(as);
    *deviceAddress = asDeviceAddress;
    *buffer = asBuffer;
    *allocation = asAllocation;
    *rangeInfo = vk::AccelerationStructureBuildRangeInfoKHR(primitiveCount, 0, 0, 0);
}



/**
 * Builds the geometry of the Top-Level acceleration structure. In doing so, we create a instance buffer to
 * retrieve the geometry and update the Geometry struct accordingly.
 */
void AccelerationStructureManager::buildTLASGeometry(std::vector<AccelerationStructureData>& blasData, std::vector<Entity*> entities, MVP* mvp) {
    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    int i = 0;
    int materialIndex = 0;
    for (Entity* entity : entities) {
        // if (entity -> isEmissive()) {
        //     if (entity -> hasMaterial()) {
        //         materialIndex++;
        //     }
        //     continue;
        // }
        entity -> updateTransformationMatrix();
        glm::mat4 transposed = glm::transpose(mvp -> transformation);
        vk::TransformMatrixKHR transformMatrix;
        memcpy(&transformMatrix, &transposed, sizeof(vk::TransformMatrixKHR));
        vk::AccelerationStructureInstanceKHR asInstance(
            transformMatrix,
            (entity -> hasMaterial()) ? materialIndex : 0,
            0xFF,
            (entity -> hasMaterial()) ? (materialIndex) : 0,
            vk::GeometryInstanceFlagBitsKHR::eTriangleFrontCounterclockwise,
            blasData[i].deviceAddress
        );
        if (entity -> hasMaterial()) {
            materialIndex++;
        }
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


/**
 * Builds the geometry of the Bottom-Level acceleration structure. In doing so, we upload our vertex & index buffer addresses
 * and update our Blas Geometry.
 */
void AccelerationStructureManager::buildBLASGeometry(std::vector<Entity*> entities) {

    int i = 0;
    for (Entity* entity : entities) {
        // if (entity -> isEmissive()) {
        //     continue;
        // }
        vk::BufferDeviceAddressInfo vertexBufferDeviceAddressInfo(
            *entity -> getModel().vertexBuffer
        );
        vk::DeviceAddress vertexAddress = device -> getBufferAddress(vertexBufferDeviceAddressInfo);
        vk::BufferDeviceAddressInfo indexBufferDeviceAddressInfo(
            *entity -> getModel().indexBuffer
        );
        vk::DeviceAddress indexAddress = device -> getBufferAddress(indexBufferDeviceAddressInfo);

        vk::DeviceOrHostAddressConstKHR vertexData(vertexAddress);
        vk::DeviceOrHostAddressConstKHR indexData(indexAddress);
        vk::AccelerationStructureGeometryTrianglesDataKHR triangleData(
            vk::Format::eR32G32B32Sfloat,
            vertexData,
            sizeof(float) * 3,
            entity -> getModel().numVertices - 1,
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


        GeometryData geometryData = {.geometry = triangleGeometry, .primitiveCount = (uint32_t)(entity -> getModel().numIndices / 3)};
        geometry.blasGeometry.push_back(geometryData);
        i++;
    }

    geometry.tlasGeometry.primitiveCount = i;

}

void AccelerationStructureManager::cleanUp() {
    vmaDestroyBuffer(*allocator, tlasData ->buffer, tlasData -> allocation);
    for (auto& blas : blasData) {
        vmaDestroyBuffer(*allocator, blas.buffer, blas.allocation);
    }
    vmaDestroyBuffer(*allocator, instanceBuffer, instanceAllocation);
}




