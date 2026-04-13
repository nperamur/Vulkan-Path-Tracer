//
// Created by neele on 4/10/2026.
//

#include "ShaderPipeline.h"

#include <cstring>
#include <vk_mem_alloc.h>

ShaderPipeline::ShaderPipeline(std::string str, vk::raii::Device& device, vk::Format& swapChainImageFormat, VmaAllocator& allocator, DescriptorsInfo desc) {
    this->device = &device;
    this->swapChainImageFormat = &swapChainImageFormat;
    this->allocator = &allocator;
    this->desc = desc;
    this->identifier = str;
    setUpDescriptors();
}

void ShaderPipeline::setUniform(DescriptorBinding descriptorBinding, void *data, uint32_t size, int frameIndex) {
    if (!uboAllocations[frameIndex].contains(descriptorBinding)) {
        VkBuffer buffer;
        VmaAllocation allocation;
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo resultInfo;
        vmaCreateBuffer(*allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &resultInfo);

        uboBuffers[frameIndex][descriptorBinding] = buffer;
        uboAllocations[frameIndex][descriptorBinding] = allocation;

        vk::DescriptorBufferInfo descriptorBufferInfo(
            buffer,
            0,
            size
        );

        vk::WriteDescriptorSet write(
            *descriptorSets[frameIndex][descriptorBinding.set],
            descriptorBinding.binding,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &descriptorBufferInfo,
            nullptr,
            nullptr
        );
        device->updateDescriptorSets(write, nullptr);
        persistentUBOPointers[uboBuffers[frameIndex][descriptorBinding]] = resultInfo.pMappedData;
        for (int i = 0; i < uboBuffers.size(); i++) {
            if (i != frameIndex) {
                setUniform(descriptorBinding, data, size, i);
            }
        }
    }
    // void* mapped;
    // vmaMapMemory(*allocator, uboAllocations[frameIndex][descriptorBinding], &mapped);
    memcpy(persistentUBOPointers[uboBuffers[frameIndex][descriptorBinding]], data, size);
    // vmaUnmapMemory(*allocator, uboAllocations[frameIndex][descriptorBinding]);
}


void ShaderPipeline::cleanUp() {
    for (int i = 0; i < uboBuffers.size(); i++) {
        for (const auto& [key, value] : uboBuffers[i]) {
            vmaDestroyBuffer(*allocator, uboBuffers[i].at(key), uboAllocations[i].at(key));
        }
    }
    persistentUBOPointers.clear();
}



//Note to self: descriptors architecture
//2 layouts:once-added, per-frame data
//what we need to know per layout: number of UBOs, number of TextureSamplers.
//Plan; we can model this with 2 params each of which having a struct that contains numUBOs, and numTextureSamplers
//Limitations: this will require the external caller to remember to manually call the setup of the descriptors
//as it will not be done in the constructor

void ShaderPipeline::setUpDescriptors() {
    if (desc.dynamicData.numUBOs == 0 && desc.dynamicData.numTextureSamplers == 0
          && desc.staticData.numUBOs == 0 && desc.staticData.numTextureSamplers == 0
          && desc.dynamicData.numAccelerationStructures == 0 && desc.staticData.numAccelerationStructures == 0) {
        return;
    }

    std::vector<vk::DescriptorPoolSize> poolSizes;
    if (desc.dynamicData.numUBOs + desc.staticData.numUBOs > 0)
        poolSizes.push_back({ vk::DescriptorType::eUniformBuffer, 100 });
    if (desc.dynamicData.numTextureSamplers + desc.staticData.numTextureSamplers > 0)
        poolSizes.push_back({ vk::DescriptorType::eCombinedImageSampler, 100 });
    if (desc.dynamicData.numAccelerationStructures + desc.staticData.numAccelerationStructures > 0)
        poolSizes.push_back({ vk::DescriptorType::eAccelerationStructureKHR, 100 });


    vk::DescriptorPoolCreateInfo poolInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        (desc.dynamicData.numTextureSamplers + desc.dynamicData.numUBOs + desc.staticData.numTextureSamplers + desc.staticData.numUBOs
                + desc.dynamicData.numAccelerationStructures + desc.staticData.numAccelerationStructures) * descriptorSets.size(),
        poolSizes.size(),
        poolSizes.data()
    );

    descriptorPool.emplace(*device, poolInfo);

    std::vector<vk::DescriptorSetLayoutBinding> staticBindingsArray;
    onceAddedLayout.emplace(*device, getDescriptorSetCreateInfo(desc.staticData, staticBindingsArray));
    layouts.push_back(**onceAddedLayout);
    std::vector<vk::DescriptorSetLayoutBinding> dynamicBindingsArray;
    perFrameLayout.emplace(*device, getDescriptorSetCreateInfo(desc.dynamicData, dynamicBindingsArray));
    layouts.push_back(**perFrameLayout);

    uint32_t count =
        ((desc.staticData.numUBOs || desc.staticData.numTextureSamplers || desc.staticData.numAccelerationStructures) ? 1 : 0) +
        ((desc.dynamicData.numUBOs || desc.dynamicData.numTextureSamplers || desc.dynamicData.numAccelerationStructures) ? 1 : 0);
    vk::DescriptorSetAllocateInfo allocInfo(
        *descriptorPool,
        count,
        layouts.data()
    );
    for (uint32_t i = 0; i < descriptorSets.size(); ++i) {
        descriptorSets[i] = device -> allocateDescriptorSets(allocInfo);
    }


}

vk::ShaderStageFlags ShaderPipeline::getShaderStageFlags() {
    return vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
}


vk::DescriptorSetLayoutCreateInfo ShaderPipeline::getDescriptorSetCreateInfo(DescriptorLayoutDesc desc, std::vector<vk::DescriptorSetLayoutBinding>& bindings) {
    for (int i = 0; i < desc.numUBOs; i++) {
        vk::DescriptorSetLayoutBinding binding(
            i,
            vk::DescriptorType::eUniformBuffer,
            1,
            {getShaderStageFlags()},
            {}

        );
        bindings.push_back(binding);
    }

    for (int i = 0; i < desc.numTextureSamplers; i++) {
        vk::DescriptorSetLayoutBinding binding(
            desc.numUBOs + i,
            vk::DescriptorType::eCombinedImageSampler,
            1,
            {getShaderStageFlags()},
            {}

        );
        bindings.push_back(binding);
    }

    for (int i = 0; i < desc.numAccelerationStructures; i++) {
        vk::DescriptorSetLayoutBinding binding(
            desc.numUBOs + desc.numTextureSamplers + i,
            vk::DescriptorType::eAccelerationStructureKHR,
            1,
            {getShaderStageFlags()},
            {}

        );
        bindings.push_back(binding);
    }


    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
        {},
        bindings
    );
    return descriptorSetLayoutCreateInfo;
}



std::vector<vk::PipelineShaderStageCreateInfo> ShaderPipeline::getStageCreateInfos() {
    std::vector<vk::PipelineShaderStageCreateInfo> stageCreateInfos;

    for (ShaderProgram& program : shaders) {
        stageCreateInfos.push_back(program.getCreateInfo());
    }

    return stageCreateInfos;
}