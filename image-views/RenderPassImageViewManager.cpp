
#include "RenderPassImageViewManager.h"





void RenderPassImageViewManager::registerImage(std::string id, VkFormat format, int width, int height) {
    Image image = {.imageView = nullptr, .sampler = nullptr};
    image.identifier = id;
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
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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
    this -> images.push_back(std::move(image));
}

void RenderPassImageViewManager::resizeImage(std::string id, int width, int height) {
    VkFormat format;
    for (int i = 0; i < images.size(); i++) {
        if (images[i].identifier == id) {
            device->waitIdle();
            format = images[i].format;
            vmaDestroyImage(*allocator, images[i].image, images[i].allocation);
            images.erase(images.begin() + i);
            break;
        }
    }

    registerImage(id, format, width, height);
}

RenderPassImageViewManager::RenderPassImageViewManager(VmaAllocator*allocator, vk::raii::Device &device) {
    this -> allocator = allocator;
    this -> device = &device;
}

Image& RenderPassImageViewManager::getImage(const std::string& id) {
    for (auto & image : images) {
        if (image.identifier == id) {
            return image;
        }
    }
    throw std::runtime_error("Failed to get image. Invalid id");
}



void RenderPassImageViewManager::cleanUp() {
    for (auto & image : images) {
        vmaDestroyImage(*allocator, image.image, image.allocation);
    }
}


