

#include "Entity.h"

#include <utility>
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

Entity::Entity(std::string identifier, Model&& model, glm::mat4& transform) : transform(transform), model(std::move(model)) {
    this->identifier = std::move(identifier);
}

Entity::Entity(std::string identifier, Model&& model, Material material, glm::mat4& transform) : transform(transform), model(std::move(model)) {
    this->identifier = std::move(identifier);
    this -> material = material;
}

void Entity::updateTransformationMatrix() {
    glm::mat4 transformation = glm::mat4(1.0f);
    transformation = glm::translate(transformation, position);
    transformation = glm::scale(transformation, scale);
    transformation = glm::rotate(transformation, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    transformation = glm::rotate(transformation, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    transformation = glm::rotate(transformation, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    this -> transform = parentTransform * transformation;
    this -> material -> modelMatrix = parentTransform * transformation;
}

void Entity::updateEmissiveVertices() {
    if (emissiveIndex >= 0 && emissiveStride > 0) {
        for (int i = emissiveIndex; i < emissiveIndex + emissiveStride; i+=3) {

            glm::vec4 vertex = glm::vec4((*emissiveVertices)[i],
                                         (*emissiveVertices)[i + 1],
                                         (*emissiveVertices)[i + 2],
                                         1.0f);

            glm::vec4 p = transform * vertex;

            auto newVertex = glm::vec3(p);

            (*emissiveVertices)[i] = newVertex.x;
            (*emissiveVertices)[i + 1] = newVertex.y;
            (*emissiveVertices)[i + 2] = newVertex.z;

        }
    }
}

bool Entity::isEmissive() {
    return emissiveIndex >= 0;
}

void Entity::increasePosition(glm::vec3 v) {
    position += v;
}

void Entity::increaseRotation(glm::vec3 v) {
    rotation += v;
}

void Entity::increaseScale(glm::vec3 v) {
    scale += v;
}

void Entity::setParentTransform(glm::mat4& parentTransformation) {
    this -> parentTransform = parentTransformation;
}


bool Entity::hasMaterial() {
    return this -> material != std::nullopt;
}

Material Entity::getMaterial() {
    return this -> material.value();
}

Model& Entity::getModel() {
    return model;
}

void Entity::setMaterial(Material newMaterial) {
    this -> material = newMaterial;
}

void Entity::setVertexAddress(uint64_t vertexAddress) {
    this -> material -> vertexAddress = vertexAddress;
}

void Entity::setIndexAddress(uint64_t indexAddress) {
    this -> material -> indexAddress = indexAddress;
}

void Entity::setNormalAddress(uint64_t normalAddress) {
    this -> material -> normalAddress = normalAddress;
}

void Entity::setTextureCoordsAddress(uint64_t textureCoordsAddress) {
    this -> material -> textureCoordsAddress = textureCoordsAddress;
}

std::string Entity::getIdentifier() {
    return identifier;
}


void Entity::setTransform(const glm::mat4& matrix)
{
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::quat orientation;

    glm::decompose(
        matrix,
        scale,
        orientation,
        position,
        skew,
        perspective
    );

    rotation = glm::eulerAngles(orientation);

}



