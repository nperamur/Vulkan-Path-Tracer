

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
#include "rendering/Renderer.h"
#include "shader-pipeline/ShaderPair.h"
#include "shader-pipeline/ShaderPipelineRegistry.h"

struct InFlightImageAvailabilitySemaphoreInfo {
    uint64_t startCount;
    uint64_t endCount;
    int acquireIndex;
    vk::Semaphore semaphore;
};
namespace std {
    template<> struct hash<vk::Semaphore> {
        size_t operator()(vk::Semaphore const& s) const noexcept {
            return std::hash<VkSemaphore>{}(static_cast<VkSemaphore>(s));
        }
    };
}
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
    std::vector<vk::raii::Semaphore> timelineSemaphores;
    std::vector<vk::raii::Fence> imageAvailableFences;
    std::vector<vk::raii::Fence> syncHostWithDeviceFences;
    std::vector<vk::raii::ImageView> imageViews;
    std::optional<vk::Format> swapChainImageFormat;
    std::optional<vk::raii::CommandPool> commandPool;
    std::optional<vk::raii::CommandBuffers> commandBuffers;
    std::optional<vk::Extent2D> swapChainExtent;
    std::array<vk::Fence, 3> imagesInFlight;
    std::array<vk::Semaphore, 3> imageAvailableSemaphoresInFlight;
    //note to self: endCount, imageavailabilitysemaphore
    std::vector<InFlightImageAvailabilitySemaphoreInfo> inFlightAvailabilitySemaphores;
    std::unordered_map<vk::Semaphore, uint64_t> semaphoreCounter;
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

    VmaAllocator getMemoryAllocator();

    static void setup();

    GLFWwindow* getWindow();

    Camera getCamera();
    void resetAllCommandBuffers();

private:void cleanUp(GLFWwindow *window);


    void loop(GLFWwindow *window);

    void draw(int imageIndex, vk::PipelineStageFlags stageFlags);

    void present(const uint32_t *imagePointer, int imageIndex);

    int getNextImage(int acquireIndex);

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

    std::pair<int, bool> getNextAcquireIndex();


};



#endif //VULKAN_TEST_APPLICATION_H

