#include "Renderer.h"

#include "Application.h"
#include "Loader.h"
#include "ModelLoader.h"
#include "GLFW/glfw3.h"
#include "glm/fwd.hpp"
#include "glm/vec4.hpp"
#include "glm/glm.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtx/dual_quaternion.hpp"
#include "shader-pipeline/RaytracingShaderPipeline.h"
#include "VulkanCommon.h"

static inline void begin_render_pass(
    vk::CommandBuffer cmd,
    vk::ImageView colorView,
    vk::ImageView const* depthView,
    vk::Extent2D extent)
{
    vk::RenderingAttachmentInfo colorAttachment(
        colorView,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ResolveModeFlagBits::eNone,
        {}, {},
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::ClearValue{vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}}
    );

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
        1, 0, 1, &colorAttachment,
        depthView ? &depthAttachment : nullptr,
        nullptr
    );

    cmd.beginRendering(renderingInfo);
}

int numFramesSinceResize = 0;


static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    numFramesSinceResize = 0;
}

Renderer::Renderer(ShaderPipelineRegistry &shaderPipelineRegistry, vk::raii::Device& device, vk::Format& swapChainImageFormat,
                   vk::Extent2D& swapChainExtent, vk::raii::PhysicalDevice& physicalDevice, VmaAllocator* allocator) {
    this -> shaderPipelineRegistry = &shaderPipelineRegistry;
    this -> swapChainImageFormat = &swapChainImageFormat;
    this -> swapChainExtent = &swapChainExtent;
    this -> device = &device;
    DescriptorsInfo triangleDescriptorsInfo = {
        .staticData = {.numUBOs = 1, .numTextureSamplers = 0},
        .dynamicData = {.numUBOs = 2, .numTextureSamplers = 0}
    };
    this -> imageViewManager.emplace(allocator, device);
    this -> barrierManager.emplace();

    glfwSetFramebufferSizeCallback(Application::get() -> getWindow(), framebufferResizeCallback);
    this -> shaderPipelineRegistry -> registerShaderPipeline(std::make_unique<ShaderPair>(Shaders::triangle, device, swapChainImageFormat, *allocator, triangleDescriptorsInfo, 1));
    std::vector<float> triangleVertices = {
        0.0f, -1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,
       -1.0f,  1.0f, 0.0f,
    };
    std::vector<float> triangleNormals = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
    };
    std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 1};
    //triangleEntity.emplace(loader.load(triangleVertices, indices, triangleNormals, device, physicalDevice), mvp.transformation);
    this -> mvp = {.transformation = glm::mat4(1.0f), .view = Application::get() -> getCamera().createViewMatrix(), .projection = createProjectionMatrix()};
    this -> inverseViewProj = {.inverseView = glm::inverse(Application::get() -> getCamera().createViewMatrix()), .inverseProj = glm::inverse(createProjectionMatrix())};

    ModelLoader modelLoader;
    entities.emplace_back("sponza", modelLoader.load("sponza", loader, device, physicalDevice), mvp.transformation);
    this->allocator = allocator;


    light.color = glm::vec4(1.0, 0.95, 0.8, 1.0);
    light.position = glm::vec4(500.0, 800.0, 300.0, 1.0);

    this -> shaderPipelineRegistry -> getShaderPipeline(Shaders::triangle) -> setUniform({.set = 0, .binding = 0}, &light, sizeof(light),  0);

    //raytracing
    // this -> geometry = {};
    DescriptorsInfo raytracingDescriptorsInfo = {
        .staticData = {.numTextureSamplers = 1, .numAccelerationStructures = 1, .numStorageImages = 1},
        .dynamicData = {.numUBOs = 2}
    };
    this->accelStructureManager = AccelerationStructureManager(device, physicalDevice, &*allocator);
    accelStructureManager -> build(entities, mvp);
    std::unique_ptr<RaytracingShaderPipeline> rtShaderPipeline = std::make_unique<RaytracingShaderPipeline>(Shaders::raytracing, device, physicalDevice, swapChainImageFormat, *allocator, raytracingDescriptorsInfo);
    this -> shaderPipelineRegistry -> registerShaderPipeline(std::move(rtShaderPipeline));
    RaytracingShaderPipeline* rtShader = dynamic_cast<RaytracingShaderPipeline*> (this -> shaderPipelineRegistry -> getShaderPipeline(Shaders::raytracing).get());

    for (int i = 0; i < 3; i++) {
        rtShader->setAccelerationStructure({0, 0}, accelStructureManager -> getTLASData(), i);
    }

    int width, height;
    glfwGetFramebufferSize(Application::get() -> getWindow(), &width, &height);
    imageViewManager -> registerImage(RenderPassImages::baseForwardPass, width, height);



}



