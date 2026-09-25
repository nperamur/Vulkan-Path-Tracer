
#include "Camera.h"

#include "Application.h"
#include "rendering/Renderer.h"
#include "glm/ext/matrix_transform.hpp"

glm::mat4 Camera::createViewMatrix() {
    glm::vec3 dir;
    dir.x = glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
    dir.y = glm::sin(glm::radians(pitch));
    dir.z = glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
    dir = glm::normalize(dir);
    return glm::lookAt(position, position + dir, upVector);
}

void Camera::handleInputs() {
    GLFWwindow* window = Application::get()->getWindow();

    float currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        yaw -= deltaTime * cameraRotationSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        yaw += deltaTime * cameraRotationSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        pitch += deltaTime * cameraRotationSpeed;
    }


    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        pitch -= deltaTime * cameraRotationSpeed;
    }

    if (pitch >= 89.99) pitch = 89.99;
    if (pitch <= -89.99) pitch = -89.99;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        position.x += deltaTime * movementSpeed * glm::cos(glm::radians(yaw));
        position.z += deltaTime * movementSpeed * glm::sin(glm::radians(yaw));
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        position.x -= deltaTime * movementSpeed * glm::cos(glm::radians(yaw));
        position.z -= deltaTime * movementSpeed * glm::sin(glm::radians(yaw));
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        position.x += deltaTime * movementSpeed * glm::cos(glm::radians(yaw - 90));
        position.z += deltaTime * movementSpeed * glm::sin(glm::radians(yaw - 90));
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        position.x += deltaTime * movementSpeed * glm::cos(glm::radians(yaw + 90));
        position.z += deltaTime * movementSpeed * glm::sin(glm::radians(yaw + 90));

    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        position += deltaTime * glm::vec3(0, 1, 0) * movementSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        position += deltaTime * glm::vec3(0, -1, 0) * movementSpeed;
    }
}
