#ifndef VULKAN_TEST_SHADERREGISTRY_H
#define VULKAN_TEST_SHADERREGISTRY_H
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ShaderProgram.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "ShaderPipeline.h"
#include "vk_mem_alloc.h"






class ShaderPair : public ShaderPipeline {
    std::optional<vk::PipelineRenderingCreateInfo> rasterPipeline;
    std::optional<vk::raii::PipelineLayout> rasterPipelineLayout;
    std::optional<vk::raii::Pipeline> rasterGraphicsPipeline;
    int numColorAttachments;

    public:

    ShaderPair(std::string str, vk::raii::Device &device, vk::Format &swapChainImageFormat, VmaAllocator &allocator,
               DescriptorsInfo desc, int numColorAttachments);

        std::string getIdentifier() const;
        void bind(vk::raii::CommandBuffer &cmd, int frameIndex) override;

    private:


        void setUpPipeline() override;
};



#endif //VULKAN_TEST_SHADERREGISTRY_H
