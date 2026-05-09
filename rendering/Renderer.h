#ifndef VULKAN_TEST_RENDERER_H
#define VULKAN_TEST_RENDERER_H
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <complex.h>

#include "../Loader.h"
#include "../shader-pipeline/ShaderPipelineRegistry.h"
#include "vk_mem_alloc.h"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "../shader-pipeline/RaytracingShaderPipeline.h"
#include "../raytracing/AccelerationStructureManager.h"
#include "../VulkanCommon.h"
#include "../image-views/RenderPassImageViewManager.h"
#include "../scene/SceneManager.h"
#include "RenderGraph.h"

#include "../syncronization/BarrierManager.h"
#define ID(name, str) inline constexpr const char* name = str;

namespace Shaders {
    ID(triangle, "triangle")
    ID(raytracing, "raytracing")
    ID(combineShader, "combineShader")
}

namespace StorageImages {
    ID(raytracingOutput, "raytracingOutput")
}

namespace RenderPassImages {
    ID(baseForwardPass, "baseForwardPass")
    ID(historyBuffer, "historyBuffer")
    ID(blendOutput, "blendOutput")
    ID(visibilityBuffer, "visiblityBuffer")
}


struct InverseViewProj {
    alignas(16) glm::mat4 inverseView;
    alignas(16) glm::mat4 inverseProj;
};



struct Light {
    glm::vec4 position;
    glm::vec4 color;
};

class Renderer {
    ShaderPipelineRegistry* shaderPipelineRegistry;
    vk::raii::Device* device;
    const VmaAllocator* allocator;
    std::optional<BarrierManager> barrierManager;

    // VkBuffer instanceBuffer;
    // VmaAllocation instanceAllocation;

    const vk::Format* swapChainImageFormat;
    const vk::Extent2D* swapChainExtent;

    std::optional<SceneManager> sceneManager;

    std::vector<Material> materials;

    std::optional<vk::RenderingInfo> renderingInfo;
    Light light;
    Loader loader;

    MVP mvp;
    RenderGraph renderGraph;
    // Geometry geometry;

    std::array<TextureView, Config::maxFramesInFlight> depthTextureViews;
    std::array<TextureView, Config::maxFramesInFlight> rtTextureViews;

    InverseViewProj inverseViewProj;

    std::optional<AccelerationStructureManager> accelStructureManager;

    std::optional<RenderPassImageViewManager> imageViewManager;


    Model screenQuad;


    public:

    Renderer(ShaderPipelineRegistry &shaderPipelineRegistry, vk::raii::Device &device, vk::Format &swapChainImageFormat, vk::Extent2D& swapChainExtent, vk::raii::PhysicalDevice& physicalDevice, VmaAllocator* allocator);

    void render(vk::raii::CommandBuffer &commandBuffer, vk::raii::ImageView &swapChainImageView, vk::raii::ImageView &depthImageView, vk::Image
                &, VkImage depthImage, int frameIndex);

    void cleanUp();


private:
    void renderModel(vk::raii::CommandBuffer &commandBuffer, Model &model, vk::Viewport viewport, vk::Rect2D rect2D);
    void resizeImageViews(ShaderPair* combineShaders, RaytracingShaderPipeline* rtShaderPipeline, vk::raii::ImageView& depthImageView, int width, int height, int frameIndex);
    glm::mat4 createProjectionMatrix();
    void traceRays(vk::raii::CommandBuffer &commandBuffer, vk::Viewport viewport, vk::Rect2D rect2D, int width, int height, int depth);
    void initScreenQuad(vk::raii::PhysicalDevice& physicalDevice);
};



#endif //VULKAN_TEST_RENDERER_H
