#include "ShaderPair.h"

ShaderPair::ShaderPair(std::string str, vk::raii::Device& device, std::vector<vk::Format> imageFormats, VmaAllocator& allocator, DescriptorsInfo desc, int numColorAttachments) : ShaderPipeline(str, device, allocator, desc) {
    setUpDescriptors();
    ShaderProgram vertex(str + "Vertex.vert.spv", &device, vk::ShaderStageFlagBits::eVertex);
    ShaderProgram fragment(str + "Fragment.frag.spv", &device, vk::ShaderStageFlagBits::eFragment);
    shaders.push_back(std::move(vertex));
    shaders.push_back(std::move(fragment));
    this -> numColorAttachments = numColorAttachments;
    this -> imageFormats = imageFormats;
    ShaderPair::setUpPipeline();
}







void ShaderPair::setUpPipeline() {
    rasterPipeline.emplace(0, numColorAttachments, imageFormats.data(), vk::Format::eD32Sfloat);
    uint32_t count =
        ((desc.staticData.numUBOs || desc.staticData.numTextureSamplers) ? 1 : 0) +
        ((desc.dynamicData.numUBOs || desc.dynamicData.numTextureSamplers) ? 1 : 0);
    std::vector<vk::PushConstantRange> pushRanges;
    vk::PushConstantRange pushRange(
        vk::ShaderStageFlagBits::eVertex,
        0,
        192
    );
    vk::PushConstantRange pushRange2(
        vk::ShaderStageFlagBits::eFragment,
        0,
        192
    );

    pushRanges.push_back(pushRange);
    pushRanges.push_back(pushRange2);
    vk::PipelineLayoutCreateInfo layoutInfo({}, count, layouts.data(), 2, pushRanges.data());
    rasterPipelineLayout.emplace(*device, layoutInfo);

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo(
        {},
        vk::PrimitiveTopology::eTriangleList
    );
    vk::PipelineViewportStateCreateInfo viewportStateInfo(
        {},
        1,
        nullptr,
        1,
        nullptr
    );

    vk::PipelineRasterizationStateCreateInfo rasterCreateInfo(
        {},
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eCounterClockwise,
        VK_FALSE,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    );

    vk::DynamicState dynamicStates[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState({}, 2, dynamicStates);

    //TODO: Add normals & textures to vertex arrays if I want to go further with this
    std::array<vk::VertexInputBindingDescription, 3> bindingDesc = {
        vk::VertexInputBindingDescription(0, sizeof(float) * 3, vk::VertexInputRate::eVertex),
        vk::VertexInputBindingDescription(1, sizeof(float) * 3, vk::VertexInputRate::eVertex),
        vk::VertexInputBindingDescription(2, sizeof(float) * 2, vk::VertexInputRate::eVertex)
    };
    std::array<vk::VertexInputAttributeDescription, 3> attributes = {
        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, 0),
        vk::VertexInputAttributeDescription(1, 1, vk::Format::eR32G32B32Sfloat, 0),
        vk::VertexInputAttributeDescription(2, 2, vk::Format::eR32G32Sfloat, 0)
    };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, 3, bindingDesc.data(), 3, attributes.data());


    std::vector<vk::PipelineShaderStageCreateInfo> info = getStageCreateInfos();

    std::vector<vk::PipelineColorBlendAttachmentState> blendAttachments;
    for (int i = 0; i < numColorAttachments; i++) {
        vk::PipelineColorBlendAttachmentState blendAttachment(VK_FALSE);
        blendAttachment.colorWriteMask =
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA;
        blendAttachments.push_back(blendAttachment);
    }

    vk::PipelineColorBlendStateCreateInfo colorBlendInfo(
        {},
        VK_FALSE,
        vk::LogicOp::eCopy,
        numColorAttachments,
        blendAttachments.data()
    );

    vk::PipelineMultisampleStateCreateInfo multisampleInfo(
        {},
        vk::SampleCountFlagBits::e1
    );


    vk::PipelineDepthStencilStateCreateInfo depthStencil(
        {},
        true,
        true,
        vk::CompareOp::eLess,
        false,
        false
    );


    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
        {},
        info.size(),
        info.data(),
        &vertexInputInfo,
        &inputAssemblyCreateInfo,
        nullptr,
        &viewportStateInfo,
        &rasterCreateInfo,
        &multisampleInfo,
        &depthStencil,
        &colorBlendInfo,
        &dynamicState,
        *rasterPipelineLayout,
        nullptr,
        0,
        nullptr,
        0,
        &rasterPipeline


    );

    rasterGraphicsPipeline.emplace(*device, nullptr, graphicsPipelineCreateInfo);
}








void ShaderPair::bind(vk::raii::CommandBuffer& commandBuffer, int frameIndex) {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *rasterGraphicsPipeline);

    std::vector<uint32_t> offsets(0, 0);
    std::vector<vk::DescriptorSet> rawSets;
    rawSets.reserve(descriptorSets[frameIndex].size());

    for (const auto& set : descriptorSets[frameIndex]) {
        rawSets.push_back(*set);
    }
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *rasterPipelineLayout, 0, rawSets, offsets);


}

vk::raii::PipelineLayout & ShaderPair::getPipelineLayout() {
    return *this -> rasterPipelineLayout;
}













