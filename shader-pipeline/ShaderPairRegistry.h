

#ifndef VULKAN_TEST_SHADERPAIRREGISTRY_H
#define VULKAN_TEST_SHADERPAIRREGISTRY_H
#include <memory>
#include <vector>

#include "ShaderPair.h"


class ShaderPairRegistry {
    std::unordered_map<std::string, std::unique_ptr<ShaderPair>> shaderPairs;

    public: void registerShaderPair(std::unique_ptr<ShaderPair>);

    const std::unique_ptr<ShaderPair> &getShaderPair(std::string id) const;
    
    void cleanUp();
};



#endif //VULKAN_TEST_SHADERPAIRREGISTRY_H
