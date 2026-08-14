

#ifndef VULKAN_TEST_ENTITY_H
#define VULKAN_TEST_ENTITY_H
#include <iostream>

#include "Loader.h"
#include "glm/fwd.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/glm.hpp"

struct alignas(16) Material {
    glm::vec4 color;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    uint64_t vertexAddress;
    uint64_t indexAddress;
    uint64_t normalAddress;
    uint64_t textureCoordsAddress;
    float roughness = 1.0f;
    float metalness = 0.0f;
    float reflectivity = 0.04f;
    float albedoFactor = 1.0f;
    float transmissionFactor = 0.0f;
    float ior = 1.5f;
    int lightIndex = -1;
    int baseColorTextureIndex = -1;
    int normalMapTextureIndex = -1;
};

class Entity {
    Model model;
    std::optional<Material> material;
    std::vector<float>* emissiveVertices;
    int emissiveStride = -1;
    int emissiveIndex = -1;
    std::string identifier;

    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::vec3 rotation = glm::vec3(0, 0, 0);
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::mat4& transform;
    glm::mat4 parentTransform = glm::mat4(1.0);

    public:
        Entity(std::string identifier, Model&& model, glm::mat4& transform);
        Entity(std::string identifier, Model&& model, Material material, glm::mat4& transform);
        glm::vec3 getPosition() const {
            return position;
        }

        void setPosition(const glm::vec3 &position) {
            this->position = position;
        }

        glm::vec3 getRotation() const {
            return rotation;
        }

        bool isEmissive();

        void setRotation(const glm::vec3 &rotation) {
            this->rotation = rotation;
        }

        glm::vec3 getScale() const {
            return scale;
        }

        void updateEmissiveVertices();

        void setScale(const glm::vec3 &scale) {
            this->scale = scale;
        }

        void setEmission(std::vector<float>* vertices, int index, int stride) {
            this -> emissiveVertices = vertices;
            this -> emissiveIndex = index;
            this -> emissiveStride = stride;

        }

        Material getMaterial();
        Model& getModel();
        void setMaterial(Material material);

        void setVertexAddress(uint64_t vertexAddress);
        void setIndexAddress(uint64_t indexAddress);
        void setNormalAddress(uint64_t normalAddress);
        void setTextureCoordsAddress(uint64_t normalAddress);

        std::string getIdentifier();

        void updateTransformationMatrix();
        void increasePosition(glm::vec3 v);
        void increaseRotation(glm::vec3 v);
        void increaseScale(glm::vec3 v);

        void setParentTransform(glm::mat4& parentTransform);


        bool hasMaterial();
        void setTransform(const glm::mat4& matrix);


};



#endif //VULKAN_TEST_ENTITY_H
