
#include "RenderPassImageViewManager.h"

/**
 * @Author: Neelesh Peramur
 * The purpose of this class is to manage the entire lifecycle of image, particularly for render pass attachments
 * Note: May expand this to be more than just render pass attachments in the future...
 */



RenderPassImageViewManager::RenderPassImageViewManager(VmaAllocator*allocator, vk::raii::Device &device) {
    this -> allocator = allocator;
    this -> device = &device;
}


/**
 * Note: registerImage must be called first before this method is ever called.
 * Retrieves the registered image for a given frameIndex and identifier.
 */
Image& RenderPassImageViewManager::getImage(const std::string& id, int frameIndex) {
    for (auto & image : images) {
        if (image.identifier == id && image.frameIndex == frameIndex) {
            return image;
        }
    }
    throw std::runtime_error("Failed to get image. Invalid id");
}



/**
 * Registers a new image given the dimensions, format, and a registration identifier.
 * Allocates as many images as needed based on the number of in-flight frames
 */
void RenderPassImageViewManager::registerImage(std::string id, VkFormat format, int width, int height) {
    for (int i = 0; i < Config::maxFramesInFlight; i++) {
        Image image = {.imageView = nullptr, .sampler = nullptr};
        image.identifier = id;
        image.frameIndex = i;
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
        textureViews[id][i] = {.imageView=image.imageView};
        images.push_back(std::move(image));
    }
}


/**
 * Resizes the image by destroying all the images and re-creating them.
 */
void RenderPassImageViewManager::resizeImage(std::string id, int width, int height) {
    VkFormat format;
    for (int i = 0; i < images.size(); i += Config::maxFramesInFlight) {
        if (images[i].identifier == id) {
            device->waitIdle();
            format = images[i].format;
            for (int j = 0; j < Config::maxFramesInFlight; j++) {
                vmaDestroyImage(*allocator, images[i + j].image, images[i + j].allocation);
            }
            images.erase(images.begin() + i, images.begin() + i + Config::maxFramesInFlight);
            break;
        }
    }

    registerImage(id, format, width, height);
}

TextureView* RenderPassImageViewManager::getTextureView(std::string id, int frameIndex) {
    return &textureViews[id][frameIndex];
}


void RenderPassImageViewManager::cleanUp() const {
    for (auto & image : images) {
        vmaDestroyImage(*allocator, image.image, image.allocation);
    }
}


