
#ifndef VULKAN_TEST_MODELLOADER_H
#define VULKAN_TEST_MODELLOADER_H
#include "Loader.h"


class ModelLoader {
    public:
    Model load(std::string name, Loader &loader, vk::raii::Device &device, vk::raii::PhysicalDevice &physicalDevice);
};



#endif //VULKAN_TEST_MODELLOADER_H
