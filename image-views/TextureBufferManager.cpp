
#include "TextureBufferManager.h"

#include <iostream>
#include <ostream>

TextureBufferManager::TextureBufferManager(VmaAllocator* allocator, vk::raii::Device &device) {
    this -> allocator = allocator;
    this -> device = &device;
}

void TextureBufferManager::registerTextureBuffer(std::string id) {
    textureViews.try_emplace(id);
}

void TextureBufferManager::registerImage(std::string imageId, std::string textureBufferId, VkFormat format, int width, int height) {
    Image image = {.imageView = nullptr, .sampler = nullptr};
    image.identifier = imageId;
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    // imageInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
    imageInfo.format = format;
    imageInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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
    viewInfo.format = static_cast<vk::Format>(format);
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    image.imageView = device -> createImageView(viewInfo);
    image.format = format;
    textureViews[textureBufferId].push_back({.imageView=image.imageView});
    images.insert(std::make_pair(imageId, std::move(image)));
}



std::vector<TextureView>& TextureBufferManager::getTextureViews(std::string textureBufferId) {
    return textureViews[textureBufferId];
}

Image& TextureBufferManager::getImage(std::string &id) {
    return images.at(id);
}

void TextureBufferManager::cleanUp() const {
    for (auto & pair : images) {
        vmaDestroyImage(*allocator, pair.second.image, pair.second.allocation);
    }
}



