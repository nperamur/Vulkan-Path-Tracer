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
    int numColorAttachments, std::vector<vk::ImageView> colorViews,
    vk::ImageView const* depthView,
    vk::Extent2D extent) {

    std::vector<vk::RenderingAttachmentInfo> colorAttachments;
    for (int i = 0; i < numColorAttachments; i++) {
        vk::RenderingAttachmentInfo colorAttachment(
            colorViews[i],
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ResolveModeFlagBits::eNone,
            {}, {},
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::ClearValue{vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}}
        );

        colorAttachments.push_back(colorAttachment);

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
    this -> allocator = allocator;
    this -> mvp = {.transformation = glm::mat4(1.0f), .view = Application::get() -> getCamera().createViewMatrix(), .projection = createProjectionMatrix()};
    this -> inverseViewProj = {.inverseView = glm::inverse(Application::get() -> getCamera().createViewMatrix()), .inverseProj = glm::inverse(createProjectionMatrix())};
    this -> imageViewManager.emplace(allocator, device);
    this -> barrierManager.emplace();
    light.color = glm::vec4(1.0, 0.95, 0.8, 1.0);
    light.position = glm::vec4(500.0, 800.0, 300.0, 1.0);

    DescriptorsInfo triangleDescriptorsInfo = {
        .staticData = {.numUBOs = 1, .numTextureSamplers = 0},
        .dynamicData = {.numUBOs = 2, .numTextureSamplers = 0}
    };
    glfwSetFramebufferSizeCallback(Application::get() -> getWindow(), framebufferResizeCallback);
    sceneManager.emplace(loader, device, physicalDevice, mvp);

    this -> shaderPipelineRegistry -> registerShaderPipeline(std::make_unique<ShaderPair>(Shaders::triangle, device,
                                            std::vector<vk::Format>{swapChainImageFormat}, *allocator, triangleDescriptorsInfo, 1));

    this -> shaderPipelineRegistry -> getShaderPipeline(Shaders::triangle) -> setUniform({.set = 0, .binding = 0}, &light, sizeof(light),  0);

    //raytracing
    DescriptorsInfo raytracingDescriptorsInfo = {
        .staticData = {.numTextureSamplers = 1, .numAccelerationStructures = 1, .numStorageImages = 1},
        .dynamicData = {.numUBOs = 2}
    };
    this->accelStructureManager = AccelerationStructureManager(device, physicalDevice, &*allocator);
    accelStructureManager -> build(sceneManager -> getEntities(), mvp);
    std::unique_ptr<RaytracingShaderPipeline> rtShaderPipeline = std::make_unique<RaytracingShaderPipeline>(Shaders::raytracing, device,
                                                            physicalDevice, *allocator, raytracingDescriptorsInfo);
    this -> shaderPipelineRegistry -> registerShaderPipeline(std::move(rtShaderPipeline));
    RaytracingShaderPipeline* rtShader = dynamic_cast<RaytracingShaderPipeline*> (this -> shaderPipelineRegistry -> getShaderPipeline(Shaders::raytracing).get());

    for (int i = 0; i < Config::maxFramesInFlight; i++) {
        rtShader->setAccelerationStructure(RTShaderSlots::accelerationStructure, accelStructureManager -> getTLASData(), i);
    }

    DescriptorsInfo combineShadersDescriptorsInfo = {
        .staticData = {.numUBOs = 0, .numTextureSamplers = 2},
        .dynamicData = {.numUBOs = 0, .numTextureSamplers = 0}
    };
    this -> shaderPipelineRegistry -> registerShaderPipeline(std::make_unique<ShaderPair>(Shaders::combineShader, device,
                                    std::vector<vk::Format>{swapChainImageFormat, vk::Format::eR32Uint}, *allocator, combineShadersDescriptorsInfo, 1));



    int width, height;
    glfwGetFramebufferSize(Application::get() -> getWindow(), &width, &height);
    imageViewManager -> registerImage(RenderPassImages::baseForwardPass, VK_FORMAT_B8G8R8A8_SRGB, width, height);

    initScreenQuad(physicalDevice);

}



