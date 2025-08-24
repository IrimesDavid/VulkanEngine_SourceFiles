#pragma once 

#include "lve_game_object.h"
#include "lve_window.h"
#include "lve_utils.h"

namespace lve {
	class InputController {
	public:
        struct KeyMappings {
            int moveLeft = GLFW_KEY_A;
            int moveRight = GLFW_KEY_D;
            int moveForward = GLFW_KEY_W;
            int moveBackward = GLFW_KEY_S;
            int moveUp = GLFW_KEY_SPACE;
            int moveDown = GLFW_KEY_LEFT_CONTROL;
        };

        void bindStatusBar(StatusBar* statusBar) {
            this->statusBar = statusBar;
        }
        void bindWindow(GLFWwindow* window) { 
            this->window = window; 
            glfwSetKeyCallback(this->window, key_callback);
            glfwSetMouseButtonCallback(this->window, mouse_button_callback);
        }
        void moveInSpace(float dt, LveGameObject &gameObject);
        void processMouseMovement(float dt, LveGameObject& gameObject);

        static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

        KeyMappings keys{};
        float moveSpeed{ 3.f };
        float lookSpeed{ 1.5f };

        // Store last mouse position
        static bool cursorEnabled;  // Track cursor state
        static bool firstMouse;  // To avoid jump when first moving mouse
        double lastX{ 0.0 }, lastY{ 0.0 };

    private:
		GLFWwindow* window; //reference to the window for key and mouse callbacks
		static StatusBar* statusBar; //reference to the status bar for commands
	};
}