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
#include "shader-pipeline/RaytracingShaderPipeline.h"


#define BEGIN_RENDER_PASS(cmd, view, depthView, extent, infoName) \
vk::RenderingAttachmentInfo colorAttachment( \
view, \
vk::ImageLayout::eColorAttachmentOptimal, \
vk::ResolveModeFlagBits::eNone, \
{}, {}, \
vk::AttachmentLoadOp::eClear, \
vk::AttachmentStoreOp::eStore, \
vk::ClearValue{vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}} \
); \
vk::RenderingAttachmentInfo depthAttachment( \
depthView, \
vk::ImageLayout::eDepthStencilAttachmentOptimal, \
vk::ResolveModeFlagBits::eNone, \
{}, {}, \
vk::AttachmentLoadOp::eClear, \
vk::AttachmentStoreOp::eDontCare, \
vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0)) \
); \
vk::RenderingInfo infoName( \
{}, \
vk::Rect2D({0, 0}, extent), \
1, 0, 1, &colorAttachment, \
&depthAttachment, nullptr \
); \
cmd.beginRendering(infoName);



Renderer::Renderer(ShaderPairRegistry &shaderPairRegistry, vk::raii::Device& device, vk::Format& swapChainImageFormat,
                   vk::Extent2D& swapChainExtent, vk::raii::PhysicalDevice& physicalDevice, VmaAllocator& allocator) {
    this -> shaderPairRegistry = &shaderPairRegistry;
    this -> swapChainImageFormat = &swapChainImageFormat;
    this -> swapChainExtent = &swapChainExtent;
    DescriptorsInfo triangleDescriptorsInfo = {
        .staticData = {.numUBOs = 1, .numTextureSamplers = 0},
        .dynamicData = {.numUBOs = 2, .numTextureSamplers = 0}
    };
    shaderPairRegistry.registerShaderPair(std::make_unique<ShaderPair>("triangle", device, swapChainImageFormat, allocator, triangleDescriptorsInfo));
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
    ModelLoader modelLoader;
    entities.emplace_back("sponza", modelLoader.load("sponza", loader, device, physicalDevice), mvp.transformation);
    this->allocator = &allocator;


    light.color = glm::vec4(1.0, 0.95, 0.8, 1.0);
    light.position = glm::vec4(500.0, 800.0, 300.0, 1.0);
    shaderPairRegistry.getShaderPair("triangle") -> setUniform({.set = 0, .binding = 0}, &light, sizeof(light),  0);
    this -> device = &device;
}



void Renderer::render(vk::raii::CommandBuffer &commandBuffer, vk::raii::ImageView &imageView, vk::raii::ImageView &depthImageView, vk::Image &image, VkImage depthImage, int frameIndex) {
    //TODO: add secondary command buffer support later
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

    BEGIN_RENDER_PASS(commandBuffer, *imageView, *depthImageView, *swapChainExtent, myRenderInfo);

    ShaderPair* triangleShader = shaderPairRegistry->getShaderPair("triangle").get();
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

void Renderer::presentationMemoryBarrier(vk::raii::CommandBuffer& commandBuffer, vk::Image& image, VkImage& depthImage) {
    vk::ImageMemoryBarrier2 barrierToPresent(
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::AccessFlagBits2::eNone,
        vk::ImageLayout::eColorAttachmentOptimal,
           vk::ImageLayout::ePresentSrcKHR,
        {}, {},
        image,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );

    vk::ImageMemoryBarrier2 depthBarrier(
        vk::PipelineStageFlagBits2::eEarlyFragmentTests,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::AccessFlagBits2::eNone,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
           vk::ImageLayout::ePresentSrcKHR,
        {}, {},
        depthImage,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)
    );

    std::array<vk::ImageMemoryBarrier2, 2> barriers = {{
        barrierToPresent,
        depthBarrier
    }};

    vk::DependencyInfo presentDepInfo({}, {}, {}, barriers);
    commandBuffer.pipelineBarrier2(presentDepInfo);
}

