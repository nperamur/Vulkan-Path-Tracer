
#include "BarrierManager.h"

#include <vulkan/vulkan_raii.hpp>
/**
 *@Author: Neelesh Peramur
 */



/**
 * Transitions the image state by staging a memory barrier
 * @param image the image to transition
 * @param firstState the initial state. Can be constructed using the barrier usage namespace for simplicity or the bit-masked Resource enums for more control
 * @param secondState the final state to transition to. Can be constructed using the barrier usage namespace for simplicity or the bit-masked Resource for more control
 */
void BarrierManager::transition(vk::Image image, uint32_t firstState, uint32_t secondState) {
    vk::ImageMemoryBarrier2 barrier(
        resolveStage(firstState),
        resolveAccess(firstState),
        resolveStage(secondState),
        resolveAccess(secondState),
        resolveImageLayout(firstState),
        resolveImageLayout(secondState),
        {}, {},
        image,
        vk::ImageSubresourceRange(resolveImageAspectFlagBits(firstState, secondState), 0, 1, 0, 1)
    );
    imageBarriers.push_back(barrier);

}


/**
 * Transitions the buffer state by staging a memory barrier
 * @param buffer the buffer to transition
 * @param firstState the initial state. Can be constructed using the barrier usage namespace for simplicity or the bit-masked Resource for more control
 * @param secondState the final state to transition to. Can be constructed using the barrier usage namespace for simplicity or the bit-masked Resource for more control
 */
void BarrierManager::transition(vk::Buffer buffer, float bufferSize, uint32_t firstState, uint32_t secondState) {
    vk::BufferMemoryBarrier2 barrier(
    resolveStage(firstState),
    resolveAccess(firstState),
    resolveStage(secondState),
    resolveAccess(secondState),
    0,
    0,
    buffer, {}, bufferSize);
    bufferBarriers.push_back(barrier);

}



/**
 * Uploads all the staged memory barriers to the command buffer
 * @param commandBuffer the command buffer
 */
void BarrierManager::commit(vk::raii::CommandBuffer& commandBuffer) {
    vk::DependencyInfo depInfo({}, {}, bufferBarriers, imageBarriers);
    commandBuffer.pipelineBarrier2(depInfo);
    imageBarriers.clear();
    bufferBarriers.clear();
}


vk::PipelineStageFlagBits2 BarrierManager::resolveStage(uint32_t state) {
    if (state & topOfPipe) return vk::PipelineStageFlagBits2::eTopOfPipe;
    if (state & bottomOfPipe) return vk::PipelineStageFlagBits2::eBottomOfPipe;
    if (state & raytracing) return vk::PipelineStageFlagBits2::eRayTracingShaderKHR;
    if (state & earlyFragmentTests) return vk::PipelineStageFlagBits2::eEarlyFragmentTests;
    if (state & colorAttachmentOutput) return vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    if (state & vertexShader) return vk::PipelineStageFlagBits2::eVertexShader;
    if (state & fragmentShader) return vk::PipelineStageFlagBits2::eFragmentShader;
    if (state & transferStage) return vk::PipelineStageFlagBits2::eTransfer;
    if (state & allCommands) return vk::PipelineStageFlagBits2::eAllCommands;
    if (state & hostStage) return vk::PipelineStageFlagBits2::eHost;
    throw std::runtime_error("Cannot resolve empty stage");
}

vk::Flags<vk::AccessFlagBits2> BarrierManager::resolveAccess(uint32_t state) {
    if (state & none) {
        return vk::AccessFlagBits2::eNone;
    }
    vk::Flags<vk::AccessFlagBits2> flags = ((state & read) ? resolveRead(state) : vk::AccessFlagBits2::eNone) |
                    ((state & write) ? resolveWrite(state) : vk::AccessFlagBits2::eNone);
    return flags;
}

vk::Flags<vk::AccessFlagBits2> BarrierManager::resolveRead(uint32_t state) {
    vk::AccessFlagBits2 samplerBit = (state & sampler) ? vk::AccessFlagBits2::eShaderSampledRead : vk::AccessFlagBits2::eNone;
    if (state & transfer) return vk::AccessFlagBits2::eTransferRead;
    if (state & storageImage) return (samplerBit != vk::AccessFlagBits2::eNone) ? samplerBit : vk::AccessFlagBits2::eShaderStorageRead;
    if (state & ubo) return vk::AccessFlagBits2::eUniformRead;
    if (state & vertex) return vk::AccessFlagBits2::eVertexAttributeRead;
    if (state & accelerationStructure) return vk::AccessFlagBits2::eAccelerationStructureReadKHR;
    if (state & color) return samplerBit == vk::AccessFlagBits2::eNone ? vk::AccessFlagBits2::eColorAttachmentRead : samplerBit;
    if (state & depth) return samplerBit == vk::AccessFlagBits2::eNone ? vk::AccessFlagBits2::eDepthStencilAttachmentRead: samplerBit;
    return samplerBit;
}


vk::Flags<vk::AccessFlagBits2> BarrierManager::resolveWrite(uint32_t state) {
    if (state & transfer) return vk::AccessFlagBits2::eTransferWrite;
    if (state & storageImage) return vk::AccessFlagBits2::eShaderStorageWrite;
    if (state & accelerationStructure) return vk::AccessFlagBits2::eAccelerationStructureWriteKHR;
    if (state & color) return vk::AccessFlagBits2::eColorAttachmentWrite;
    if (state & depth) return vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
    if (state & ubo) return vk::AccessFlagBits2::eHostWrite;
    return vk::AccessFlagBits2::eNone;
}

vk::ImageLayout BarrierManager::resolveImageLayout(uint32_t state) {
    if ((state & color) && (state & write) && !(state & storageImage)) {
        return vk::ImageLayout::eColorAttachmentOptimal;
    } else if ((state & color) && !(state & write) && (state & read)) {
        return vk::ImageLayout::eShaderReadOnlyOptimal;
    }
    if ((state & depth) && !(state & write) && (state & read)) {
        return vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal;
    } else if (state & depth && (state & write)) {
        return vk::ImageLayout::eDepthAttachmentOptimal;
    }
    if (state & storageImage) {
        return vk::ImageLayout::eGeneral;
    }
    if (state & present) {
        return vk::ImageLayout::ePresentSrcKHR;
    }
    if (state & transfer) {
        return (state & write) ? vk::ImageLayout::eTransferDstOptimal : vk::ImageLayout::eTransferSrcOptimal;;
    }
    return vk::ImageLayout::eUndefined;
}

vk::ImageAspectFlagBits BarrierManager::resolveImageAspectFlagBits(uint32_t state, uint32_t state2) {
    if ((state & color) || (state2 & color)) return vk::ImageAspectFlagBits::eColor;
    if ((state & depth) || (state2 & depth)) return vk::ImageAspectFlagBits::eDepth;
    throw std::runtime_error("Cannot resolve empty aspect type");
}











