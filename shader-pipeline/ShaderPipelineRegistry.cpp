
#include "ShaderPipelineRegistry.h"

void ShaderPipelineRegistry::registerShaderPipeline(std::unique_ptr<ShaderPipeline> pipeline) {
    shaderPipelines[pipeline -> getIdentifier()] = std::move(pipeline);
}

const std::unique_ptr<ShaderPipeline>& ShaderPipelineRegistry::getShaderPipeline(std::string id) const {
    return shaderPipelines.at(id);
}



void ShaderPipelineRegistry::cleanUp() {
    for (const auto& ptr : shaderPipelines) {
        ptr.second -> cleanUp();
    }
}

