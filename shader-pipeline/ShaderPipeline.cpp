
#include "ShaderPipeline.h"

#include <cstring>
#include <vk_mem_alloc.h>

ShaderPipeline::ShaderPipeline(std::string str, vk::raii::Device& device,  VmaAllocator& allocator, DescriptorsInfo desc) {
    this->device = &device;
    this->allocator = &allocator;
    this->desc = desc;
    this->identifier = str;
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
        // for (int i = 0; i < uboBuffers.size(); i++) {
        //     if (i != frameIndex) {
        //         setUniform(descriptorBinding, data, size, i);
        //     }
        // }
    }
    // void* mapped;
    // vmaMapMemory(*allocator, uboAllocations[frameIndex][descriptorBinding], &mapped);
    memcpy(persistentUBOPointers[uboBuffers[frameIndex][descriptorBinding]], data, size);
    vmaFlushAllocation(*allocator, uboAllocations[frameIndex][descriptorBinding], 0, size);
    // vmaUnmapMemory(*allocator, uboAllocations[frameIndex][descriptorBinding]);
}


void ShaderPipeline::setStorageBuffer(DescriptorBinding descriptorBinding, void *data, uint32_t size, int frameIndex) {
    if (!storageBufferAllocations[frameIndex].contains(descriptorBinding)) {
        VkBuffer buffer;
        VmaAllocation allocation;
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo resultInfo;
        vmaCreateBuffer(*allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &resultInfo);

        storageBuffers[frameIndex][descriptorBinding] = buffer;
        storageBufferAllocations[frameIndex][descriptorBinding] = allocation;

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
            vk::DescriptorType::eStorageBuffer,
            nullptr,
            &descriptorBufferInfo,
            nullptr,
            nullptr
        );
        device->updateDescriptorSets(write, nullptr);
        persistentStoragePointers[storageBuffers[frameIndex][descriptorBinding]] = resultInfo.pMappedData;
        // for (int i = 0; i < storageBuffers.size(); i++) {
        //     if (i != frameIndex) {
        //         setStorageBuffer(descriptorBinding, data, size, i);
        //     }
        // }
    }
    memcpy(persistentStoragePointers[storageBuffers[frameIndex][descriptorBinding]], data, size);
    vmaFlushAllocation(*allocator, storageBufferAllocations[frameIndex][descriptorBinding], 0, size);
}

void ShaderPipeline::setStorageImage(DescriptorBinding descriptorBinding, int width, int height, int frameIndex, std::string identifier) {
    for (int i = 0; i < storageImages.size(); i++) {
        if (storageImages[i].identifier == identifier && storageImages[i].frameIndex == frameIndex) {
            device->waitIdle();
            vmaDestroyImage(*allocator, storageImages[i].image, storageImages[i].allocation);
            storageImages.erase(storageImages.begin() + i);
            break;
        }
    }
    Image image = {.imageView = nullptr, .sampler = nullptr};
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;


    vmaCreateImage(
        *allocator,
        &imageInfo,
        &allocInfo,
        &image.image,
        &image.allocation,
        nullptr
    );

    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = image.image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = vk::Format::eR32G32B32A32Sfloat;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    image.imageView = device -> createImageView(viewInfo);

    vk::SamplerCreateInfo samplerCreateInfo(
        {},
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eClampToEdge,
        vk::SamplerAddressMode::eClampToEdge,
    vk::SamplerAddressMode::eClampToEdge,
        0.0,
        0.0,
        0.0,
        vk::False, vk::CompareOp::eNever,
        0.0, 0.0, vk::BorderColor::eIntOpaqueBlack

    );
    image.sampler = device->createSampler(samplerCreateInfo);
    image.identifier = identifier;
    image.frameIndex = frameIndex;
    vk::DescriptorImageInfo descriptorImageInfo(
        image.sampler,
        image.imageView,
        vk::ImageLayout::eGeneral

    );
    vk::WriteDescriptorSet write(
        *descriptorSets[frameIndex][descriptorBinding.set],
        descriptorBinding.binding,
        0,
        1,
        vk::DescriptorType::eStorageImage,
        &descriptorImageInfo,
        nullptr,
        nullptr,
        nullptr
    );
    device->updateDescriptorSets(write, nullptr);
    storageImages.push_back(std::move(image));


}

//This method is for externally managed texture samplers