void Renderer::render(vk::raii::CommandBuffer &commandBuffer, vk::raii::ImageView &swapChainImageView, vk::raii::ImageView &depthImageView, vk::Image &swapChainImage, VkImage depthImage, int frameIndex) {
    //TODO: add secondary command buffer support later
    RaytracingShaderPipeline* rtShaderPipeline = dynamic_cast<RaytracingShaderPipeline*> (shaderPipelineRegistry->getShaderPipeline(Shaders::raytracing).get());
    ShaderPair* triangleShader = dynamic_cast<ShaderPair*> (shaderPipelineRegistry->getShaderPipeline(Shaders::triangle).get());
    ShaderPair* combineShaders = dynamic_cast<ShaderPair*> (shaderPipelineRegistry->getShaderPipeline(Shaders::combineShader).get());

    int width, height;
    glfwGetFramebufferSize(Application::get() -> getWindow(), &width, &height);
    resizeImageViews(combineShaders, rtShaderPipeline, depthImageView, width, height, frameIndex);


    Image* baseForwardPassImage = &(this -> imageViewManager -> getImage(RenderPassImages::baseForwardPass, frameIndex));

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


    barrierManager -> begin();
    barrierManager -> transition(baseForwardPassImage -> image, BarrierUsage::colorNone, BarrierUsage::colorWrite);
    barrierManager -> transition(depthImage, BarrierUsage::depthNone, BarrierUsage::depthWrite);
    barrierManager -> commit(commandBuffer);

    //Base Forward Pass
    begin_render_pass(commandBuffer, 1, {baseForwardPassImage -> imageView}, &*depthImageView, *swapChainExtent);
    triangleShader -> bind(commandBuffer, frameIndex);

    float time = glfwGetTime();

    struct TriangleUBO {
        glm::vec4 color;
    };
    TriangleUBO data;
    data.color = glm::vec4(std::sin(time * 1.0f) * 0.5f + 0.5f, std::sin(time * 1.3f) * 0.5f + 0.5f, std::sin(time * 1.7f) * 0.5f + 0.5f, 1.0f);
    triangleShader -> setUniform(ForwardPassShaderSlots::triangleUBO, &data, sizeof(data), frameIndex);

    mvp.view = Application::get() -> getCamera().createViewMatrix();

    sceneManager -> updateAndDrawEntities([this, triangleShader, frameIndex]() {
        triangleShader -> setUniform(ForwardPassShaderSlots::mvp, &mvp, sizeof(mvp), frameIndex);
    }, [&commandBuffer, viewport, rect2D, this](Entity& entity) {
        renderModel(commandBuffer, entity.getModel(), viewport, rect2D);
    });


    commandBuffer.endRendering();
    mvp.view = Application::get() -> getCamera().createViewMatrix();
    glm::dmat4 dProj = mvp.projection;
    glm::dmat4 dView = mvp.view;
    inverseViewProj.inverseProj = glm::inverse(dProj);
    inverseViewProj.inverseView = glm::inverse(dView);

    //Raytracing Pass
    rtShaderPipeline -> bind(commandBuffer, frameIndex);
    rtShaderPipeline -> setUniform(RTShaderSlots::lightUBO, &light, sizeof(light), frameIndex);
    rtShaderPipeline -> setUniform(RTShaderSlots::inverseViewProj, &inverseViewProj, sizeof(inverseViewProj), frameIndex);

    barrierManager -> begin();
    barrierManager -> transition(depthImage, BarrierUsage::depthWrite, BarrierUsage::depthRead);
    barrierManager -> transition(rtShaderPipeline -> getUniformBuffer(RTShaderSlots::inverseViewProj, frameIndex),
         sizeof(inverseViewProj),  ResourceStages::hostStage | ResourceAccess::write | ResourceType::ubo,
         ResourceAccess::read | ResourceType::ubo | ResourceStages::raytracing);

    barrierManager -> transition(rtShaderPipeline -> getStorageImage(StorageImages::raytracingOutput, frameIndex).image,
            ResourceAccess::none | ResourceType::color | ResourceStages::allCommands,
            ResourceAccess::write | ResourceType::storageImage | ResourceType::color | ResourceStages::raytracing);

    barrierManager -> commit(commandBuffer);
    this -> traceRays(commandBuffer, viewport, rect2D, width, height, 1);


    barrierManager -> begin();
    barrierManager -> transition(baseForwardPassImage -> image, BarrierUsage::colorWrite, BarrierUsage::colorRead);
    barrierManager -> transition(swapChainImage, BarrierUsage::colorNone, BarrierUsage::colorWrite);
    barrierManager -> transition(rtShaderPipeline -> getStorageImage(StorageImages::raytracingOutput, frameIndex).image,
            ResourceAccess::write | ResourceType::storageImage | ResourceType::color | ResourceStages::raytracing,
            BarrierUsage::colorRead);

    barrierManager -> commit(commandBuffer);

    //Post-process pass
    begin_render_pass(commandBuffer, 1, {swapChainImageView}, nullptr, *swapChainExtent);
    combineShaders -> bind(commandBuffer, frameIndex);
    renderModel(commandBuffer, screenQuad, viewport, rect2D);
    commandBuffer.endRendering();


    barrierManager -> begin();
    barrierManager -> transition(swapChainImage, BarrierUsage::colorWrite, BarrierUsage::presentColor);
    barrierManager -> commit(commandBuffer);

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
    if (model.textureCoordBuffer.has_value()) {
        commandBuffer.bindVertexBuffers(2, **model.textureCoordBuffer, offsets);
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

void Renderer::resizeImageViews(ShaderPair* combineShaders, RaytracingShaderPipeline* rtShaderPipeline, vk::raii::ImageView& depthImageView, int width, int height, int frameIndex) {
    if (numFramesSinceResize == 0) {
        device->waitIdle();
        Application::get() -> resetAllCommandBuffers();

        this->imageViewManager->resizeImage(RenderPassImages::baseForwardPass, width, height);

    }
    if (numFramesSinceResize < Config::maxFramesInFlight) {
        depthTextureViews[frameIndex] = TextureView{.imageView = depthImageView};

        rtShaderPipeline -> setTextureSampler(RTShaderSlots::depthBuffer, depthTextureViews[frameIndex], vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal,frameIndex);
        rtShaderPipeline -> setStorageImage(RTShaderSlots::rtOutput, width, height, frameIndex, StorageImages::raytracingOutput);

        TextureView* textureView = this->imageViewManager->getTextureView(RenderPassImages::baseForwardPass, frameIndex);
        combineShaders -> setTextureSampler(CombineShaderSlots::firstImage, *textureView,
                                         vk::ImageLayout::eShaderReadOnlyOptimal, frameIndex);

        rtTextureViews[frameIndex] = TextureView{.imageView = rtShaderPipeline -> getStorageImage(StorageImages::raytracingOutput, frameIndex).imageView};

        combineShaders -> setTextureSampler(CombineShaderSlots::secondImage, (rtTextureViews[frameIndex]), vk::ImageLayout::eShaderReadOnlyOptimal, frameIndex);
    }
    numFramesSinceResize++;
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


    glm::mat4 proj = glm::perspectiveRH_ZO(fov, aspect, nearPlane, farPlane);
    proj[1][1] *= -1;
    return proj;
}

void Renderer::initScreenQuad(vk::raii::PhysicalDevice& physicalDevice) {
    std::vector<float> screenQuadVertices = {
        -1.0f, -1.0f,  0.0f,
        -1.0f,  1.0f,  0.0f,
         1.0f, -1.0f,  0.0f,
         1.0f,  1.0f,  0.0f
    };

    std::vector<float> screenQuadUv = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };

    std::vector<uint32_t> screenQuadIndices = {
        0, 1, 2,
        1, 3, 2
    };
    screenQuad = loader.load(screenQuadVertices, screenQuadIndices, std::nullopt, screenQuadUv, *device, physicalDevice);
}


