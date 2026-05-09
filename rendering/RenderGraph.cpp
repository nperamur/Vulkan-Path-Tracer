
#include "RenderGraph.h"

#include <barrier>
#include <iostream>
#include <set>
#include <utility>

#include "../scene/SceneManager.h"
/** Author: Neelesh Peramur  */


class SceneManager;


/**
 * The render graph takes in callbacks from the Renderer to execute the passes...
 * @param model the callback to render things to the screen
 * @param rays the callback to trace rays
 * @param transform the callback to update the transformations
 */
void RenderGraph::initCallbacks(std::function<void(Model&)> model, std::function<void()> rays, std::function<void()> transform) {
    if (!initialized) {
        this -> renderModel = std::move(model);
        this -> traceRays = std::move(rays);
        this -> updateTransformUniform = std::move(transform);
    }
}

/**
 * Begins a regular rasterized render pass.
 */
static inline void begin_render_pass(
    vk::CommandBuffer cmd,
    int numColorAttachments, std::vector<AbstractRenderPassImage>& colorViews,
    vk::ImageView const* depthView,
    vk::Extent2D extent) {

    std::vector<vk::RenderingAttachmentInfo> colorAttachments;
    for (int i = 0; i < numColorAttachments; i++) {
        std::visit([&colorAttachments](auto& renderPassImage) {
            vk::RenderingAttachmentInfo colorAttachment(
                renderPassImage.imageResource.getImageView(),
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ResolveModeFlagBits::eNone,
                {}, {},
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ClearValue{vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}}
            );

            colorAttachments.push_back(colorAttachment);
        }, colorViews[i]);

    }

    vk::RenderingAttachmentInfo depthAttachment;
    if (depthView) {
        depthAttachment = vk::RenderingAttachmentInfo(
            *depthView,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            vk::ResolveModeFlagBits::eNone,
            {}, {},
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0))
        );
    }

    vk::RenderingInfo renderingInfo(
        {},
        vk::Rect2D({0, 0}, extent),
        1, 0, numColorAttachments, colorAttachments.data(),
        depthView ? &depthAttachment : nullptr,
        nullptr
    );

    cmd.beginRendering(renderingInfo);

}

/**
 * Adds a new render pass to the pipeline. Note: all render passes are processed sequentially.
 * @param renderPass the render pass data/information
 */
void RenderGraph::addPass(RenderPass& renderPass) {
    renderPasses.push_back(std::move(renderPass));
}


/**
 * This method executes all the render passes in the order they were added to the graph.
 * @param barrierManager The barrier manager used for synchronization
 */
void RenderGraph::execute(vk::raii::CommandBuffer &commandBuffer, BarrierManager &barrierManager,
    SceneManager &sceneManager, Model &screenQuad, vk::ImageView const *swapChainDepthView, vk::raii::ImageView &swapChainImageView, vk::Extent2D swapChainExtent) {
    for (RenderPass& renderPass : renderPasses) {
        executeRenderPass(renderPass, commandBuffer, barrierManager, sceneManager, screenQuad, swapChainDepthView, swapChainImageView, swapChainExtent);
    }
    renderPasses.clear();
}

/**
 * Records execution commands for a single pass inside the command buffer, handling each RenderStage type
 */
void RenderGraph::executeRenderPass(RenderPass &renderPass, vk::raii::CommandBuffer &commandBuffer,
    BarrierManager &barrierManager, SceneManager &sceneManager, Model &screenQuad, vk::ImageView const *swapChainDepthView, vk::raii::ImageView &swapChainImageView, vk::Extent2D swapChainExtent) {

    if (renderPass.renderStage != RenderStage::copy) {
        resolveBarriers(renderPass, barrierManager, commandBuffer);
    }
    if (renderPass.renderStage == RenderStage::forward || renderPass.renderStage == RenderStage::postProcessing) {
        std::vector<AbstractRenderPassImage> attachments;
        int numAttachments = 0;
        for (auto& image : renderPass.writes) {
            std::visit([&attachments, &numAttachments](auto& renderPassImage) {
                if (renderPassImage.attachmentFormatType == AttachmentFormatType::color) {
                    attachments.push_back(renderPassImage);
                    numAttachments++;
                }
            }, image.get());
        }
        if (renderPass.renderStage == RenderStage::forward) {
            begin_render_pass(commandBuffer, numAttachments, attachments, swapChainDepthView, swapChainExtent);
        }
        if (renderPass.renderStage == RenderStage::postProcessing) {
            begin_render_pass(commandBuffer, numAttachments, attachments, nullptr, swapChainExtent);
        }
    }
    renderPass.bindPipeline();
    if (renderPass.renderStage == RenderStage::forward) {
        sceneManager.updateAndDrawEntities([this]() {
            updateTransformUniform();
        }, [this](Entity& entity) {
            renderModel(entity.getModel());
        });
    }
    if (renderPass.renderStage == RenderStage::raytracing) {
        traceRays();
    }
    if (renderPass.renderStage == RenderStage::postProcessing) {
        renderModel(screenQuad);
    }

    if (renderPass.renderStage == RenderStage::copy) {
        AbstractRenderPassImage& readAny = renderPass.reads.at(0);
        AbstractRenderPassImage& writeAny = renderPass.writes.at(0);

        std::visit([&barrierManager, &commandBuffer, &renderPass](auto& srcImage, auto& dstImage) {
            barrierManager.begin();

            if (srcImage.prevStage != BarrierUsage::transferRead) {
                barrierManager.transition(srcImage.imageResource.image, srcImage.prevStage != 0 ? srcImage.prevStage : BarrierUsage::colorNone, BarrierUsage::transferRead);
            }
            if (dstImage.prevStage != BarrierUsage::transferWrite) {
                barrierManager.transition(dstImage.imageResource.image, dstImage.prevStage != 0 ? dstImage.prevStage : BarrierUsage::colorNone, BarrierUsage::transferWrite);
            }
            barrierManager.commit(commandBuffer);
            vk::ImageSubresourceLayers subresource{
                vk::ImageAspectFlagBits::eColor,
                0,
                0,
                1
            };


            vk::ImageCopy2 imageCopy{
                subresource,
                vk::Offset3D{0, 0, 0},
                subresource,
                vk::Offset3D{0, 0, 0},
                vk::Extent3D{renderPass.width, renderPass.height, 1}
            };

            vk::CopyImageInfo2 copyImageInfo2(
                srcImage.imageResource.image,
                vk::ImageLayout::eTransferSrcOptimal,
                dstImage.imageResource.image,
                vk::ImageLayout::eTransferDstOptimal,
                1,
                &imageCopy
            );

            commandBuffer.copyImage2(copyImageInfo2);
            srcImage.prevStage = BarrierUsage::transferRead;
            dstImage.prevStage = BarrierUsage::transferWrite;
        }, readAny, writeAny);

    }

    if (renderPass.renderStage == RenderStage::forward || renderPass.renderStage == RenderStage::postProcessing) {
        commandBuffer.endRendering();
    }

    if (renderPass.toPresent) {
        AbstractRenderPassImage& any = renderPass.writes.at(0);
        std::visit([&barrierManager, &commandBuffer](auto& image) {
            barrierManager.begin();
            barrierManager.transition(image.imageResource.getImage(), image.prevStage, BarrierUsage::presentColor);
            barrierManager.commit(commandBuffer);
        }, any);
    }


}

