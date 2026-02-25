#pragma once

#include "../core/essentials.hpp"
#include "../core/manager.hpp"

#include <cmath>
#include <iostream>

struct Camera {
    vec2 pos = vec2(0.0);
    float zoom = 5.0;

    vec2 vel = vec2(0.0);

    vec2 target;

    void follow() {
        pos = pos + (target - pos) * 0.008;

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

    float speed = 0.3;

    void controls() {
        if (Input::IsDown("w")) {
            vel.y += speed;
        }
        if (Input::IsDown("s")) {
            vel.y -= speed;
        }
        if (Input::IsDown("d")) {
            vel.x += speed;
        }
        if (Input::IsDown("a")) {
            vel.x -= speed;
        }

        vel = vel * 0.7;
        pos = pos + vel;
    }
};