

#include "ShaderProgram.h"


#include <fstream>
#include <iostream>
#include <string>


#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_enums.hpp>




ShaderProgram::ShaderProgram(std::string filename, vk::raii::Device* device, vk::ShaderStageFlagBits type) {
    std::ifstream file("resources/shaders/" + filename, std::ios::ate | std::ios::binary);
    std::vector<char> bytecode(file.tellg());
    file.seekg(0);
    file.read(bytecode.data(), bytecode.size());

    vk::ShaderModuleCreateInfo moduleInfo(
        {},
        bytecode.size(),
        reinterpret_cast<const uint32_t*>(bytecode.data())
    );

    shaderModule.emplace(*device, moduleInfo);

    vk::PipelineShaderStageCreateInfo stageInfo(
        {},
        type,
        *shaderModule,
        "main"
    );

    createInfo.emplace(stageInfo);
}


const vk::PipelineShaderStageCreateInfo &ShaderProgram::getCreateInfo() const {
    return *createInfo;
}