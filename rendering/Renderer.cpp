

#include "../Application.h"
#include "../Loader.h"
#include "../ModelLoader.h"
#include "GLFW/glfw3.h"
#include "glm/fwd.hpp"
#include "glm/vec4.hpp"
#include "glm/glm.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtx/dual_quaternion.hpp"
#include "../shader-pipeline/RaytracingShaderPipeline.h"
#include "../VulkanCommon.h"

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
                                            std::vector<vk::Format>{swapChainImageFormat, vk::Format::eR32G32Sfloat, vk::Format::eR32G32B32A32Sfloat}, *allocator, triangleDescriptorsInfo, 3));

    for (int i = 0; i < Config::maxFramesInFlight; i++) {
        this -> shaderPipelineRegistry -> getShaderPipeline(Shaders::triangle) -> setUniform({ForwardPassShaderSlots::lightUBO}, &light, sizeof(light),  i);
    }
    //raytracing
    DescriptorsInfo raytracingDescriptorsInfo = {
        .staticData = {.numTextureSamplers = 3, .numAccelerationStructures = 1, .numStorageImages = 1, .numStorageBuffers = 2},
        .dynamicData = {.numUBOs = 2}
    };
    materials = sceneManager -> getAllMaterials();
    this->accelStructureManager = AccelerationStructureManager(device, physicalDevice, &*allocator);
    accelStructureManager -> build(sceneManager -> getEntities(), mvp);
    std::unique_ptr<RaytracingShaderPipeline> rtShaderPipeline = std::make_unique<RaytracingShaderPipeline>(Shaders::raytracing, device,
                                                            physicalDevice, *allocator, raytracingDescriptorsInfo, materials.size());
    this -> shaderPipelineRegistry -> registerShaderPipeline(std::move(rtShaderPipeline));
    RaytracingShaderPipeline* rtShader = dynamic_cast<RaytracingShaderPipeline*> (this -> shaderPipelineRegistry -> getShaderPipeline(Shaders::raytracing).get());

    for (int i = 0; i < Config::maxFramesInFlight; i++) {
        rtShader->setAccelerationStructure(RTShaderSlots::accelerationStructure, accelStructureManager -> getTLASData(), i);
        rtShader->setStorageBuffer(RTShaderSlots::materialsBuffer, materials.data(), materials.size() * sizeof(Material), i);
    }

    DescriptorsInfo combineShadersDescriptorsInfo = {
        .staticData = {.numUBOs = 0, .numTextureSamplers = 3},
        .dynamicData = {.numUBOs = 1, .numTextureSamplers = 0}
    };
    this -> shaderPipelineRegistry -> registerShaderPipeline(std::make_unique<ShaderPair>(Shaders::combineShader, device,
                                    std::vector<vk::Format>{swapChainImageFormat, vk::Format::eR32Uint}, *allocator, combineShadersDescriptorsInfo, 1));



    int width, height;
    glfwGetFramebufferSize(Application::get() -> getWindow(), &width, &height);
    imageViewManager -> registerImage(RenderPassImages::baseForwardPass, VK_FORMAT_B8G8R8A8_SRGB, width, height);
    imageViewManager -> registerImage(RenderPassImages::historyBuffer, VK_FORMAT_B8G8R8A8_SRGB, width, height);
    imageViewManager -> registerImage(RenderPassImages::blendOutput, VK_FORMAT_B8G8R8A8_SRGB, width, height);
    imageViewManager -> registerImage(RenderPassImages::visibilityBuffer, VK_FORMAT_R32G32_SFLOAT, width, height);
    imageViewManager -> registerImage(RenderPassImages::normalBuffer, VK_FORMAT_R32G32B32A32_SFLOAT, width, height);


    initScreenQuad(physicalDevice);

}



