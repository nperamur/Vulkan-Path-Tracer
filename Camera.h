

#ifndef VULKAN_TEST_CAMERA_H
#define VULKAN_TEST_CAMERA_H
#include "glm/fwd.hpp"
#include "glm/vec3.hpp"


class Camera {
    glm::vec3 position = glm::vec3(0.0f, 0.2f, 3.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    float lastTime = 0.0f;
    float deltaTime = 0.0f;
    float cameraRotationSpeed = 2.5f;
    float movementSpeed = 2;

    public:
    glm::mat4 createViewMatrix();

    void handleInputs();
};



#endif //VULKAN_TEST_CAMERA_H
