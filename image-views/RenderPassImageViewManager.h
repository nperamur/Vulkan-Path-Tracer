
#ifndef VULKAN_TEST_IMAGEVIEWMANAGER_H
#define VULKAN_TEST_IMAGEVIEWMANAGER_H
#include <string>
#include <vector>

#include "../VulkanCommon.h"
#include "../shader-pipeline/ShaderPipeline.h"

namespace vk::raii {
    class ImageView;
}

class RenderPassImageViewManager {

    const VmaAllocator* allocator;
    vk::raii::Device* device;

    std::vector<Image> images;

    std::unordered_map<std::string, std::array<TextureView, Config::maxFramesInFlight>> textureViews;



    public:

    RenderPassImageViewManager(VmaAllocator* allocator, vk::raii::Device& device);

    Image& getImage(const std::string &id, int frameIndex);

    void registerImage(std::string id, VkFormat format, int width, int height);

    void resizeImage(std::string id, int width, int height);

    TextureView* getTextureView(std::string id, int frameIndex);

    void cleanUp() const;
};



#endif //VULKAN_TEST_IMAGEVIEWMANAGER_H