void Renderer::render(vk::raii::CommandBuffer &commandBuffer, vk::raii::ImageView &imageView, vk::raii::ImageView &depthImageView, vk::Image &image, VkImage depthImage, int frameIndex) {
    //TODO: add secondary command buffer support later
    RaytracingShaderPipeline* rtShaderPipeline = dynamic_cast<RaytracingShaderPipeline*> (shaderPipelineRegistry->getShaderPipeline(Shaders::raytracing).get());
    ShaderPair* triangleShader = dynamic_cast<ShaderPair*> (shaderPipelineRegistry->getShaderPipeline(Shaders::triangle).get());
    int width, height;
    glfwGetFramebufferSize(Application::get() -> getWindow(), &width, &height);
    if (numFramesSinceResize == 0) {
        device->waitIdle();
        Application::get() -> resetAllCommandBuffers();

        this->depthTextureView.reset();
        this->depthTextureView.emplace(TextureView{.imageView = depthImageView});
        this->imageViewManager->resizeImage(RenderPassImages::baseForwardPass, width, height);
    }

    if (numFramesSinceResize < 3) {
        rtShaderPipeline -> setTextureSampler({0, 1}, *depthTextureView, vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal,frameIndex);
        rtShaderPipeline -> setStorageImage({0, 2}, width, height, frameIndex, StorageImages::raytracingOutput);
    }
    numFramesSinceResize++;

    mvp.projection = createProjectionMatrix();
    vk::Rect2D rect2D(
    {0, 0}, *swapChainExtent
    );
    vk::Viewport viewport(
        0.0f, 0.0f, (float)swapChainExtent->width,
        (float)swapChainExtent->height, 0.0f, 1.0f
    );
    vk::CommandBufferBeginInfo beginInfo(
        {},
        nullptr
    );
    commandBuffer.begin(beginInfo);
    renderingMemoryBarrier(commandBuffer, image, depthImage);


    //base forward pass
    begin_render_pass(commandBuffer, *imageView, &*depthImageView, *swapChainExtent);
    triangleShader -> bind(commandBuffer, frameIndex);

    float time = glfwGetTime();


    struct TriangleUBO {
        glm::vec4 color;
    };
    TriangleUBO data;
    data.color = glm::vec4(std::sin(time * 1.0f) * 0.5f + 0.5f, std::sin(time * 1.3f) * 0.5f + 0.5f, std::sin(time * 1.7f) * 0.5f + 0.5f, 1.0f);
    triangleShader -> setUniform({.set = 1,.binding = 0}, &data, sizeof(data), frameIndex);


    mvp.view = Application::get() -> getCamera().createViewMatrix();
    for (Entity& entity : entities) {
        if (entity.getIdentifier() == "triangle") {
            entity.setRotation(glm::vec3(0.0f, time, 0.0f));
        } else if (entity.getIdentifier() == "sponza") {
            entity.setScale(glm::vec3(0.02, 0.02, 0.02));
        }
        entity.updateTransformationMatrix();
        triangleShader -> setUniform({.set = 1, .binding = 1}, &mvp, sizeof(mvp), frameIndex);
        renderModel(commandBuffer, entity.getModel(), viewport, rect2D);

    }


    commandBuffer.endRendering();

    inverseViewProj.inverseProj = glm::inverse(mvp.projection);
    inverseViewProj.inverseView = glm::inverse(mvp.view);


    //raytracing
    rtShaderPipeline -> bind(commandBuffer, frameIndex);
    rtShaderPipeline -> setUniform({.set = 1, .binding = 0}, &light, sizeof(light), frameIndex);
    rtShaderPipeline -> setUniform({.set = 1, .binding = 1}, &inverseViewProj, sizeof(inverseViewProj), frameIndex);
    barrierManager -> begin();
    barrierManager -> transition(depthImage, BarrierUsage::depthWrite, BarrierUsage::depthRead);
    barrierManager -> commit(commandBuffer);
    this -> traceRays(commandBuffer, viewport, rect2D, width, height, 1);

    presentationMemoryBarrier(commandBuffer, image, depthImage);

    commandBuffer.end();
}


