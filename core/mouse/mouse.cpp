#include "mouse.hpp"
#include <GLFW/glfw3.h>
#include <unordered_map>

namespace {

    struct ButtonState {
        bool down = false;
        bool pressed = false;
        bool released = false;
    };

    std::unordered_map<int, ButtonState> buttons;

    double mouseX = 0.0;
    double mouseY = 0.0;
    double lastX = 0.0;
    double lastY = 0.0;
    double deltaX = 0.0;
    double deltaY = 0.0;

    double scrollX = 0.0;
    double scrollY = 0.0;

    GLFWwindow* g_window = nullptr;
    bool mouseLocked = false;

    void CursorPosCallback(GLFWwindow*, double x, double y) {
        mouseX = x;
        mouseY = y;
    }

    void MouseButtonCallback(GLFWwindow* window, int button, int action, int) {
        auto& state = buttons[button];

        if (action == GLFW_PRESS) {
            state.down = true;
            state.pressed = true;

            // if (!mouseLocked && button == GLFW_MOUSE_BUTTON_LEFT) {
            //     mouseLocked = true;
            //     glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            // }
        }
        else if (action == GLFW_RELEASE) {
            state.down = false;
            state.released = true;
        }
    }

    void ScrollCallback(GLFWwindow*, double xoff, double yoff) {
        scrollX += xoff;
        scrollY += yoff;
    }
}

namespace Mouse {

    void Init(GLFWwindow* window) {
        g_window = window;

        glfwSetCursorPosCallback(window, CursorPosCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
        glfwSetScrollCallback(window, ScrollCallback);

        // Center cursor in the window on init so it doesn't start at (0,0)
        int w = 0, h = 0;
        glfwGetWindowSize(window, &w, &h);
        double centerX = w / 2.0;
        double centerY = h / 2.0;

        glfwSetCursorPos(window, centerX, centerY);

        // Initialize internal positions to the centered cursor
        lastX = centerX;
        lastY = centerY;
        mouseX = centerX;
        mouseY = centerY;
    }

    void Update() {
        // Compute relative movement
        deltaX = mouseX - lastX;
        deltaY = mouseY - lastY;

        // Update last position
        lastX = mouseX;
        lastY = mouseY;

        // Reset scroll
        scrollX = 0.0;
        scrollY = 0.0;

        // Reset pressed/released
        for (auto& [_, state] : buttons) {
            state.pressed = false;
            state.released = false;
        }
    }

    double X() { return mouseX; }
    double Y() { return mouseY; }

    double DeltaX() { return deltaX; }
    double DeltaY() { return deltaY; }

    bool IsDown(int button) { return buttons[button].down; }
    bool IsPressed(int button) { return buttons[button].pressed; }
    bool IsReleased(int button) { return buttons[button].released; }

    double ScrollX() { return scrollX; }
    double ScrollY() { return scrollY; }

    void Lock() {
        if (g_window) {
            mouseLocked = true;
            glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    void Unlock() {
        if (g_window) {
            mouseLocked = false;
            glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    bool IsLocked() { return mouseLocked; }
}