void Renderer::render(vk::raii::CommandBuffer &commandBuffer, vk::raii::ImageView &swapChainImageView, vk::raii::ImageView &depthImageView, vk::Image &swapChainImage, VkImage depthImage, int frameIndex) {
    //TODO: add secondary command buffer support later
    light.playerPos = glm::vec4(Application::get() -> getCamera().getPos(), 1);
    RaytracingShaderPipeline* rtShaderPipeline = dynamic_cast<RaytracingShaderPipeline*> (shaderPipelineRegistry->getShaderPipeline(Shaders::raytracing).get());
    ShaderPair* triangleShader = dynamic_cast<ShaderPair*> (shaderPipelineRegistry->getShaderPipeline(Shaders::triangle).get());
    ShaderPair* combineShaders = dynamic_cast<ShaderPair*> (shaderPipelineRegistry->getShaderPipeline(Shaders::combineShader).get());

    int width, height;
    glfwGetFramebufferSize(Application::get() -> getWindow(), &width, &height);
    resizeImageViews(combineShaders, rtShaderPipeline, depthImageView, width, height, frameIndex);


    Image* baseForwardPassImage = &(this -> imageViewManager -> getImage(RenderPassImages::baseForwardPass, frameIndex));
    Image* historyImage = &(this -> imageViewManager -> getImage(RenderPassImages::historyBuffer, frameIndex));
    Image* blendOutputImage = &(this -> imageViewManager -> getImage(RenderPassImages::blendOutput, frameIndex));
    Image* visibilityBuffer = &(this -> imageViewManager) -> getImage(RenderPassImages::visibilityBuffer, frameIndex);
    Image* normalBuffer = &(this -> imageViewManager) -> getImage(RenderPassImages::normalBuffer, frameIndex);


    mvp.projection = createProjectionMatrix();
    vk::Rect2D rect2D({0, 0}, *swapChainExtent);
    vk::Viewport viewport(
        0.0f, 0.0f, (float)swapChainExtent->width,
        (float)swapChainExtent->height, 0.0f, 1.0f
    );
    vk::CommandBufferBeginInfo beginInfo({}, nullptr);
    commandBuffer.begin(beginInfo);

    vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
    // commandBuffer.clearColorImage(rtShaderPipeline -> getStorageImage(StorageImages::raytracingOutput, frameIndex).image, vk::ImageLayout::eTransferDstOptimal, {0.0f, 0.0f, 0.0f, 1.0f},
    //                 range);


    if (mvp.view != Application::get() -> getCamera().createViewMatrix()) {
        light.frameCount = 0;
    }

    mvp.view = Application::get() -> getCamera().createViewMatrix();
    float time = glfwGetTime();

    struct TriangleUBO { glm::vec4 color; };
    TriangleUBO data;
    data.color = glm::vec4(std::sin(time * 1.0f) * 0.5f + 0.5f, std::sin(time * 1.3f) * 0.5f + 0.5f, std::sin(time * 1.7f) * 0.5f + 0.5f, 1.0f);
    glm::dmat4 dProj = mvp.projection;
    glm::dmat4 dView = mvp.view;
    inverseViewProj.inverseProj = glm::inverse(dProj);
    inverseViewProj.inverseView = glm::inverse(dView);


    triangleShader -> setUniform(ForwardPassShaderSlots::triangleUBO, &data, sizeof(data), frameIndex);
    rtShaderPipeline -> setUniform(RTShaderSlots::lightUBO, &light, sizeof(light), frameIndex);
    rtShaderPipeline -> setUniform(RTShaderSlots::inverseViewProj, &inverseViewProj, sizeof(inverseViewProj), frameIndex);
    combineShaders -> setUniform(CombineShaderSlots::lightUBO, &light, sizeof(light), frameIndex);
    //
    barrierManager -> begin();
    barrierManager -> transition(combineShaders -> getUniformBuffer({.set = 1, .binding = 0}, frameIndex),
     sizeof(light),  ResourceStage::hostStage | ResourceAccess::write | ResourceType::ubo,
     ResourceAccess::read | ResourceType::ubo | ResourceStage::fragmentShader);
    barrierManager -> transition(rtShaderPipeline -> getUniformBuffer({.set = 1, .binding = 0}, frameIndex),
     sizeof(light),  ResourceStage::hostStage | ResourceAccess::write | ResourceType::ubo,
     ResourceAccess::read | ResourceType::ubo | ResourceStage::raytracing);
    barrierManager -> commit(commandBuffer);
    // Render Graph Stuff...
    uint32_t index = 0;

    commandBuffer.pushConstants2({rtShaderPipeline -> getPipelineLayout(), vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(uint32_t), &light.frameCount});
    renderGraph.initCallbacks([&](Model& model)
        { renderModel(commandBuffer, model, index, viewport, rect2D); index++; }, [&]() {
            traceRays(commandBuffer, viewport, rect2D, width, height, 1);
        }, [this, &commandBuffer, &triangleShader, &rtShaderPipeline, frameIndex]() {
            commandBuffer.pushConstants2({ triangleShader -> getPipelineLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(MVP), &mvp });
        }
    );

    ImageReference depthRef(depthImage, depthImageView);
    ImageReference swapChainRef(swapChainImage, swapChainImageView);

    AbstractRenderPassImage baseForwardRenderPassImage = renderGraph.colorAttachment(*baseForwardPassImage);
    AbstractRenderPassImage depthRenderPassImage = renderGraph.depthAttachment(depthRef);
    AbstractRenderPassImage rtOutputRenderPassImage = renderGraph.storageImage(
                    rtShaderPipeline -> getStorageImage(StorageImages::raytracingOutput, frameIndex));
    AbstractRenderPassImage swapChainRenderPassImage = renderGraph.colorAttachment(swapChainRef);
    AbstractRenderPassImage historyRenderPassImage = renderGraph.colorAttachment(*historyImage);
    AbstractRenderPassImage blendOutputRenderPassImage = renderGraph.colorAttachment(*blendOutputImage);
    AbstractRenderPassImage visibilityRenderPassImage = renderGraph.colorAttachment(*visibilityBuffer);
    AbstractRenderPassImage normalRenderPassImage = renderGraph.colorAttachment(*normalBuffer);

    RenderPass forwardPass = {
        .renderStage = RenderStage::forward,
        .reads = {},
        .writes = {baseForwardRenderPassImage, depthRenderPassImage, visibilityRenderPassImage, normalRenderPassImage},
        .bindPipeline = [&]() { triangleShader -> bind(commandBuffer, frameIndex );},
        .toPresent = false
    };
    renderGraph.addPass(forwardPass);

    RenderPass rtPass = {
        .renderStage = RenderStage::raytracing,
        .reads = {depthRenderPassImage, visibilityRenderPassImage, normalRenderPassImage},
        .writes = {rtOutputRenderPassImage},
        .bindPipeline = [&](){ rtShaderPipeline -> bind(commandBuffer, frameIndex); },
        .toPresent = false
    };
    renderGraph.addPass(rtPass);

    RenderPass combinePass = {
        .renderStage = RenderStage::postProcessing,
        .reads = { rtOutputRenderPassImage, baseForwardRenderPassImage, historyRenderPassImage},
        .writes = { blendOutputRenderPassImage},
        .bindPipeline = [&]() { combineShaders -> bind(commandBuffer, frameIndex); },
        .toPresent = false
    };
    renderGraph.addPass(combinePass);


    RenderPass copyToHistory = {
        .renderStage = RenderStage::copy,
        .reads = { blendOutputRenderPassImage},
        .writes = { historyRenderPassImage},
        .bindPipeline = [&](){},
        .toPresent = false,
        .width  = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),

    };
    renderGraph.addPass(copyToHistory);


    RenderPass copyToSwapchain = {
        .renderStage = RenderStage::copy,
        .reads = { blendOutputRenderPassImage},
        .writes = { swapChainRenderPassImage},
        .bindPipeline = [&](){},
        .toPresent = true,
        .width  = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),

    };
    renderGraph.addPass(copyToSwapchain);


    renderGraph.execute(commandBuffer, *barrierManager, *sceneManager, screenQuad, &*depthImageView, swapChainImageView, *swapChainExtent);
    light.frameCount++;
    commandBuffer.end();
    // device->waitIdle();

}


