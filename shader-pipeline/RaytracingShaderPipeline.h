
#ifndef VULKAN_TEST_RAYTRACINGSHADERPIPELINE_H
#define VULKAN_TEST_RAYTRACINGSHADERPIPELINE_H
#include "ShaderPipeline.h"


struct AccelerationStructureData;



enum class RaytracingRegion {
    rayGen,
    miss,
    closestHit,
    anyHit,
    intersection,
    callable
};

class RaytracingShaderPipeline : public ShaderPipeline {
    std::optional<vk::raii::PipelineLayout> rtPipelineLayout;
    std::optional<vk::raii::Pipeline> rtPipeline;
    std::optional<vk::raii::PhysicalDevice> physicalDevice;
    std::optional<vk::StridedDeviceAddressRegionKHR> raygenRegion;
    std::optional<vk::StridedDeviceAddressRegionKHR> missRegion;
    std::optional<vk::StridedDeviceAddressRegionKHR> closestHitRegion;
    vk::StridedDeviceAddressRegionKHR emptyRegion{};
    VkBuffer sbtBuffer;
    VmaAllocation sbtAllocation;
    int numMaterials;

    public:
        RaytracingShaderPipeline(std::string string, vk::raii::Device &device, vk::raii::PhysicalDevice &physicalDevice,
                              VmaAllocator &allocator, DescriptorsInfo desc, int numMaterials);

        void cleanUp() override;

        void bind(vk::raii::CommandBuffer &commandBuffer, int frameIndex) override;

        void setAccelerationStructure(DescriptorBinding descriptorBinding, AccelerationStructureData &tlasData, int frameIndex);

        vk::StridedDeviceAddressRegionKHR* getRegion(RaytracingRegion region) {
            switch (region) {
                case RaytracingRegion::rayGen:
                    return &*raygenRegion;
                case RaytracingRegion::miss:
                    return &*missRegion;
                case RaytracingRegion::closestHit:
                    return &*closestHitRegion;
                case RaytracingRegion::intersection:
                    return &emptyRegion;
                case RaytracingRegion::anyHit:
                    return &emptyRegion;
                case RaytracingRegion::callable:
                    return &emptyRegion;
            }
            return &emptyRegion;
        }

    protected:
        void setUpPipeline() override;

        vk::ShaderStageFlags getShaderStageFlags() override;

};



#endif //VULKAN_TEST_RAYTRACINGSHADERPIPELINE_H
