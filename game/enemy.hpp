#pragma once

#include "../core/essentials.hpp"
#include "player.hpp"

#include <vector>
#include <iostream>


struct Enemy {
    vec2 pos = vec2(0.0);
    vec2 dim = vec2(45.0);
    float health = 100.0;

    float cooldown = 0.0;

    float speed = 0.4;

    vec2 target;

    void follow() {
        vec2 dir = target - pos;
        dir = normalize(dir);
        pos = pos + dir * speed;
    }
};

inline int findIndex(const std::vector<Enemy>& v, const Enemy& obj) {
    for (size_t i = 0; i < v.size(); ++i) {
        if (&v[i] == &obj) {
            return static_cast<int>(i);
        }
    }
    return -1;
}