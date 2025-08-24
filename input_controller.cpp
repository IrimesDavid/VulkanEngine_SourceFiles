#include "input_controller.h"

namespace lve {
    bool InputController::cursorEnabled = true;
	bool InputController::firstMouse = true;
    StatusBar* InputController::statusBar = nullptr;

    void InputController::moveInSpace(float dt, LveGameObject& gameObject) {
       
        float yaw = gameObject.transform.rotation.y;
        float pitch = gameObject.transform.rotation.x;

        // Compute full 3D movement directions
        const glm::vec3 forwardDir{ cos(pitch) * sin(yaw), -sin(pitch), cos(pitch) * cos(yaw) };
        const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };  // Stays the same
        const glm::vec3 upDir{ 0.f, -1.f, 0.f };  // This is world up, but we can change it dynamically

        glm::vec3 moveDir{ 0.f };
        if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
        if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
        if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
        if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
        if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir.y -= 1.f;  // Move freely up
        if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir.y += 1.f;  // Move freely down

        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) { // check that move isn't 0
            gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
        }
    }

    void InputController::processMouseMovement(float dt, LveGameObject& gameObject) {
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
            return; // If cursor is not disabled, do nothing
        
        // Get current cursor position
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Get window size
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // If it's the first frame, initialize lastX and lastY
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        // Calculate mouse movement (delta)
        float offsetX = static_cast<float>(xpos - lastX);
        float offsetY = static_cast<float>(lastY - ypos); 

        lastX = xpos;
        lastY = ypos;

        // Apply sensitivity
        offsetX *= lookSpeed;
        offsetY *= lookSpeed;

        // Update rotation angles
        gameObject.transform.rotation.y += offsetX * dt;
        gameObject.transform.rotation.x += offsetY * dt;

        // Limit pitch to avoid flipping
        gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
        //gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());
    }

    void InputController::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
            statusBar->reloadResources = true;
            statusBar->command = "MSAA1";
        }

        if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
			statusBar->reloadResources = true;
			statusBar->command = "MSAA2";
        }
        if (key == GLFW_KEY_F4 && action == GLFW_PRESS) {
            statusBar->reloadResources = true;
            statusBar->command = "MSAA4";
        }
        if (key == GLFW_KEY_F8 && action == GLFW_PRESS) {
            statusBar->reloadResources = true;
            statusBar->command = "MSAA8";
        }
    }

    void InputController::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            if (glfwGetWindowAttrib(window, GLFW_HOVERED) == GLFW_TRUE) {
                cursorEnabled = !cursorEnabled;
                if (cursorEnabled) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
                else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
					firstMouse = true; // Reset first mouse flag when enabling cursor
                }
            }
        }
    }
}