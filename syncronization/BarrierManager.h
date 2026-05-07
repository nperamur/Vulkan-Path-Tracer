

#ifndef VULKAN_TEST_BARRIERMANAGER_H
#define VULKAN_TEST_BARRIERMANAGER_H
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace vk {
    struct ImageMemoryBarrier2;
}

enum ResourceAccess : uint32_t {
    read = 1 << 0,
    write = 1 << 1,
    none = 1 << 9
};

enum ResourceType: uint32_t {
    color = 1 << 2,
    depth = 1 << 3,
    sampler = 1 << 4,
    ubo = 1 << 5,
    storageImage = 1 << 6,
    vertex = 1 << 7,
    accelerationStructure = 1 << 8,
};

enum ResourceStage: uint32_t {
    topOfPipe = 1 << 12,
    bottomOfPipe = 1 << 13,
    raytracing = 1 << 14,
    earlyFragmentTests = 1 << 15,
    colorAttachmentOutput = 1 << 16,
    vertexShader = 1 << 17,
    fragmentShader = 1 << 18,
    transferStage = 1 << 19,
    allCommands = 1 << 20,
    hostStage = 1<<21,
    compute = 1<<22

};

enum ResourceIntents:uint32_t {
    transfer    = 1 << 10,
    present = 1 << 11
};



constexpr inline uint32_t operator|(uint32_t mask, ResourceAccess a) {
    return mask | static_cast<uint32_t>(a);
}

constexpr inline uint32_t operator|(uint32_t mask, ResourceType t) {
    return mask | static_cast<uint32_t>(t);
}


constexpr inline uint32_t operator|(uint32_t mask, ResourceIntents t) {
    return mask | static_cast<uint32_t>(t);
}

namespace BarrierUsage {
    constexpr uint32_t colorRead = color | read | sampler | fragmentShader;
    constexpr uint32_t colorWrite = color | write | colorAttachmentOutput;
    constexpr uint32_t depthRead = depth | read | sampler | fragmentShader;
    constexpr uint32_t depthWrite = depth | write | earlyFragmentTests;
    constexpr uint32_t transferWrite = transfer | write | transferStage;
    constexpr uint32_t presentColor = color | none | present | bottomOfPipe;
    constexpr uint32_t presentDepth = depth | none | present | bottomOfPipe;
    constexpr uint32_t colorNone =  color | none | topOfPipe;
    constexpr uint32_t depthNone =  depth | none | topOfPipe;
    constexpr uint32_t storageColorNone = none | color | allCommands;
}





class BarrierManager {
    std::vector<vk::ImageMemoryBarrier2> imageBarriers;
    std::vector<vk::BufferMemoryBarrier2> bufferBarriers;

    public:
    void begin() {
        imageBarriers.clear();
    };
    void transition(vk::Image image, uint32_t firstState, uint32_t secondState);
    void transition(vk::Buffer buffer, float bufferSize, uint32_t firstState, uint32_t secondState);
    void commit(vk::raii::CommandBuffer &commandBuffer);

    private:
    vk::PipelineStageFlagBits2 resolveStage(uint32_t state);
    vk::Flags<vk::AccessFlagBits2> resolveAccess(uint32_t state);

    vk::Flags<vk::AccessFlagBits2> resolveRead(uint32_t state);
    vk::Flags<vk::AccessFlagBits2> resolveWrite(uint32_t state);

    vk::ImageLayout resolveImageLayout(uint32_t state);
    vk::ImageAspectFlagBits resolveImageAspectFlagBits(uint32_t state, uint32_t state2);



};



#endif //VULKAN_TEST_BARRIERMANAGER_H
