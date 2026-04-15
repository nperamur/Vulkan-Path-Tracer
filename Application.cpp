#include "Application.h"
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#ifndef VULKAN_SDK_FOUND
#include <volk.h>
#else
#include <vulkan/vulkan.h>
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
const char* const validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};




VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#ifdef VULKAN_SDK_FOUND
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif
Application* Application::app = nullptr;

void Application::run() {
    setupWindow();
    window = glfwCreateWindow(800, 600, "My Vulkan Triangle", NULL, NULL);
    if (!window) {
        glfwTerminate();
    }
    setupVulkan(window);
    loop(window);
    cleanUp(window);
}

void Application::cleanUp(GLFWwindow* window) {
    device->waitIdle();
    commandPool->reset();
    renderer -> cleanUp();
    vmaDestroyImage(allocator, depthImage, depthImageAllocation);
    vmaDestroyAllocator(allocator);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::loop(GLFWwindow* window) {
    int currentFrame = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        device->waitForFences({*imageAvailableFences.at(currentFrame), *syncHostWithDeviceFences.at(currentFrame)}, VK_TRUE, UINT64_MAX);


        int imageIndex = getNextImage(currentFrame);
        if (imageIndex == -1) {
            std::cout << "Swapchain next image retrieval failed";
            break;
        }
        camera.handleInputs();
        vk::PipelineStageFlagBits flagBits = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        commandBuffers->at(imageIndex).reset();
        renderer -> render(commandBuffers->at(imageIndex), imageViews.at(imageIndex), depthImageView.value(), swapChain -> getImages().at(imageIndex), depthImage, currentFrame);
        draw(imageIndex, flagBits, currentFrame);

        uint32_t image = imageIndex;
        present(&image, currentFrame);
        currentFrame = (currentFrame + 1) % 3;
    }
}

void Application::draw(int imageIndex, vk::PipelineStageFlags stageFlags, int currentFrame) {
    vk::Queue queue = device->getQueue(0, 0);
    vk::SubmitInfo submitInfo(
        1,
        &(*imageAvailableSemaphores.at(currentFrame)),
        &stageFlags,
        1,
        &*commandBuffers->at(imageIndex),
        1,
        &(*renderFinishedSemaphores.at(currentFrame))

    );
    device->resetFences(*syncHostWithDeviceFences.at(currentFrame));
    queue.submit(submitInfo, *syncHostWithDeviceFences.at(currentFrame));
}

void Application::present(const uint32_t *imagePointer, int currentFrame) {

    vk::PresentInfoKHR presentInfo(
        1,
        &(*renderFinishedSemaphores.at(currentFrame)),
        1,
        &(**swapChain),
        imagePointer

    );
    vk::Queue queue = device->getQueue(0, 0);
    vk::Result result = queue.presentKHR(&presentInfo);
    if (result != vk::Result::eSuccess) {
        switch (result) {
            case vk::Result::eErrorOutOfDateKHR:
                std::cout << "Detected out of date error. Will recreate swapchain.";
                setupSwapChain();
                break;
            case vk::Result::eSuboptimalKHR:
                //std::cout << "Suboptimal KHR. May recreate later.";
                setupSwapChain();
                break;
            default:
                throw std::runtime_error("Failed to present swapchain image: " + vk::to_string(result));
        }
    }


}


int Application::getNextImage(int currentFrame) {
    device->resetFences(*imageAvailableFences.at(currentFrame));
    auto [result, imageIndex]  = swapChain -> acquireNextImage(UINT64_MAX, *imageAvailableSemaphores.at(currentFrame), *imageAvailableFences.at(currentFrame));

    if (result != vk::Result::eSuccess) {
        switch (result) {
            case vk::Result::eErrorOutOfDateKHR:
                std::cout << "Detected out of date error. Will recreate swapchain.";
                setupSwapChain();
                std::tie(result, imageIndex) = swapChain->acquireNextImage(UINT64_MAX, *imageAvailableSemaphores.at(currentFrame), *imageAvailableFences.at(currentFrame));
                if (result != vk::Result::eSuccess) {
                    return -1;
                }
                setupSwapChain();
                break;
            case vk::Result::eSuboptimalKHR:
                //std::cout << "Suboptimal KHR. May recreate later.";
                break;
            default:
                throw std::runtime_error("Failed to present swapchain image: " + vk::to_string(result));
        }
    }
    return imageIndex;

}




