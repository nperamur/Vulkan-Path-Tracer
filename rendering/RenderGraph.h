

#ifndef VULKAN_TEST_RENDERGRAPH_H
#define VULKAN_TEST_RENDERGRAPH_H
#include <functional>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "../syncronization/BarrierManager.h"

#include "../Loader.h"
#include "../shader-pipeline/ShaderPipeline.h"

class Entity;
class SceneManager;

enum class ImageType {
    storageImage,
    attachment
};

enum class AttachmentFormatType {
    color,
    depth
};

enum class RenderStage {
    forward,
    postProcessing,
    raytracing,
    compute,
    transfer
};




template<ImageHandleProvider T>
struct RenderPassImage {
        uint32_t prevStage = 0;

        AttachmentFormatType attachmentFormatType;
        ImageType imageType;
        T& imageResource;

    RenderPassImage(AttachmentFormatType attachmentFormatType,
                ImageType type,
                T& img)
    : attachmentFormatType(attachmentFormatType),
      imageType(type),
      imageResource(img){}
};



using AbstractRenderPassImage =
    std::variant<RenderPassImage<Image>, RenderPassImage<ImageReference>>;

struct RenderPass {
    RenderStage renderStage;
    std::vector<std::reference_wrapper<AbstractRenderPassImage>> reads;
    std::vector<std::reference_wrapper<AbstractRenderPassImage>> writes;
    std::function<void()> bindPipeline;
    bool toPresent = false;
};

class RenderGraph {
    bool initialized = false;
    std::vector<RenderPass> renderPasses;
    std::function<void(Model&)> renderModel;
    std::function<void()> traceRays;
    std::function<void()> updateTransformUniform;

    public:

        template<ImageHandleProvider T>
        AbstractRenderPassImage colorAttachment(T& image) {
            return RenderPassImage(AttachmentFormatType::color, ImageType::attachment, image);
        }

        template<ImageHandleProvider T>
        AbstractRenderPassImage depthAttachment(T& image) {
            return RenderPassImage(AttachmentFormatType::depth, ImageType::attachment, image);
        }

        template<ImageHandleProvider T>
        AbstractRenderPassImage storageImage(T& image) {
            return RenderPassImage(AttachmentFormatType::color, ImageType::storageImage, image);
        }

        void initCallbacks(std::function<void(Model &)> renderModel, std::function<void()> traceRays,
                       std::function<void()> updateTransformUniform);

        void addPass(RenderPass& renderPass);
        void execute(vk::raii::CommandBuffer& commandBuffer, BarrierManager &barrierManager, SceneManager &sceneManager, Model &screenQuad, vk::ImageView const* swapChainDepthView, vk::raii::ImageView &swapChainImageView,
                                    vk::Extent2D swapChainExtent);

    private:
    void executeRenderPass(RenderPass& renderPass, vk::raii::CommandBuffer &commandBuffer, BarrierManager& barrierManager,
        SceneManager &sceneManager, Model &screenQuad, vk::ImageView const *swapChainDepthView, vk::raii::ImageView &swapChainImageView, vk::Extent2D swapChainExtent);


    static void resolveBarriers(RenderPass& renderPass, BarrierManager& barrierManager, vk::raii::CommandBuffer& commandBuffer);
    static uint32_t getResourceStage(RenderStage renderStage);
};



#endif //VULKAN_TEST_RENDERGRAPH_H
