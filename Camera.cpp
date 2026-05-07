
#include "Camera.h"

#include "Application.h"
#include "rendering/Renderer.h"
#include "glm/ext/matrix_transform.hpp"

glm::mat4 Camera::createViewMatrix() {
    return glm::lookAt(position, cameraTarget, upVector);
}

//TODO: later; make this time based
void Camera::handleInputs() {
    GLFWwindow* window = Application::get()->getWindow();
    glm::vec3 forward = glm::normalize(cameraTarget - position);

    float currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0,1,0)));

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        cameraTarget -= right * deltaTime * cameraRotationSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        cameraTarget += right * deltaTime * cameraRotationSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        cameraTarget += upVector * deltaTime * cameraRotationSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        cameraTarget -= upVector * deltaTime * cameraRotationSpeed;
    }
    glm::vec3 horizontalMotionMask =  glm::vec3(1, 0, 1);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        position += deltaTime * forward * horizontalMotionMask * movementSpeed;
        cameraTarget += deltaTime * forward * horizontalMotionMask * movementSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        position -= deltaTime * forward * horizontalMotionMask * movementSpeed;
        cameraTarget -= deltaTime * forward * horizontalMotionMask * movementSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        position -= deltaTime * right * horizontalMotionMask * movementSpeed;
        cameraTarget -= deltaTime * right * horizontalMotionMask * movementSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        position += deltaTime * right * horizontalMotionMask * movementSpeed;
        cameraTarget += deltaTime * right * horizontalMotionMask * movementSpeed;
    }
}
