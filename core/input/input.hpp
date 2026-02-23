#pragma once
#include <string>
struct GLFWwindow;

namespace Input {
    void Init(GLFWwindow* window);
    void Update();

    bool IsDown(int key);
    bool IsPressed(int key);
    bool IsReleased(int key);

    bool IsDown(const std::string& name);
    bool IsPressed(const std::string& name);
    bool IsReleased(const std::string& name);
}
