
#ifndef VULKAN_TEST_SCENEMANAGER_H
#define VULKAN_TEST_SCENEMANAGER_H
#include <variant>
#include <vulkan/vulkan_raii.hpp>
#include "../Entity.h"
#include "glm/fwd.hpp"
#include "glm/vec3.hpp"
#include "glm/glm.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "../VulkanCommon.h"
struct MVP;
class Loader;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

struct WorldObject {
    std::vector<Entity> entities;
    glm::mat4 transform = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    float rotationAngle = 0.0f;
    glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);

    void updateTransform() {
        transform = glm::mat4(1.0f);
        transform = glm::translate(transform, position);
        transform = glm::rotate(transform, glm::radians(rotationAngle), rotationAxis);
        transform = glm::scale(transform, scale);
    }

    void setPosition(glm::vec3 pos) {
        position = pos;
        updateTransform();
    }

    void setScale(glm::vec3 s) {
        scale = s;
        updateTransform();
    }

    void setRotation(float angle, glm::vec3 axis) {
        rotationAngle = angle;
        rotationAxis = axis;
        updateTransform();
    }
};

class SceneManager {


    //std::vector<Entity> entities;

    std::vector<std::variant<Entity, WorldObject>> entities;
    std::vector<float> emissiveVertices;
    std::vector<float> triangleCDFBuffer;
    std::vector<float> lightCDFBuffer;
    std::vector<LightData> lightData;


    public:
        SceneManager(Loader& loader, vk::raii::Device& device, vk::raii::PhysicalDevice& physicalDevice, MVP& mvp);

        void updateAndDrawEntities(std::function<void()> updateTransform,  std::function<void(Entity&)> draw);

        //Use as the source of truth. Do not store pointers or else I will get dangling pointer...
        std::vector<Entity *> getEntities() {
            std::vector<Entity*> finalEntities;
            for (std::variant<Entity, WorldObject>& entity : entities) {
                std::visit(overloaded {
                    [&finalEntities] (Entity& e) {
                        finalEntities.push_back(&e);
                    },
                    [&finalEntities](WorldObject& o) {
                        for (Entity& entity : o.entities) {
                            entity.setParentTransform(o.transform);
                            finalEntities.push_back(&entity);
                        }
                    },
                }, entity);
            }
            return finalEntities;
        }

        std::vector<Material> getAllMaterials();

        std::vector<float>& getTriangleCDFBuffer();

        std::vector<float>& getLightCDFBuffer();

        std::vector<float>& getEmissiveVertices();

        std::vector<LightData>& getLightData();

        private:
        static void buildCDF(std::vector<float>& emissiveVertices, std::vector<float>& triangleCDFBuffer, std::vector<LightData>& lightData, std::vector<float>& lightCDFBuffer);
        static float getTriangleSurfaceArea(std::array<glm::vec3, 3> vertices);

};



#endif //VULKAN_TEST_SCENEMANAGER_H
