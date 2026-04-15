

#ifndef VULKAN_TEST_SHADERPAIRREGISTRY_H
#define VULKAN_TEST_SHADERPAIRREGISTRY_H
#include <memory>
#include <vector>

#include "ShaderPair.h"


class ShaderPipelineRegistry {
    std::unordered_map<std::string, std::unique_ptr<ShaderPipeline>> shaderPipelines;

    public: void registerShaderPipeline(std::unique_ptr<ShaderPipeline>);

    const std::unique_ptr<ShaderPipeline> &getShaderPipeline(std::string id) const;
    
    void cleanUp();
};



#endif //VULKAN_TEST_SHADERPAIRREGISTRY_H
