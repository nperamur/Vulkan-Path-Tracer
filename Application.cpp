#include "Application.h"
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <unordered_set>

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
    window = glfwCreateWindow(800, 600, "Vulkan Path Tracer", NULL, NULL);
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
    for (int i = 0; i < Config::maxFramesInFlight; i++) {
        vmaDestroyImage(allocator, depthImages[i], depthImageAllocations[i]);
    }
    vmaDestroyAllocator(allocator);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::loop(GLFWwindow* window) {
    int currentFrame = 0;
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        int width, height;
        glfwGetFramebufferSize(Application::get() -> getWindow(), &width, &height);
        if (width == 0 || height == 0) {
            glfwWaitEvents();
            continue;
        }
        device->waitForFences({*imageAvailableFences.at(currentFrame), *syncHostWithDeviceFences.at(currentFrame)}, VK_TRUE, UINT64_MAX);


        vk::PipelineStageFlagBits flagBits = { vk::PipelineStageFlagBits::eAllCommands };
        int imageIndex = getNextImage(currentFrame);
        if (imageIndex == -1) {
            //std::cout << "Swapchain next image retrieval failed";
            device -> waitIdle();
            currentFrame = 0;
            continue;
        }
        if (imagesInFlight[imageIndex]) {
            device->waitForFences({imagesInFlight[imageIndex]}, VK_TRUE, UINT64_MAX);
        }
        imagesInFlight[imageIndex] = *syncHostWithDeviceFences.at(currentFrame);
        camera.handleInputs();
        commandBuffers->at(imageIndex).reset();
        renderer -> render(commandBuffers->at(imageIndex), imageViews.at(imageIndex),
            depthImageViews[imageIndex].value(), swapChain -> getImages().at(imageIndex), depthImages[imageIndex], imageIndex);
        draw(imageIndex, flagBits, currentFrame);

        uint32_t image = imageIndex;
        present(&image, currentFrame);
        currentFrame = (currentFrame + 1) % Config::maxFramesInFlight;
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        // printf("FPS:%f\n", 1/deltaTime);
        lastTime = currentTime;
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
                setupSwapChain();
                return -1;
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
        enabledLayerCount = 0;
    }
    // std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    // extensions.push_back("VK_EXT_debug_utils");

    std::vector<vk::ValidationFeatureEnableEXT> enabledFeatures = {
        vk::ValidationFeatureEnableEXT::eSynchronizationValidation
    };

    vk::ValidationFeaturesEXT validationFeatures(
        enabledFeatures,
        nullptr
    );

    vk::InstanceCreateInfo createInfo(
        {},
        &appInfo,
        enabledLayerCount,
        enabledLayers,
        glfwExtensionCount,
        glfwExtensions,
        &validationFeatures
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
    const char* enabledExtensions[10] = {"VK_KHR_swapchain", "VK_KHR_acceleration_structure", "VK_KHR_ray_tracing_pipeline",
        "VK_KHR_deferred_host_operations", "VK_KHR_ray_query", "VK_KHR_pipeline_library", "VK_KHR_buffer_device_address", "VK_EXT_descriptor_indexing",
    "VK_EXT_robustness2", "VK_KHR_ray_tracing_position_fetch"};
    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures(VK_TRUE);
    vk::PhysicalDeviceSynchronization2Features sync2Features(VK_TRUE);
    vk::PhysicalDeviceBufferDeviceAddressFeatures deviceAddressFeatures(VK_TRUE);
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures{};
    vk::PhysicalDeviceRobustness2FeaturesKHR robustnessFeatures{};
    vk::PhysicalDeviceRayTracingPositionFetchFeaturesKHR positionFetchFeatures{};
    positionFetchFeatures.rayTracingPositionFetch = VK_TRUE;
    robustnessFeatures.nullDescriptor = VK_TRUE;
    rtFeatures.rayTracingPipeline = VK_TRUE;
    rtFeatures.rayTracingPipelineTraceRaysIndirect = VK_TRUE;
    rtFeatures.pNext = nullptr;
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR physicalDeviceAccelerationFeatures{};
    physicalDeviceAccelerationFeatures.accelerationStructure = VK_TRUE;
    physicalDeviceAccelerationFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;
    vk::PhysicalDeviceFeatures2 physicalDeviceFeatures{};
    physicalDeviceFeatures.features.geometryShader = VK_TRUE;
    physicalDeviceFeatures.features.shaderInt64 = VK_TRUE;
    VkPhysicalDeviceDescriptorIndexingFeatures indexing{};
    indexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    indexing.runtimeDescriptorArray = VK_TRUE;
    indexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    indexing.descriptorBindingPartiallyBound = VK_TRUE;
    indexing.descriptorBindingVariableDescriptorCount = VK_TRUE;
    vk::DeviceCreateInfo deviceCreateInfo (
        {},
        1,
        &deviceQueueCreateInfo,
        0,
        nullptr,
        10,
        enabledExtensions

    );

    dynamicRenderingFeatures.setPNext(&sync2Features);
    deviceCreateInfo.setPNext(&dynamicRenderingFeatures);
    sync2Features.setPNext(&deviceAddressFeatures);
    deviceAddressFeatures.setPNext(&rtFeatures);
    rtFeatures.setPNext(&physicalDeviceAccelerationFeatures);
    physicalDeviceAccelerationFeatures.setPNext(&robustnessFeatures);
    robustnessFeatures.setPNext(&physicalDeviceFeatures);
    physicalDeviceFeatures.setPNext(&positionFetchFeatures);
    positionFetchFeatures.setPNext(&indexing);



    device.emplace(*physicalDevice, deviceCreateInfo);

    #ifndef VULKAN_SDK_FOUND
        volkLoadDevice(**device);
    #endif

}


