

#include "Entity.h"

#include <utility>
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"


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
    transformation = glm::rotate(transformation, rotation.x, glm::vec3(1, 0, 0));
    transformation = glm::rotate(transformation, rotation.y, glm::vec3(0, 1, 0));
    transformation = glm::rotate(transformation, rotation.z, glm::vec3(0, 0, 1));
    this -> transform = transformation;
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



bool Entity::hasMaterial() {
    return this -> material != std::nullopt;
}

Material& Entity::getMaterial() {
    return this -> material.value();
}

Model& Entity::getModel() {
    return model;
}

void Entity::setMaterial(Material& newMaterial) {
    this -> material = newMaterial;
}

std::string Entity::getIdentifier() {
    return identifier;
}




