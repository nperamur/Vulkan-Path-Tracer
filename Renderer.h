#ifndef VULKAN_TEST_RENDERER_H
#define VULKAN_TEST_RENDERER_H
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <complex.h>

#include "Loader.h"
#include "shader-pipeline/ShaderPipelineRegistry.h"
#include "vk_mem_alloc.h"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "Entity.h"
#include "shader-pipeline/RaytracingShaderPipeline.h"

struct MVP {
    glm::mat4 transformation;
    glm::mat4 view;
    glm::mat4 projection;
};

struct Light {
    glm::vec4 position;
    glm::vec4 color;
};

class Renderer {
    ShaderPipelineRegistry* shaderPipelineRegistry;
    vk::raii::Device* device;
    const VmaAllocator* allocator;

    VkBuffer instanceBuffer;
    VmaAllocation instanceAllocation;

    const vk::Format* swapChainImageFormat;
    const vk::Extent2D* swapChainExtent;

    std::vector<Entity> entities;

    std::optional<vk::RenderingInfo> renderingInfo;
    Light light;
    Loader loader;

    MVP mvp;
    Geometry geometry;



    public:

    Renderer(ShaderPipelineRegistry &shaderPipelineRegistry, vk::raii::Device &device, vk::Format &swapChainImageFormat, vk::Extent2D& swapChainExtent, vk::raii::PhysicalDevice& physicalDevice, VmaAllocator& allocator);

    void render(vk::raii::CommandBuffer &commandBuffer, vk::raii::ImageView &imageView, vk::raii::ImageView &depthImageView, vk::Image
                &, VkImage depthImage, int frameIndex);

    void cleanUp();

    void buildBLASGeometry();

    void buildTLASGeometry(std::vector<AccelerationStructureData> blasData);

private:

    void renderModel(vk::raii::CommandBuffer &commandBuffer, Model &model, vk::Viewport viewport, vk::Rect2D rect2D);

    void presentationMemoryBarrier(vk::raii::CommandBuffer &commandBuffer, vk::Image &image, VkImage &depthImage);

    void renderingMemoryBarrier(vk::raii::CommandBuffer &commandBuffer, vk::Image &image, VkImage &depthImage);
    glm::mat4 createProjectionMatrix();
};



#endif //VULKAN_TEST_RENDERER_H