//Does the initial vulkan setup
void Application::setupVulkan(GLFWwindow* window) {
    createVulkanInstance();
    createSurface(window);
    setupDevices();
    setUpMemoryAllocator();
    setupSwapChain();
    createCommandPool();
    createCommandBuffers();
    initRenderer();

}

void Application::createSurface(GLFWwindow* window) {
    //Get window surface
    VkSurfaceKHR vkSurface;
    glfwCreateWindowSurface(**instance, window, nullptr, &vkSurface);

    this -> surface.emplace(*instance, vkSurface);
}

void Application::setupWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

void Application::createVulkanInstance() {
    #ifndef VULKAN_SDK_FOUND
        if (volkInitialize() != VK_SUCCESS) {
            throw std::runtime_error("Failed to find a Vulkan loader");
        }
        VULKAN_HPP_DEFAULT_DISPATCHER.init((PFN_vkGetInstanceProcAddr)vkGetInstanceProcAddr);
    #endif


    constexpr vk::ApplicationInfo appInfo(
        "Vulkan Graphics Engine",
        VK_MAKE_VERSION(1, 4, 0),
        "VKRNP",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_4
    );

    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < glfwExtensionCount; ++i)
    {
        if (std::ranges::none_of(extensionProperties,
                                 [glfwExtension = glfwExtensions[i]](auto const& extensionProperty)
                                 { return strcmp(extensionProperty.extensionName, glfwExtension) == 0; }))
        {
            throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfwExtensions[i]));
        }
    }

    int enabledLayerCount = 0;
    const char* enabledLayers[1];
    if (enableValidationLayers) {
        enabledLayers[0] = "VK_LAYER_KHRONOS_validation";
        enabledLayerCount = 1;
    } else {
        enabledLayers[0] = nullptr;
    }
    // std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    // extensions.push_back("VK_EXT_debug_utils");

    vk::InstanceCreateInfo createInfo(
        {},
        &appInfo,
        enabledLayerCount,
        enabledLayers,
        glfwExtensionCount,
        glfwExtensions
    );

    instance.emplace(context, createInfo);
    #ifndef VULKAN_SDK_FOUND
        volkLoadInstance(**instance);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(**instance);
    #endif
}

void Application::setupDevices() {
    //Get available physical devices:
    auto devices = instance->enumeratePhysicalDevices();
    if (devices.empty()) {
        std::cout << "No vulkan-capable physical device found! Why are you running a vulkan engine "
                     "\n on a device without a gpu that supports vulkan?";
        std::exit(1);
    }


    //Physical device selection:
    //If the user has a discrete gpu use that otherwise use that otherwise use the igpu
    for (auto& device : devices) {
        auto properties = device.getProperties();
        if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            physicalDevice = std::move(device);
            break;
        }
    }

    if (!physicalDevice.has_value()) {
        physicalDevice = std::move(devices.at(0));
    }



    float priority = 1.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
        {},
        0,
        1,
        &priority
    );

    //Logical device selection:
    const char* enabledExtensions[8] = {"VK_KHR_swapchain", "VK_KHR_acceleration_structure", "VK_KHR_ray_tracing_pipeline", "VK_KHR_deferred_host_operations", "VK_KHR_ray_query", "VK_KHR_pipeline_library", "VK_KHR_buffer_device_address", "VK_EXT_descriptor_indexing"};
    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures(VK_TRUE);
    vk::PhysicalDeviceSynchronization2Features sync2Features(VK_TRUE);

    vk::DeviceCreateInfo deviceCreateInfo (
        {},
        1,
        &deviceQueueCreateInfo,
        0,
        nullptr,
        8,
        enabledExtensions

    );

    dynamicRenderingFeatures.setPNext(&sync2Features);
    deviceCreateInfo.setPNext(&dynamicRenderingFeatures);


    device.emplace(*physicalDevice, deviceCreateInfo);

    #ifndef VULKAN_SDK_FOUND
        volkLoadDevice(**device);
    #endif

    vk::SemaphoreCreateInfo semaphoreInfo{};
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

    for (int i = 0; i < 3; i++) {
        imageAvailableSemaphores.emplace_back(*device, semaphoreInfo);
        renderFinishedSemaphores.emplace_back(*device, semaphoreInfo);
        imageAvailableFences.emplace_back(*device, fenceInfo);
        syncHostWithDeviceFences.emplace_back(*device, fenceInfo);
    }
}


