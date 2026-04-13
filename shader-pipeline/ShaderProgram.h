

#ifndef VULKAN_TEST_SHADERPROGRAM_H
#define VULKAN_TEST_SHADERPROGRAM_H

#include <optional>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>


class ShaderProgram {
    std::optional<vk::PipelineShaderStageCreateInfo> createInfo;
    std::optional<vk::raii::ShaderModule> shaderModule;

    public:

    ShaderProgram(std::string filename, vk::raii::Device *device, vk::ShaderStageFlagBits type);

    const vk::PipelineShaderStageCreateInfo& getCreateInfo() const;
};




#endif //VULKAN_TEST_SHADERPROGRAM_H