/**
 * Specifies state transitions to the BarrierManager for both reads and writes based on the ImageType and AttachmentFormatType of the image
 */
void RenderGraph::resolveBarriers(RenderPass &renderPass, BarrierManager &barrierManager, vk::raii::CommandBuffer& commandBuffer) {
    barrierManager.begin();
    for (auto& image : renderPass.reads) {
        std::visit([&barrierManager](auto& renderPassImage) {
            uint32_t firstStage = 0;
            uint32_t secondStage = 0;

            if (renderPassImage.imageType == ImageType::attachment && renderPassImage.prevStage == 0) {
                renderPassImage.prevStage = (renderPassImage.attachmentFormatType == AttachmentFormatType::color) ? BarrierUsage::colorNone : BarrierUsage::depthNone;
            }


            if (renderPassImage.imageType == ImageType::storageImage && renderPassImage.prevStage == 0) {
                renderPassImage.prevStage = BarrierUsage::storageColorNone;

            }
            firstStage = renderPassImage.prevStage;
            secondStage = renderPassImage.attachmentFormatType == AttachmentFormatType::color ? BarrierUsage::colorRead : BarrierUsage::depthRead;

            barrierManager.transition(renderPassImage.imageResource.getImage(), firstStage, secondStage);
            renderPassImage.prevStage = secondStage;
        }, image.get());
    }


    for (auto& image : renderPass.writes) {
        std::visit([&barrierManager, &renderPass](auto& renderPassImage) {
            uint32_t firstStage = 0;
            uint32_t secondStage = 0;
            if (renderPassImage.imageType == ImageType::attachment && renderPassImage.prevStage == 0) {
                renderPassImage.prevStage = (renderPassImage.attachmentFormatType == AttachmentFormatType::color) ? BarrierUsage::colorNone : BarrierUsage::depthNone;
            }

            if (renderPassImage.imageType == ImageType::storageImage && renderPassImage.prevStage == 0) {
                renderPassImage.prevStage = BarrierUsage::storageColorNone;
            }
            firstStage = renderPassImage.prevStage;

            if (renderPassImage.imageType == ImageType::attachment) {
                secondStage = renderPassImage.attachmentFormatType == AttachmentFormatType::color ? BarrierUsage::colorWrite : BarrierUsage::depthWrite;
            } else if (renderPassImage.imageType == ImageType::storageImage) {
                secondStage = ResourceAccess::write | ResourceType::storageImage |
                    ((renderPassImage.attachmentFormatType == AttachmentFormatType::color) ? ResourceType::color : ResourceType::depth)
                    | getResourceStage(renderPass.renderStage);
            }

            barrierManager.transition(renderPassImage.imageResource.getImage(), firstStage, secondStage);
            renderPassImage.prevStage = secondStage;
        }, image.get());
    }
    barrierManager.commit(commandBuffer);
}


/**
 * Maps RenderGraph's RenderStage intent to BarrierManager's respective resource stage bits
 * @return the BarrierManager's bitwise flags associated with that particular RenderStage
 */
uint32_t RenderGraph::getResourceStage(RenderStage renderStage) {
    switch (renderStage) {
        case RenderStage::forward:
            return ResourceStage::fragmentShader;
        case RenderStage::postProcessing:
            return ResourceStage::fragmentShader;
        case RenderStage::raytracing:
            return ResourceStage::raytracing;
        case RenderStage::compute:
            return ResourceStage::compute;
        case RenderStage::copy:
            return ResourceStage::transferStage;
        default:
            return 0;
    }
}