void Application::setupSwapChain() {
    device->waitIdle();
    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice -> getSurfaceCapabilitiesKHR(*surface);
    if (capabilities.currentExtent.width == 0 || capabilities.currentExtent.height == 0) return;
    imageViews.clear();
    for (int i = 0; i < imagesInFlight.size(); i++) {
        imagesInFlight[i] = nullptr;
    }
    swapChain.reset();

    for (int i = 0; i < Config::maxFramesInFlight; i++) {
        depthImageViews[i] = VK_NULL_HANDLE;
        if (depthImages[i] != VK_NULL_HANDLE) {
            vmaDestroyImage(allocator, depthImages[i], depthImageAllocations[i]);
        }
    }
    auto surfaceFormats = physicalDevice -> getSurfaceFormatsKHR(**surface);


    std::optional<vk::ColorSpaceKHR> colorSpace;

    for (auto& surfaceFormat : surfaceFormats) {
        if (surfaceFormat.format == vk::Format::eB8G8R8A8Srgb && surfaceFormat.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
            swapChainImageFormat.emplace(surfaceFormat.format);
            colorSpace.emplace(surfaceFormat.colorSpace);
            break;
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
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
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

    vk::SemaphoreCreateInfo semaphoreInfo{};
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

    imageAvailableFences.clear();
    renderFinishedSemaphores.clear();
    imageAvailableSemaphores.clear();
    syncHostWithDeviceFences.clear();
    for (int i = 0; i < Config::maxFramesInFlight; i++) {
        imageAvailableSemaphores.emplace_back(*device, semaphoreInfo);
        renderFinishedSemaphores.emplace_back(*device, semaphoreInfo);
        imageAvailableFences.emplace_back(*device, fenceInfo);
        syncHostWithDeviceFences.emplace_back(*device, fenceInfo);
    }
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

    for (int i = 0; i < Config::maxFramesInFlight; i++) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_D32_SFLOAT;;
        imageInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;




        vmaCreateImage(
            allocator,
            &imageInfo,
            &allocInfo,
            &depthImages[i],
            &depthImageAllocations[i],
            nullptr
        );

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = depthImages[i];
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = vk::Format::eD32Sfloat;
        viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        depthImageViews[i].emplace(device -> createImageView(viewInfo));
    }



}





void Application::createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        0
    );
    commandPool.emplace(*device, poolInfo);
}



void Application::createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocInfo(*commandPool, vk::CommandBufferLevel::ePrimary, Config::maxFramesInFlight);
    commandBuffers = vk::raii::CommandBuffers(*device, allocInfo);
}

void Application::initRenderer() {
    renderer.emplace(shaderPipelineRegistry, *device, *swapChainImageFormat, *swapChainExtent, *physicalDevice, &allocator);
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
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;



    vmaCreateAllocator(&allocatorInfo, &allocator);
}

void Application::resetAllCommandBuffers() {
    commandPool -> reset();
}

int main() {
    Application::setup();
    return 0;

}

Application *Application::get() {
    return app;
}

VmaAllocator Application::getMemoryAllocator() {
    return allocator;
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



