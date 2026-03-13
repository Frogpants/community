#pragma once

#include "../core/essentials.hpp"
#include "../core/collision.hpp"
#include <string>

struct Task {
    vec2 pos = vec2(0.0);
    vec2 dim = vec2(45.0);

    int id = 0;

    int room = 0;
};

inline vec2 GetTaskSpawnPosition(const std::string& taskName, int taskIndex) {
    if (taskName == "take out trash") {
        return vec2(-300.0f, -250.0f);
    }

    if (taskName == "wash dishes") {
        return vec2(0.0f, -250.0f);
    }

    if (taskName == "do laundry") {
        return vec2(300.0f, -250.0f);
    }

    float startX = -300.0f;
    float spacing = 300.0f;
    return vec2(startX + spacing * taskIndex, -250.0f);
}