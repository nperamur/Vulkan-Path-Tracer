
#ifndef VULKAN_TEST_RAYTRACINGSHADERPIPELINE_H
#define VULKAN_TEST_RAYTRACINGSHADERPIPELINE_H
#include "ShaderPipeline.h"


struct GeometryData {
    vk::AccelerationStructureGeometryKHR geometry;
    uint32_t primitiveCount;
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

class RaytracingShaderPipeline : public ShaderPipeline {
    std::optional<vk::raii::PipelineLayout> rtPipelineLayout;
    std::optional<vk::raii::Pipeline> rtPipeline;
    std::optional<vk::raii::PhysicalDevice> physicalDevice;
    std::optional<vk::StridedDeviceAddressRegionKHR> raygenRegion;
    std::optional<vk::StridedDeviceAddressRegionKHR> missRegion;
    std::optional<vk::StridedDeviceAddressRegionKHR> closestHitRegion;
    VkBuffer sbtBuffer;
    VmaAllocation sbtAllocation;
    Geometry geometry;
    std::optional<AccelerationStructureData> tlasData;

    std::vector<AccelerationStructureData> blasData;

    public:
        RaytracingShaderPipeline(std::string string, vk::raii::Device &device, vk::raii::PhysicalDevice &physicalDevice,
                             vk::Format &swapChainImageFormat, VmaAllocator &allocator, DescriptorsInfo desc,
                             Geometry& geometry);

        void cleanUp() override;

        void bind(vk::raii::CommandBuffer &commandBuffer, int frameIndex) override;

        void buildBLAS();

        void buildTLAS();

    protected:
        void setUpPipeline() override;


        void buildAccelerationStructure(vk::AccelerationStructureTypeKHR accelerationStructureType, vk::raii::AccelerationStructureKHR *accelStructureHandle, int
                                    primitiveCount, int instanceCount, vk::AccelerationStructureGeometryKHR geometry, VkBuffer
                                    *buffer, VmaAllocation *allocation, vk::DeviceAddress *deviceAddress);

        vk::ShaderStageFlags getShaderStageFlags() override;

    void setAccelerationStructure(DescriptorBinding descriptorBinding, int frameIndex);
};



#endif //VULKAN_TEST_RAYTRACINGSHADERPIPELINE_H
