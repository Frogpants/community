#include "input.hpp"
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <algorithm>
#include <string>

namespace {

    struct KeyState {
        bool down = false;
        bool pressed = false;
        bool released = false;
    };

    std::unordered_map<int, KeyState> keys;
    std::unordered_map<std::string, int> nameToKey;

    GLFWwindow* window = nullptr;

    void RegisterKey(const std::string& name, int key) {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        nameToKey[lower] = key;
    }

    void RegisterDefaults() {
        RegisterKey("space", GLFW_KEY_SPACE);
        RegisterKey("escape", GLFW_KEY_ESCAPE);
        RegisterKey("enter", GLFW_KEY_ENTER);

        for (char c = 'a'; c <= 'z'; ++c)
            RegisterKey(std::string(1, c), GLFW_KEY_A + (c - 'a'));

        for (int i = 0; i <= 9; ++i)
            RegisterKey(std::to_string(i), GLFW_KEY_0 + i);
    }
}

namespace Input {

    void Init(GLFWwindow* win) {
        window = win;
        RegisterDefaults();
    }

    // Call once per frame AFTER glfwPollEvents()
    void Update() {

        for (auto& [key, state] : keys) {
            bool currentlyDown = glfwGetKey(window, key) == GLFW_PRESS;

            state.pressed  = (!state.down && currentlyDown);
            state.released = (state.down && !currentlyDown);
            state.down     = currentlyDown;
        }

        // Also check keys that were never used before
        for (auto& [_, key] : nameToKey) {
            if (!keys.count(key)) {
                keys[key] = {};
            }
        }
    }

    bool IsDown(int key) {
        return keys[key].down;
    }

    bool IsPressed(int key) {
        return keys[key].pressed;
    }

    bool IsReleased(int key) {
        return keys[key].released;
    }

    int Resolve(const std::string& name) {
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (nameToKey.count(lower))
            return nameToKey[lower];

        return -1;
    }

    bool IsDown(const std::string& name) {
        int key = Resolve(name);
        return key != -1 && IsDown(key);
    }

    bool IsPressed(const std::string& name) {
        int key = Resolve(name);
        return key != -1 && IsPressed(key);
    }

    bool IsReleased(const std::string& name) {
        int key = Resolve(name);
        return key != -1 && IsReleased(key);
    }
}
