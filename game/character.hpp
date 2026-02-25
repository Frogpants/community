#pragma once

#include "../core/essentials.hpp"
#include "../core/manager.hpp"
#include "../core/images/image.hpp"
#include "player.hpp"

#include <vector>
#include <iostream>


struct Character {
    vec2 pos = vec2(0.0);
    vec2 dim = vec2(45.0);
    float health = 100.0;

    GLuint texture = Image::Load("../assets/null.png");

    float cooldown = 0.0;

    float speed = 0.4;

    vec2 target;

    std::vector<vec2> positions = {};

    void follow() {
        vec2 dir = target - pos;
        dir = normalize(dir);
        pos = pos + dir * speed;
    }
};

inline int findIndex(const std::vector<Character>& v, const Character& obj) {
    for (size_t i = 0; i < v.size(); ++i) {
        if (&v[i] == &obj) {
            return static_cast<int>(i);
        }
    }
    return -1;
}