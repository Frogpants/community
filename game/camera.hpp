#pragma once

#include "../core/essentials.hpp"
#include "../core/manager.hpp"

#include <cmath>
#include <iostream>

struct Camera {
    vec2 pos = vec2(0.0);
    float zoom = 5.0;

    vec2 target;

    void follow() {
        pos = pos + (target - pos) * 0.1;

        if (std::abs(pos.x) > screen.x) {
            if (pos.x > 0.0) {
                pos.x = screen.x;
            } else {
                pos.x = -screen.x;
            }
        }

        if (std::abs(pos.y) > screen.y) {
            if (pos.y > 0.0) {
                pos.y = screen.y;
            } else {
                pos.y = -screen.y;
            }
        }
    }
};