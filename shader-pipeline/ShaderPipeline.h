
#ifndef VULKAN_TEST_SHADERPIPELINE_H
#define VULKAN_TEST_SHADERPIPELINE_H
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "ShaderProgram.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "vk_mem_alloc.h"

struct DescriptorLayoutDesc {
    uint32_t numUBOs;
    uint32_t numTextureSamplers;
    uint32_t numAccelerationStructures;
};
struct DescriptorsInfo {
    DescriptorLayoutDesc staticData;
    DescriptorLayoutDesc dynamicData;
};



struct DescriptorBinding {
    uint32_t set;
    uint32_t binding;

    bool operator==(const DescriptorBinding& other) const {
        return set == other.set && binding == other.binding;
    }
};
struct DescriptorKeyHash {
    std::size_t operator()(const DescriptorBinding& k) const {
        std::size_t h1 = std::hash<uint32_t>{}(k.set);
        std::size_t h2 = std::hash<uint32_t>{}(k.binding);
        return h1 ^ (h2 << 1);
    }
};
class ShaderPipeline {



    protected:
        const VmaAllocator* allocator;
        vk::raii::Device* device;
        vk::Format* swapChainImageFormat;
        std::array<std::vector<vk::raii::DescriptorSet>, 3> descriptorSets;
        std::optional<vk::raii::DescriptorSetLayout> onceAddedLayout;
        std::optional<vk::raii::DescriptorSetLayout> perFrameLayout;
        std::vector<vk::DescriptorSetLayout> layouts;
        std::array<std::unordered_map<DescriptorBinding, VmaAllocation, DescriptorKeyHash>, 3> uboAllocations;
        std::array<std::unordered_map<DescriptorBinding, vk::Buffer, DescriptorKeyHash>, 3> uboBuffers;
        DescriptorsInfo desc;
        std::string identifier;
        std::unordered_map<VkBuffer, void*> persistentUBOPointers;
        std::optional<vk::raii::DescriptorPool> descriptorPool;
        std::vector<ShaderProgram> shaders;
    public:
        virtual ~ShaderPipeline() = default;


        ShaderPipeline(std::string str, vk::raii::Device &device, vk::Format &swapChainImageFormat, VmaAllocator &allocator,
                       DescriptorsInfo desc);

        void setUniform(DescriptorBinding descriptorBinding, void *data, uint32_t size, int frameIndex);

        virtual void cleanUp();

        std::string getIdentifier() const;


        virtual void bind(vk::raii::CommandBuffer& cmd, int frameIndex) = 0;
    protected:
        void setUpDescriptors();
        virtual void setUpPipeline() = 0;
        vk::DescriptorSetLayoutCreateInfo getDescriptorSetCreateInfo(DescriptorLayoutDesc desc,
                                                                     std::vector<vk::DescriptorSetLayoutBinding> &bindings);

        std::vector<vk::PipelineShaderStageCreateInfo> getStageCreateInfos();

        virtual vk::ShaderStageFlags getShaderStageFlags();
};



#endif //VULKAN_TEST_SHADERPIPELINE_H