void ShaderPipeline::setTextureSampler(DescriptorBinding descriptorBinding, TextureView &image, vk::ImageLayout imageLayout, int frameIndex) const {
        vk::SamplerCreateInfo samplerCreateInfo(
         {},
         vk::Filter::eLinear,
         vk::Filter::eLinear,
         vk::SamplerMipmapMode::eLinear,
         vk::SamplerAddressMode::eClampToEdge,
         vk::SamplerAddressMode::eClampToEdge,
     vk::SamplerAddressMode::eClampToEdge,
         0.0,
         0.0,
         0.0,
         vk::False, vk::CompareOp::eNever,
         0.0, 0.0, vk::BorderColor::eIntOpaqueBlack

     );
    if (!image.sampler.has_value()) {
        image.sampler.emplace(device->createSampler(samplerCreateInfo));
    }
    vk::DescriptorImageInfo descriptorImageInfo(
        *image.sampler,
        image.imageView,
        imageLayout

    );
    vk::WriteDescriptorSet write(
        *descriptorSets[frameIndex][descriptorBinding.set],
        descriptorBinding.binding,
        0,
        1,
        vk::DescriptorType::eCombinedImageSampler,
        &descriptorImageInfo,
        nullptr,
        nullptr,
        nullptr
    );
    device->updateDescriptorSets(write, nullptr);
}


//This method supports bindless texture arrays
void ShaderPipeline::setTextureBuffer(DescriptorBinding descriptorBinding, std::vector<TextureView>& images, vk::ImageLayout imageLayout, int frameIndex) const {
    std::vector<vk::DescriptorImageInfo> imageInfo;
    for (TextureView& image : images) {
        vk::SamplerCreateInfo samplerCreateInfo(
         {},
             vk::Filter::eLinear,
             vk::Filter::eLinear,
             vk::SamplerMipmapMode::eLinear,
             vk::SamplerAddressMode::eRepeat,
             vk::SamplerAddressMode::eRepeat,
         vk::SamplerAddressMode::eRepeat,
             0.0,
             VK_TRUE,
             8.0,
             vk::False, vk::CompareOp::eNever,
             0.0, VK_LOD_CLAMP_NONE, vk::BorderColor::eIntOpaqueBlack
         );
        if (!image.sampler.has_value()) {
            image.sampler.emplace(device->createSampler(samplerCreateInfo));
        }
        vk::DescriptorImageInfo descriptorImageInfo(
            *image.sampler,
            image.imageView,
            imageLayout

        );
        imageInfo.push_back(descriptorImageInfo);
    }

    vk::WriteDescriptorSet write(
    *descriptorSets[frameIndex][descriptorBinding.set],
        descriptorBinding.binding,
        0,
        imageInfo.size(),
        vk::DescriptorType::eCombinedImageSampler,
        imageInfo.data(),
        nullptr,
        nullptr,
        nullptr
    );
    device->updateDescriptorSets(write, nullptr);

}


const std::vector<Image> & ShaderPipeline::getStorageImages() const {
    return storageImages;
}

Image & ShaderPipeline::getStorageImage(std::string identifier, int frameIndex) {
    for (auto & image : storageImages) {
        if (image.identifier == identifier && image.frameIndex == frameIndex) {
            return image;
        }
    }
    throw std::runtime_error("No storage image found with specified identifier!");
}





void ShaderPipeline::cleanUp() {
    for (int i = 0; i < uboBuffers.size(); i++) {
        for (const auto& [key, value] : uboBuffers[i]) {
            vmaDestroyBuffer(*allocator, uboBuffers[i].at(key), uboAllocations[i].at(key));
        }
    }
    for (int i = 0; i < storageBuffers.size(); i++) {
        for (const auto& [key, value] : storageBuffers[i]) {
            vmaDestroyBuffer(*allocator, storageBuffers[i].at(key), storageBufferAllocations[i].at(key));
        }
    }
    for (int i = 0; i < storageImages.size(); i++) {
        vmaDestroyImage(*allocator, storageImages[i].image, storageImages[i].allocation);
    }
    persistentUBOPointers.clear();
    persistentStoragePointers.clear();
}

const std::string ShaderPipeline::getIdentifier() const {
    return identifier;
}

vk::Buffer & ShaderPipeline::getUniformBuffer(DescriptorBinding binding, int frameIndex) {
    return uboBuffers[frameIndex][binding];
}