void Application::setupSwapChain() {


    device->waitIdle();
    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice -> getSurfaceCapabilitiesKHR(*surface);
    if (capabilities.currentExtent.width == 0 || capabilities.currentExtent.height == 0) return;
    imageViews.clear();
    swapChain.reset();
    depthImageView = VK_NULL_HANDLE;
    if (depthImage != VK_NULL_HANDLE) {
        vmaDestroyImage(allocator, depthImage, depthImageAllocation);
    }
    auto surfaceFormats = physicalDevice -> getSurfaceFormatsKHR(**surface);


    std::optional<vk::ColorSpaceKHR> colorSpace;

    for (auto& surfaceFormat : surfaceFormats) {
        if (surfaceFormat.format == vk::Format::eB8G8R8A8Srgb) {
            swapChainImageFormat.emplace(surfaceFormat.format);
        }
        if (surfaceFormat.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
            colorSpace.emplace(surfaceFormat.colorSpace);
        }
    }

    if (!swapChainImageFormat.has_value()) {
        swapChainImageFormat.emplace(surfaceFormats.at(0).format);
    }

    if (!colorSpace.has_value()) {
        colorSpace.emplace(surfaceFormats.at(0).colorSpace);
    }

    //Setup swapchain
    vk::SwapchainCreateInfoKHR swapChainCreateInfo(
        {},
        *surface,
        3,
        swapChainImageFormat.value(),
        colorSpace.value(),
        capabilities.currentExtent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        vk::PresentModeKHR::eImmediate,
        VK_TRUE,
        nullptr
    );
    swapChainExtent.emplace(capabilities.currentExtent);

    swapChain.emplace(device.value(), swapChainCreateInfo);
    createSwapChainImageViews();
}

void Application::createSwapChainImageViews() {
    auto images = swapChain -> getImages();
    for (auto image : images) {
        vk::ImageViewCreateInfo imageViewCreateInfo(
            {},
            image,
            vk::ImageViewType::e2D,
            *swapChainImageFormat,
            vk::ComponentMapping{},
            vk::ImageSubresourceRange(
                vk::ImageAspectFlagBits::eColor,
                0, 1,
                0, 1
            )
        );

        imageViews.emplace_back(*device, imageViewCreateInfo);
    }

    int width, height;
    glfwGetFramebufferSize(Application::get() -> getWindow(), &width, &height);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;;
    imageInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;




    vmaCreateImage(
        allocator,
        &imageInfo,
        &allocInfo,
        &depthImage,
        &depthImageAllocation,
        nullptr
    );

    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = depthImage;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = vk::Format::eD32Sfloat;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    depthImageView.emplace(device -> createImageView(viewInfo));


}





void Application::createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        0
    );
    commandPool.emplace(*device, poolInfo);
}



void Application::createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocInfo(*commandPool, vk::CommandBufferLevel::ePrimary,3);
    commandBuffers = vk::raii::CommandBuffers(*device, allocInfo);
}

void Application::initRenderer() {
    renderer.emplace(shaderPipelineRegistry, *device, *swapChainImageFormat, *swapChainExtent, *physicalDevice, allocator);
}

void Application::setUpMemoryAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = **physicalDevice;
    allocatorInfo.device = **device;
    allocatorInfo.instance = **instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
    VmaVulkanFunctions vulkanFunctions{};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    allocatorInfo.pVulkanFunctions = &vulkanFunctions;


    vmaCreateAllocator(&allocatorInfo, &allocator);
}

int main() {
    Application::setup();
    return 0;

}

Application *Application::get() {
    return app;
}

void Application::setup() {
    app = new Application();
    app->run();
}

GLFWwindow* Application::getWindow() {
    return window;
}


Camera Application::getCamera() {
    return camera;
}



