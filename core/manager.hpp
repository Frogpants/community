#pragma once

#include <GLFW/glfw3.h>

#include "input/input.hpp"
#include "mouse/mouse.hpp"
#include "essentials.hpp"

#include "../game/bullet.hpp"

inline float gamemode = 0.0f;

inline vec2 screen = vec2(1280.0, 720.0);

// namespace KeyMaps {
//     inline controls = 
// }

namespace Manager {

    inline GLFWwindow* window = nullptr;

    inline void Init(GLFWwindow* win) {
        window = win;

        Input::Init(window);
        Mouse::Init(window);
    }

    inline void Update() {
        Input::Update();
        Mouse::Update();
    }
}

inline float pointAt(vec2 obj1, vec2 obj2) {
    vec2 diff = obj2 - obj1;
    return atan2(diff.y, diff.x);
}

inline int randInt(int a, int b) {
    return floor((rand() % (b - a + 1)) + a);
}