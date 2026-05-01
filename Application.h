

#ifndef VULKAN_TEST_APPLICATION_H
#define VULKAN_TEST_APPLICATION_H
#include <optional>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vk_mem_alloc.h"


#include <vector>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "Camera.h"
#include "Renderer.h"
#include "shader-pipeline/ShaderPair.h"
#include "shader-pipeline/ShaderPipelineRegistry.h"

//This class owns and manages the lifecycle of the application
//It handles window management, initialization of vulkan (i.e. devices, swap chain, command buffers ect.) and coordination of systems
class Application {
    vk::raii::Context context;
    std::optional<vk::raii::Instance> instance;
    std::optional<vk::raii::PhysicalDevice> physicalDevice;
    std::optional<vk::raii::Device> device;
    std::optional<vk::raii::SurfaceKHR> surface;
    std::optional<vk::raii::SwapchainKHR> swapChain;
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> imageAvailableFences;
    std::vector<vk::raii::Fence> syncHostWithDeviceFences;
    std::vector<vk::raii::ImageView> imageViews;
    std::optional<vk::Format> swapChainImageFormat;
    std::optional<vk::raii::CommandPool> commandPool;
    std::optional<vk::raii::CommandBuffers> commandBuffers;
    std::optional<vk::Extent2D> swapChainExtent;
    // VkImage depthImage;
    // VmaAllocation depthImageAllocation;
    std::array<VkImage, 3> depthImages;
    std::array<VmaAllocation, 3> depthImageAllocations;

    std::array<std::optional<vk::raii::ImageView>, 3> depthImageViews;

    std::optional<Renderer> renderer;
    ShaderPipelineRegistry shaderPipelineRegistry;
    VmaAllocator allocator;
    GLFWwindow* window;
    Camera camera;
    static Application* app;
    public:void run();
    static Application *get();
    static void setup();

    GLFWwindow* getWindow();

    Camera getCamera();
    void resetAllCommandBuffers();

private:void cleanUp(GLFWwindow *window);


    void loop(GLFWwindow *window);

    void draw(int imageIndex, vk::PipelineStageFlags stageFlags, int currentFrame);

    void present(const uint32_t *imagePointer, int currentFrame);

    int getNextImage(int currentFrame);

    void setupVulkan(GLFWwindow *window);

    void createSurface(GLFWwindow *window);

    void setupWindow();

    void createVulkanInstance();

    void setupDevices();

    void setupSwapChain();

    void createSwapChainImageViews();


    void createCommandPool();

    void createCommandBuffers();

    void initRenderer();

    void setUpMemoryAllocator();

};



#endif //VULKAN_TEST_APPLICATION_H

