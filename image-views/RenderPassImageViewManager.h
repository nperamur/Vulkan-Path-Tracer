
#ifndef VULKAN_TEST_IMAGEVIEWMANAGER_H
#define VULKAN_TEST_IMAGEVIEWMANAGER_H
#include <string>
#include <vector>

#include "../shader-pipeline/ShaderPipeline.h"

namespace vk::raii {
    class ImageView;
}

class RenderPassImageViewManager {

    const VmaAllocator* allocator;
    vk::raii::Device* device;

    std::vector<Image> images;



    public:

    RenderPassImageViewManager(VmaAllocator* allocator, vk::raii::Device& device);

    Image& getImage(const std::string &id);

    void registerImage(std::string id, int width, int height);

    void resizeImage(std::string id, int width, int height);

    void cleanUp();
};



#endif //VULKAN_TEST_IMAGEVIEWMANAGER_H
