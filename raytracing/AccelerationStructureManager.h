
#ifndef VULKAN_TEST_ACCELERATIONSTRUCTUREMANAGER_H
#define VULKAN_TEST_ACCELERATIONSTRUCTUREMANAGER_H

#include <optional>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "vk_mem_alloc.h"
#include "../Entity.h"
#include "glm/ext/matrix_transform.hpp"
#include "../VulkanCommon.h"
struct GeometryData {
    vk::AccelerationStructureGeometryKHR geometry{};
    uint32_t primitiveCount = 0;
};

struct Geometry {
    GeometryData tlasGeometry;
    std::vector<GeometryData> blasGeometry;
};

struct AccelerationStructureData {
    vk::raii::AccelerationStructureKHR handle;
    VkBuffer buffer;
    VmaAllocation allocation;
    vk::DeviceAddress deviceAddress;
};
class AccelerationStructureManager {
    std::optional<AccelerationStructureData> tlasData;

    std::vector<AccelerationStructureData> blasData;
    const VmaAllocator* allocator;
    vk::raii::Device* device;
    std::optional<vk::raii::PhysicalDevice> physicalDevice;
    Geometry geometry;
    VkBuffer instanceBuffer;
    VmaAllocation instanceAllocation;

    public:
    AccelerationStructureManager(vk::raii::Device &device, vk::raii::PhysicalDevice &physicalDevice,
                                 VmaAllocator* allocator);
    AccelerationStructureData& getTLASData() {
        return tlasData.value();
    }


    void cleanUp();

    void build(std::vector<Entity>& entities, MVP& mvp);

    private:

    void buildAccelerationStructure(vk::AccelerationStructureTypeKHR accelerationStructureType,
                                    vk::raii::AccelerationStructureKHR *accelStructureHandle, uint32_t primitiveCount,
                                    vk::AccelerationStructureGeometryKHR &geometry, VkBuffer *buffer,
                                    VmaAllocation *allocation, vk::DeviceAddress *deviceAddress);


    std::vector<AccelerationStructureData>& getBLASData() {
        return blasData;
    }

    void buildBLAS();

    void buildTLAS();

    void buildTLASGeometry(std::vector<AccelerationStructureData>& blasData, std::vector<Entity>& entities, MVP& mvp);
    void buildBLASGeometry(std::vector<Entity>& entities);
};



#endif