void Renderer::renderModel(vk::raii::CommandBuffer& commandBuffer, Model& model, int instanceIndex,  vk::Viewport viewport, vk::Rect2D rect2D) {
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
        commandBuffer.drawIndexed(model.numIndices, 1, 0, 0, instanceIndex);
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
        this->imageViewManager->resizeImage(RenderPassImages::historyBuffer, width, height);
        this->imageViewManager->resizeImage(RenderPassImages::blendOutput, width, height);
        this->imageViewManager->resizeImage(RenderPassImages::visibilityBuffer, width, height);
        this->imageViewManager->resizeImage(RenderPassImages::normalBuffer, width, height);
    }
    if (numFramesSinceResize < Config::maxFramesInFlight) {
        depthTextureViews[frameIndex] = TextureView{.imageView = depthImageView};

        rtShaderPipeline -> setTextureSampler(RTShaderSlots::depthBuffer, depthTextureViews[frameIndex], vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal,frameIndex);
        TextureView* visibilityBufferTextureView = this->imageViewManager->getTextureView(RenderPassImages::visibilityBuffer, frameIndex);
        TextureView* normalTextureView = this->imageViewManager->getTextureView(RenderPassImages::normalBuffer, frameIndex);


        rtShaderPipeline -> setTextureSampler(RTShaderSlots::visibilityBuffer, *visibilityBufferTextureView, vk::ImageLayout::eShaderReadOnlyOptimal, frameIndex);
        rtShaderPipeline -> setTextureSampler(RTShaderSlots::normalBuffer, *normalTextureView, vk::ImageLayout::eShaderReadOnlyOptimal, frameIndex);

        rtShaderPipeline -> setStorageImage(RTShaderSlots::rtOutput, width, height, frameIndex, StorageImages::raytracingOutput);
        TextureView* baseForwardPassTextureView = this->imageViewManager->getTextureView(RenderPassImages::baseForwardPass, frameIndex);
        combineShaders -> setTextureSampler(CombineShaderSlots::firstImage, *baseForwardPassTextureView,
                                         vk::ImageLayout::eShaderReadOnlyOptimal, frameIndex);

        rtTextureViews[frameIndex] = TextureView{.imageView = rtShaderPipeline -> getStorageImage(StorageImages::raytracingOutput, frameIndex).imageView};

        combineShaders -> setTextureSampler(CombineShaderSlots::secondImage, (rtTextureViews[frameIndex]), vk::ImageLayout::eShaderReadOnlyOptimal, frameIndex);

        TextureView* historyBufferTextureView = this->imageViewManager->getTextureView(RenderPassImages::historyBuffer, frameIndex);
        combineShaders -> setTextureSampler(CombineShaderSlots::historyBuffer, *historyBufferTextureView, vk::ImageLayout::eShaderReadOnlyOptimal, frameIndex);

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