void Renderer::renderModel(vk::raii::CommandBuffer& commandBuffer, Model& model, vk::Viewport viewport, vk::Rect2D rect2D) {
    //bind viewport + scissor
    VkDeviceSize offsets[] = {0};
    commandBuffer.setViewport(0, viewport);
    commandBuffer.setScissor(0, rect2D);
    commandBuffer.bindVertexBuffers(0, **model.vertexBuffer, offsets);
    if (model.normalBuffer.has_value()) {
        commandBuffer.bindVertexBuffers(1, **model.normalBuffer, offsets);
    }
    if (model.numIndices > 0) {
        commandBuffer.bindIndexBuffer(**model.indexBuffer, 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(model.numIndices, 1, 0, 0, 0);
    } else {
        commandBuffer.draw(model.numVertices, 1, 0, 0);

    }

}

void Renderer::traceRays(vk::raii::CommandBuffer &commandBuffer, vk::Viewport viewport, vk::Rect2D rect2D, int width, int height, int depth) {
    commandBuffer.setViewport(0, viewport);
    commandBuffer.setScissor(0, rect2D);
    if (width == 0 || height == 0) return;
    RaytracingShaderPipeline* rtShaderPipeline = dynamic_cast<RaytracingShaderPipeline*> (shaderPipelineRegistry->getShaderPipeline(Shaders::raytracing).get());
    commandBuffer.traceRaysKHR(*rtShaderPipeline -> getRegion(RaytracingRegion::rayGen),
                                *rtShaderPipeline -> getRegion(RaytracingRegion::miss),
                                *rtShaderPipeline -> getRegion(RaytracingRegion::closestHit),
                                *rtShaderPipeline -> getRegion(RaytracingRegion::callable), width, height, depth);
}


void Renderer::renderingMemoryBarrier(vk::raii::CommandBuffer& commandBuffer, vk::Image& image, VkImage& depthImage) {
    RaytracingShaderPipeline* rtShader = dynamic_cast<RaytracingShaderPipeline*> (shaderPipelineRegistry -> getShaderPipeline(Shaders::raytracing).get());
    barrierManager -> begin();
    barrierManager -> transition(image, BarrierUsage::colorInitial, BarrierUsage::colorWrite);
    barrierManager -> transition(depthImage, BarrierUsage::depthInitial, BarrierUsage::depthWrite);
    for (const Image& storageImage : rtShader->getStorageImages()) {
        barrierManager -> transition(storageImage.image,
            ResourceAccess::none | ResourceType::color | ResourceStages::allCommands,
            ResourceAccess::write | ResourceType::storageImage | ResourceType::color | ResourceStages::raytracing);

    }
    barrierManager -> commit(commandBuffer);

}



//TODO: If I add refitting, add a memory barrier for acceleration structures
void Renderer::presentationMemoryBarrier(vk::raii::CommandBuffer& commandBuffer, vk::Image& image, VkImage& depthImage) {
    RaytracingShaderPipeline* rtShader = dynamic_cast<RaytracingShaderPipeline*> (shaderPipelineRegistry -> getShaderPipeline(Shaders::raytracing).get());
    barrierManager -> begin();
    barrierManager -> transition(image, BarrierUsage::colorWrite, BarrierUsage::presentColor);
    barrierManager -> transition(depthImage, BarrierUsage::depthRead, BarrierUsage::presentDepth);
    for (const Image& storageImage : rtShader->getStorageImages()) {
        //TODO: change all commands
        barrierManager -> transition(storageImage.image,
        ResourceAccess::write | ResourceType::storageImage | ResourceType::color | ResourceStages::raytracing,
        ResourceAccess::none | ResourceType::storageImage | ResourceType::color | ResourceStages::allCommands);

    }
    barrierManager -> commit(commandBuffer);
}



void Renderer::cleanUp() {
    this -> shaderPipelineRegistry -> cleanUp();
    this -> accelStructureManager -> cleanUp();
    this -> imageViewManager -> cleanUp();
}



glm::mat4 Renderer::createProjectionMatrix() {
    float fov = glm::radians(70.0f);
    int width, height;
    glfwGetFramebufferSize(Application::get() -> getWindow(), &width, &height);
    if (width == 0 || height == 0) return glm::mat4();
    float aspect = (float)width / (float)height;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;


    glm::mat4 proj = glm::perspective(fov, aspect, nearPlane, farPlane);
    proj[1][1] *= -1;
    return proj;
}

