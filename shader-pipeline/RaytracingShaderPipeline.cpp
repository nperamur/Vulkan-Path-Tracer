

#include "RaytracingShaderPipeline.h"

RaytracingShaderPipeline::RaytracingShaderPipeline(std::string string, vk::raii::Device &device,vk::raii::PhysicalDevice &physicalDevice,
    vk::Format &swapChainImageFormat, VmaAllocator&allocator, DescriptorsInfo desc, Geometry& geometry
    ) : ShaderPipeline(string, device, swapChainImageFormat, allocator, desc) {
    this -> geometry = geometry;
    this -> physicalDevice.emplace(physicalDevice);

    tlasData.emplace(AccelerationStructureData{.handle = nullptr, .buffer = nullptr, .allocation = nullptr, .deviceAddress = 0});
    ShaderProgram rayGen(string + "Raygen.rgen.spv", &device, vk::ShaderStageFlagBits::eRaygenKHR);
    ShaderProgram miss(string + "Miss.rmiss.spv", &device, vk::ShaderStageFlagBits::eMissKHR);
    ShaderProgram closestHit(string + "ClosestHit.rchit.spv", &device, vk::ShaderStageFlagBits::eClosestHitKHR);
    shaders.push_back(std::move(rayGen));
    shaders.push_back(std::move(miss));
    shaders.push_back(std::move(closestHit));
    RaytracingShaderPipeline::setUpPipeline();
}


void RaytracingShaderPipeline::bind(vk::raii::CommandBuffer &commandBuffer, int frameIndex) {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *rtPipeline);

}

void RaytracingShaderPipeline::setUpPipeline() {
    //
    //
    //
    // vk::BufferDeviceAddressInfo addressInfo(**model.vertexBuffer);
    // vk::DeviceAddress vertexAddress = device.getBufferAddress(addressInfo);
    //
    // vk::DeviceOrHostAddressConstKHR vertexData(vertexAddress);
    //
    // vk::AccelerationStructureGeometryTrianglesDataKHR triangleData(
    //
    //
    //
    // );
    // vk::AccelerationStructureGeometryDataKHR({}
    //
    //
    // );
    // vk::AccelerationStructureGeometryKHR(
    //
    // );
    std::vector<vk::PipelineShaderStageCreateInfo> info = getStageCreateInfos();

    //TODO: Add Any Hit or Intersection if Needed

    vk::RayTracingShaderGroupCreateInfoKHR raygenGroup(
        vk::RayTracingShaderGroupTypeKHR::eGeneral,
        0, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR
    );

    vk::RayTracingShaderGroupCreateInfoKHR missGroup(
        vk::RayTracingShaderGroupTypeKHR::eGeneral,
        1, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR
    );


    vk::RayTracingShaderGroupCreateInfoKHR hitGroup(
        vk::RayTracingShaderGroupTypeKHR::eGeneral,
        VK_SHADER_UNUSED_KHR, 2, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR
    );

    std::array<vk::RayTracingShaderGroupCreateInfoKHR, 3> groups = {raygenGroup, missGroup, hitGroup};


    //TODO: max pipeline recursion depth is number of bounces. I might want to change this number later
    uint32_t count =
         ((desc.staticData.numUBOs || desc.staticData.numTextureSamplers) ? 1 : 0) +
         ((desc.dynamicData.numUBOs || desc.dynamicData.numTextureSamplers) ? 1 : 0);

    vk::PipelineLayoutCreateInfo layoutInfo({}, count, layouts.data(), 0, nullptr);

    rtPipelineLayout.emplace(*device, layoutInfo);
    vk::RayTracingPipelineCreateInfoKHR pipelineCreateInfo(
        {},
        3,
        (info.data()),
        3,
        (groups.data()),
        1,
        nullptr,
        nullptr, nullptr, *rtPipelineLayout,
        nullptr,
        {}


    );

    //TODO: deferred operations and pipeline caching are optimizations to make rt faster.
    auto result = device->createRayTracingPipelinesKHR(
        nullptr,                          // deferred operation
        nullptr,                          // pipeline cache
        pipelineCreateInfo,          // vk::RayTracingPipelineCreateInfoKHR
        nullptr                      // allocator
    );



    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
    rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 deviceProperties2{};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &rayTracingPipelineProperties;
    vkGetPhysicalDeviceProperties2(**physicalDevice, &deviceProperties2);
    uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;

    rtPipeline.emplace(std::move(result.at(0)));
    std::vector<uint8_t> handles = rtPipeline->getRayTracingShaderGroupHandlesKHR<uint8_t>(0, 3, (size_t)(handleSize * 3));



    VkBufferCreateInfo sbtBufferInfo{};
    sbtBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

    sbtBufferInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;    VmaAllocationCreateInfo asAllocInfo{};
    asAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    asAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    uint32_t handleSizeAligned = (handleSize + rayTracingPipelineProperties.shaderGroupHandleAlignment - 1) & ~(rayTracingPipelineProperties.shaderGroupHandleAlignment - 1);
    sbtBufferInfo.size = handleSizeAligned * 3;
    vmaCreateBuffer(*allocator, &sbtBufferInfo, &asAllocInfo, &sbtBuffer, &sbtAllocation, nullptr);

    VmaAllocationInfo sbtAllocInfo;
    vmaGetAllocationInfo(*allocator, sbtAllocation, &sbtAllocInfo);
    uint8_t* mapped = (uint8_t*)sbtAllocInfo.pMappedData;

    memcpy(mapped, handles.data(), handleSize);
    memcpy(mapped + handleSizeAligned, handles.data() + handleSizeAligned, handleSize);
    memcpy(mapped + handleSizeAligned * 2, handles.data() + handleSizeAligned * 2, handleSize);

    vk::BufferDeviceAddressInfo deviceAddressInfo(
        sbtBuffer
    );

    vk::DeviceAddress address = device -> getBufferAddress(deviceAddressInfo);

    raygenRegion.emplace(
        address,
        handleSizeAligned,
        handleSizeAligned
    );

    missRegion.emplace(
        address + handleSizeAligned,
        handleSizeAligned,
        handleSizeAligned
    );

    closestHitRegion.emplace(
        address + handleSizeAligned * 2,
        handleSizeAligned,
        handleSizeAligned
    );



}

