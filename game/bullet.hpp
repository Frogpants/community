#pragma once

#include "../core/essentials.hpp"

struct Bullet {
    vec2 pos = vec2(0.0);
    vec2 dim = vec2(10.0);
    float dir = 0.0;

    float speed = 4.0;

    void move() {
        pos = pos + vec2(cos(dir), sin(dir)) * speed;
    }
};

inline int findIndex(const std::vector<Bullet>& v, const Bullet& obj) {
    for (size_t i = 0; i < v.size(); ++i) {
        if (&v[i] == &obj) {
            return static_cast<int>(i);
        }
    }
    return -1;
}