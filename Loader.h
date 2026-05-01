#ifndef VULKAN_TEST_LOADER_H
#define VULKAN_TEST_LOADER_H
#include <array>
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

struct Model {
    std::optional<vk::raii::Buffer> vertexBuffer;
    int numVertices = 0;
    std::optional<vk::raii::Buffer> indexBuffer;
    int numIndices = 0;
    std::optional<vk::raii::Buffer> normalBuffer;
    std::optional<vk::raii::Buffer> textureCoordBuffer;
    std::optional<vk::raii::DeviceMemory> vertexMemory;
    std::optional<vk::raii::DeviceMemory> normalMemory;
    std::optional<vk::raii::DeviceMemory> textureCoordMemory;
    std::optional<vk::raii::DeviceMemory> indexMemory;
};
class Loader {
    int numVertices = 0;

    std::optional<vk::raii::Buffer> vertexBuffer;
    std::optional<vk::raii::Buffer> stagingBuffer;
    std::optional<vk::raii::Buffer> normalBuffer;
    std::optional<vk::raii::Buffer> textureCoordBuffer;
    std::optional<vk::raii::Buffer> indexBuffer;
    std::optional<vk::raii::DeviceMemory> vertexMemory;
    std::optional<vk::raii::DeviceMemory> stagingMemory;
    std::optional<vk::raii::DeviceMemory> indexMemory;
    std::optional<vk::raii::DeviceMemory> normalMemory;
    std::optional<vk::raii::DeviceMemory> textureCoordMemory;



    public: Model load(std::vector<float> vertices, std::optional<std::vector<uint32_t>> indices, std::optional<std::vector<float>> normals,
        std::optional<std::vector<float>> textureCoords, vk::raii::Device &device, vk::raii::PhysicalDevice &physicalDevice);


};



#endif //VULKAN_TEST_LOADER_H
