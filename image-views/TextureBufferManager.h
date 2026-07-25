

#ifndef VULKAN_PATHTRACER_TEXTUREBUFFERMANAGER_H
#define VULKAN_PATHTRACER_TEXTUREBUFFERMANAGER_H
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "vk_mem_alloc.h"
#include "../shader-pipeline/ShaderPipeline.h"


namespace vk::raii {
    class Device;
}

struct TextureView;

class TextureBufferManager {
    const VmaAllocator* allocator;
    vk::raii::Device* device;
    std::unordered_map<std::string, std::vector<TextureView>> textureViews;
    std::unordered_map<std::string, Image> images;

    public:
        TextureBufferManager(VmaAllocator* allocator, vk::raii::Device& device);

        void registerTextureBuffer(std::string id);

        void registerImage(std::string imageId, std::string textureBufferId, VkFormat format, int width, int height);
        std::vector<TextureView>& getTextureViews(std::string bufferId);
        Image& getImage(std::string& id);

        void cleanUp() const;


};



#endif //VULKAN_PATHTRACER_TEXTUREBUFFERMANAGER_H