void Renderer::renderingMemoryBarrier(vk::raii::CommandBuffer& commandBuffer, vk::Image& image, VkImage& depthImage) {
    vk::ImageMemoryBarrier2 barrierToRender(
    vk::PipelineStageFlagBits2::eTopOfPipe,
    vk::AccessFlagBits2::eNone,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::AccessFlagBits2::eColorAttachmentWrite,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eColorAttachmentOptimal,
    {}, {},
    image,
    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );

    vk::ImageMemoryBarrier2 depthBarrier(
    vk::PipelineStageFlagBits2::eTopOfPipe,
    vk::AccessFlagBits2::eNone,
    vk::PipelineStageFlagBits2::eEarlyFragmentTests,
    vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eDepthStencilAttachmentOptimal,
    {}, {},
        depthImage,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)
    );

    std::array<vk::ImageMemoryBarrier2, 2> barriers = {{
        barrierToRender,
        depthBarrier
    }};


    vk::DependencyInfo depInfo({}, {}, {}, barriers);
    commandBuffer.pipelineBarrier2(depInfo);

}



void Renderer::cleanUp() {
    this -> shaderPairRegistry -> cleanUp();
    vmaDestroyBuffer(*allocator, instanceBuffer, instanceAllocation);
}


void Renderer::buildBLASGeometry() {
    geometry.tlasGeometry.primitiveCount = entities.size();

    int i = 0;
    for (Entity &entity : entities) {
        vk::BufferDeviceAddressInfo vertexBufferDeviceAddressInfo(
            *entity.getModel().vertexBuffer
        );
        vk::DeviceAddress vertexAddress = device -> getBufferAddress(vertexBufferDeviceAddressInfo);
        vk::BufferDeviceAddressInfo indexBufferDeviceAddressInfo(
            *entity.getModel().indexBuffer
        );
        vk::DeviceAddress indexAddress = device -> getBufferAddress(indexBufferDeviceAddressInfo);

        vk::DeviceOrHostAddressConstKHR vertexData(vertexAddress);
        vk::DeviceOrHostAddressConstKHR indexData(indexAddress);
        vk::AccelerationStructureGeometryTrianglesDataKHR triangleData(
            vk::Format::eR32G32B32Sfloat,
            vertexData,
            sizeof(float) * 3,
            entity.getModel().numVertices - 1,
            vk::IndexType::eUint32,
            indexData,
            {}

        );
        vk::AccelerationStructureGeometryDataKHR asGeometryData(triangleData);
        vk::AccelerationStructureGeometryKHR triangleGeometry(
            vk::GeometryTypeKHR::eTriangles,
            asGeometryData,
            {}

        );


        GeometryData geometryData = {.geometry = triangleGeometry, .primitiveCount = (uint32_t)(entity.getModel().numIndices / 3)};
        geometry.blasGeometry.push_back(geometryData);

    }



}

void Renderer::buildTLASGeometry(std::vector<AccelerationStructureData> blasData) {
    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    int i = 0;
    for (Entity &entity : entities) {
        entity.updateTransformationMatrix();
        glm::mat4 transposed = glm::transpose(mvp.transformation);
        vk::TransformMatrixKHR transformMatrix;
        memcpy(&transformMatrix, &transposed, sizeof(vk::TransformMatrixKHR));
        vk::AccelerationStructureInstanceKHR asInstance(
            transformMatrix,
            i,
            0xFF,
            0,
            {},
            blasData[i].deviceAddress
        );
        instances.push_back(asInstance);
        i++;
    }


    VkBufferCreateInfo instanceBufferInfo{};
    instanceBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

    instanceBufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VmaAllocationCreateInfo instanceBufferAllocInfo{};
    instanceBufferAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    instanceBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    instanceBufferInfo.size = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
    vmaCreateBuffer(*allocator, &instanceBufferInfo, &instanceBufferAllocInfo, &instanceBuffer, &instanceAllocation, nullptr);

    VmaAllocationInfo instanceAllocInfo;
    vmaGetAllocationInfo(*allocator, instanceAllocation, &instanceAllocInfo);
    uint8_t* mapped = (uint8_t*)instanceAllocInfo.pMappedData;

    memcpy(mapped, instances.data(), instances.size() * sizeof(VkAccelerationStructureInstanceKHR));

    vk::BufferDeviceAddressInfo addressInfo(
        instanceBuffer
    );
    vk::DeviceAddress instanceAddress = device -> getBufferAddress(addressInfo);
    vk::DeviceOrHostAddressConstKHR address(instanceAddress);
    vk::AccelerationStructureGeometryInstancesDataKHR instanceGeometry(
        VK_FALSE,
         address

    );
    vk::AccelerationStructureGeometryDataKHR asGeometryData(instanceGeometry);

    vk::AccelerationStructureGeometryKHR tlasGeometry(
        vk::GeometryTypeKHR::eInstances,
        asGeometryData,
        {}

    );

    this -> geometry.tlasGeometry.geometry = tlasGeometry;
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

