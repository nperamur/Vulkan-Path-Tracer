

#ifndef VULKAN_TEST_ENTITY_H
#define VULKAN_TEST_ENTITY_H
#include "Loader.h"
#include "glm/fwd.hpp"
#include "glm/vec3.hpp"


class Entity {
    Model model;
    std::string identifier;

    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::vec3 rotation = glm::vec3(0, 0, 0);
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::mat4& transform;

    public:
        Entity(std::string identifier, Model&& model, glm::mat4& transform);
        glm::vec3 getPosition() const {
            return position;
        }

        void setPosition(const glm::vec3 &position) {
            this->position = position;
        }

        glm::vec3 getRotation() const {
            return rotation;
        }

        void setRotation(const glm::vec3 &rotation) {
            this->rotation = rotation;
        }

        glm::vec3 getScale() const {
            return scale;
        }

        void setScale(const glm::vec3 &scale) {
            this->scale = scale;
        }

        Model& getModel();

        std::string getIdentifier();

        void updateTransformationMatrix();
            void increasePosition(glm::vec3 v);
            void increaseRotation(glm::vec3 v);
            void increaseScale(glm::vec3 v);



};



#endif //VULKAN_TEST_ENTITY_H
