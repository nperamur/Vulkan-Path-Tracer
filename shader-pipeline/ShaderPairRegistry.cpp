
#include "ShaderPairRegistry.h"

void ShaderPairRegistry::registerShaderPair(std::unique_ptr<ShaderPair> pair) {
    shaderPairs[pair -> getIdentifier()] = std::move(pair);
}

const std::unique_ptr<ShaderPair>& ShaderPairRegistry::getShaderPair(std::string id) const {
    return shaderPairs.at(id);
}



void ShaderPairRegistry::cleanUp() {
    for (const auto& pairPtr : shaderPairs) {
        pairPtr.second -> cleanUp();
    }
}