//Note to self: descriptors architecture
//2 layouts:once-added, per-frame data
//what we need to know per layout: number of UBOs, number of TextureSamplers.
//Plan; we can model this with 2 params each of which having a struct that contains numUBOs, and numTextureSamplers
//Limitations: this will require the external caller to remember to manually call the setup of the descriptors
//as it will not be done in the constructor

void ShaderPipeline::setUpDescriptors() {
    uint32_t count =
        ((desc.staticData.numUBOs || desc.staticData.numTextureSamplers || desc.staticData.numAccelerationStructures || desc.dynamicData.numStorageImages) ? 1 : 0) +
        ((desc.dynamicData.numUBOs || desc.dynamicData.numTextureSamplers || desc.dynamicData.numAccelerationStructures || desc.dynamicData.numStorageImages) ? 1 : 0);

    if (count == 0) {
        return;
    }

    std::vector<vk::DescriptorPoolSize> poolSizes;
    if (desc.dynamicData.numUBOs + desc.staticData.numUBOs > 0)
        poolSizes.push_back({ vk::DescriptorType::eUniformBuffer, 100 });
    if (desc.dynamicData.numTextureSamplers + desc.staticData.numTextureSamplers > 0)
        poolSizes.push_back({ vk::DescriptorType::eCombinedImageSampler, 1000 });
    if (desc.dynamicData.numAccelerationStructures + desc.staticData.numAccelerationStructures > 0)
        poolSizes.push_back({ vk::DescriptorType::eAccelerationStructureKHR, 100 });
    if (desc.dynamicData.numStorageImages + desc.staticData.numStorageImages > 0)
        poolSizes.push_back({ vk::DescriptorType::eStorageImage, 100 });
    if (desc.dynamicData.numStorageBuffers + desc.staticData.numStorageBuffers > 0)
        poolSizes.push_back({ vk::DescriptorType::eStorageBuffer, 100 });


    vk::DescriptorPoolCreateInfo poolInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        (desc.dynamicData.numTextureSamplers + desc.dynamicData.numUBOs + desc.staticData.numTextureSamplers + desc.staticData.numUBOs
                + desc.dynamicData.numAccelerationStructures + desc.staticData.numAccelerationStructures + desc.dynamicData.numStorageImages + desc.staticData.numStorageImages
                + desc.staticData.numStorageBuffers + desc.dynamicData.numStorageBuffers) * descriptorSets.size(),
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

    for (int i = 0; i < desc.numAccelerationStructures; i++) {
        vk::DescriptorSetLayoutBinding binding(
            desc.numUBOs + i,
            vk::DescriptorType::eAccelerationStructureKHR,
            1,
            {getShaderStageFlags()},
            {}
        );
        bindings.push_back(binding);
    }

    for (int i = 0; i < desc.numTextureSamplers; i++) {
        vk::DescriptorSetLayoutBinding binding(
            desc.numUBOs + desc.numAccelerationStructures + i,
            vk::DescriptorType::eCombinedImageSampler,
            1,
            {getShaderStageFlags()},
            {}

        );
        bindings.push_back(binding);
    }

    for (int i = 0; i < desc.textureBuffersInfo.size(); i++) {
        vk::DescriptorSetLayoutBinding binding(
            desc.numUBOs + desc.numAccelerationStructures + desc.numTextureSamplers + i,
            vk::DescriptorType::eCombinedImageSampler,
            desc.textureBuffersInfo[i].numTextures,
            {getShaderStageFlags()},
            {}

        );
        bindings.push_back(binding);
    }

    for (int i = 0; i < desc.numStorageImages; i++) {
        vk::DescriptorSetLayoutBinding binding(
            desc.numUBOs + desc.numTextureSamplers + desc.textureBuffersInfo.size() + desc.numAccelerationStructures + i,
            vk::DescriptorType::eStorageImage,
            1,
            {getShaderStageFlags()},
            {}
        );
        bindings.push_back(binding);
    }

    for (int i = 0; i < desc.numStorageBuffers; i++) {
        vk::DescriptorSetLayoutBinding binding(
            desc.numUBOs + desc.numTextureSamplers + desc.textureBuffersInfo.size() + desc.numAccelerationStructures + desc.numStorageImages + i,
            vk::DescriptorType::eStorageBuffer,
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