void RaytracingShaderPipeline::buildBLAS() {
    blasData.reserve(geometry.blasGeometry.size());
    for (int i = 0; i < geometry.blasGeometry.size(); i++) {
        AccelerationStructureData accelStructureData = {.handle = nullptr, .buffer = nullptr, .allocation = nullptr, .deviceAddress = 0};
        buildAccelerationStructure(
            vk::AccelerationStructureTypeKHR::eBottomLevel,
            &accelStructureData.handle,
            1,
            geometry.blasGeometry[i].primitiveCount,
            geometry.blasGeometry[i].geometry,
            &accelStructureData.buffer,
            &accelStructureData.allocation,
            &accelStructureData.deviceAddress

        );
        blasData.push_back(std::move(accelStructureData));
    }
}

void RaytracingShaderPipeline::buildTLAS() {
    buildAccelerationStructure(
        vk::AccelerationStructureTypeKHR::eTopLevel,
        &tlasData -> handle,
        1,
        geometry.tlasGeometry.primitiveCount,
        geometry.tlasGeometry.geometry,
        &tlasData -> buffer,
        &tlasData -> allocation,
        &tlasData -> deviceAddress
    );
}

void RaytracingShaderPipeline::buildAccelerationStructure(vk::AccelerationStructureTypeKHR accelerationStructureType,
    vk::raii::AccelerationStructureKHR* accelStructureHandle, int primitiveCount, int instanceCount, vk::AccelerationStructureGeometryKHR geometry,  VkBuffer* buffer, VmaAllocation* allocation, vk::DeviceAddress* deviceAddress) {
        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo(
        accelerationStructureType,
        {},
        vk::BuildAccelerationStructureModeKHR::eBuild,
        nullptr,
        nullptr,
        primitiveCount,
        &geometry,
        nullptr,
        nullptr,
        nullptr

    );


    vk::AccelerationStructureBuildSizesInfoKHR sizeInfo = device -> getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        buildInfo,
        {1}
    );

    VkBuffer scratchBuffer;
    VmaAllocation scratchAllocation;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    bufferInfo.size = sizeInfo.buildScratchSize;
    vmaCreateBuffer(*allocator, &bufferInfo, &allocInfo, &scratchBuffer, &scratchAllocation, nullptr);
    vk::BufferDeviceAddressInfo deviceAddressInfo(
        scratchBuffer
    );
    vk::DeviceAddress address = device -> getBufferAddress(deviceAddressInfo);
    buildInfo.setScratchData(address);

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
        instanceCount,
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

vk::ShaderStageFlags RaytracingShaderPipeline::getShaderStageFlags() {
    return vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR |
           vk::ShaderStageFlagBits::eClosestHitKHR;
}


//TODO: MAKE TLAS DYAAMIC
void RaytracingShaderPipeline::setAccelerationStructure(DescriptorBinding descriptorBinding, int frameIndex) {
    vk::WriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAcceleration(
        1,
        &*(tlasData -> handle),
        nullptr
    );

    vk::WriteDescriptorSet write(
         *descriptorSets[frameIndex][descriptorBinding.set],
         descriptorBinding.binding,
         0,
         1,
         vk::DescriptorType::eAccelerationStructureKHR,
         nullptr,
         nullptr,
         nullptr,
         &writeDescriptorSetAcceleration

    );

    device->updateDescriptorSets(write, nullptr);
}

//TODO: CLEAN UP ALL ACCELERATION STRUCTURES
void RaytracingShaderPipeline::cleanUp() {
    ShaderPipeline::cleanUp();
    vmaDestroyBuffer(*allocator, tlasData ->buffer, tlasData -> allocation);
    for (auto& blas : blasData) {
        vmaDestroyBuffer(*allocator, blas.buffer, blas.allocation);
    }
    vmaDestroyBuffer(*allocator, sbtBuffer, sbtAllocation);
}



