

#include "RaytracingShaderPipeline.h"

#include <iostream>

#include "../raytracing/AccelerationStructureManager.h"


struct AccelerationStructureData;

RaytracingShaderPipeline::RaytracingShaderPipeline(std::string string, vk::raii::Device &device,vk::raii::PhysicalDevice &physicalDevice,
                                                   VmaAllocator&allocator, DescriptorsInfo desc, int numMaterials
) : ShaderPipeline(string, device, allocator, desc) {
    setUpDescriptors();
    this -> physicalDevice = physicalDevice;
    ShaderProgram rayGen(string + "Raygen.rgen.spv", &device, vk::ShaderStageFlagBits::eRaygenKHR);
    ShaderProgram miss(string + "Miss.rmiss.spv", &device, vk::ShaderStageFlagBits::eMissKHR);
    ShaderProgram closestHit(string + "ClosestHit.rchit.spv", &device, vk::ShaderStageFlagBits::eClosestHitKHR);
    shaders.push_back(std::move(rayGen));
    shaders.push_back(std::move(miss));
    shaders.push_back(std::move(closestHit));
    this -> numMaterials = numMaterials;
    RaytracingShaderPipeline::setUpPipeline();


}


void RaytracingShaderPipeline::bind(vk::raii::CommandBuffer &commandBuffer, int frameIndex) {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *rtPipeline);

    std::vector<uint32_t> offsets(0, 0);
    std::vector<vk::DescriptorSet> rawSets;
    rawSets.reserve(descriptorSets[frameIndex].size());

    for (const auto& set : descriptorSets[frameIndex]) {
        rawSets.push_back(*set);
    }
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *rtPipelineLayout, 0, rawSets, offsets);

}

void RaytracingShaderPipeline::setUpPipeline() {
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
        vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
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
        VK_NULL_HANDLE,                          // deferred operation
        VK_NULL_HANDLE,                          // pipeline cache
        pipelineCreateInfo,          // vk::RayTracingPipelineCreateInfoKHR
        VK_NULL_HANDLE
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

    uint32_t stride = (handleSize + sizeof(uint32_t) + rayTracingPipelineProperties.shaderGroupBaseAlignment - 1) & ~(rayTracingPipelineProperties.shaderGroupBaseAlignment - 1);



    VkBufferCreateInfo sbtBufferInfo{};
    sbtBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

    sbtBufferInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VmaAllocationCreateInfo asAllocInfo{};
    asAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    asAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    uint32_t handleSizeAligned = (handleSize + rayTracingPipelineProperties.shaderGroupHandleAlignment - 1) & ~(rayTracingPipelineProperties.shaderGroupHandleAlignment - 1);
    sbtBufferInfo.size = stride * (numMaterials != 0 ? 2 + numMaterials : 3);

    vmaCreateBuffer(*allocator, &sbtBufferInfo, &asAllocInfo, &sbtBuffer, &sbtAllocation, nullptr);

    VmaAllocationInfo sbtAllocInfo;
    vmaGetAllocationInfo(*allocator, sbtAllocation, &sbtAllocInfo);
    uint8_t* mapped = (uint8_t*)sbtAllocInfo.pMappedData;

    memcpy(mapped, handles.data(), handleSize);
    memcpy(mapped + stride, handles.data() + handleSize, handleSize);
    if (numMaterials == 0) {
        memcpy(mapped + stride * 2, handles.data() + handleSize * 2, handleSize);

    }

    for (int i = 2; i < 2 + numMaterials; i++) {
        uint8_t* dst = mapped + (stride * i);
        memcpy(dst, handles.data() + handleSize * 2, handleSize);
        uint32_t matID = i - 2;
        memcpy(dst + handleSize, &matID, sizeof(uint32_t));
    }

    vk::BufferDeviceAddressInfo deviceAddressInfo(
        sbtBuffer
    );

    vk::DeviceAddress address = device -> getBufferAddress(deviceAddressInfo);

    raygenRegion.emplace(
        address,
        stride,
        stride
    );

    missRegion.emplace(
        address + stride,
        stride,
        stride
    );

    closestHitRegion.emplace(
        address + (stride * 2),
        stride,
        stride * numMaterials
    );
    vmaFlushAllocation(*allocator, sbtAllocation, 0, VK_WHOLE_SIZE);

}

vk::ShaderStageFlags RaytracingShaderPipeline::getShaderStageFlags() {
    return vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR |
           vk::ShaderStageFlagBits::eClosestHitKHR;
}


//TODO: MAKE TLAS DYNAMIC
void RaytracingShaderPipeline::setAccelerationStructure(DescriptorBinding descriptorBinding, AccelerationStructureData& tlasData,  int frameIndex) {
    vk::WriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAcceleration(
        1,
        &*(tlasData.handle),
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
    vmaDestroyBuffer(*allocator, sbtBuffer, sbtAllocation);
}



