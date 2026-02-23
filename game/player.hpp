#pragma once

#include "../core/essentials.hpp"
#include "../core/manager.hpp"
#include "camera.hpp"

struct Player {
    vec2 pos = vec2(0.0);
    vec2 dim = vec2(45.0);
    vec2 vel = vec2(0.0);

    float coins = 0.0;

    float health = 100.0;

    float cooldown = 0.0;

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


        if (std::abs(pos.x) > screen.x + screen.x*0.5) {
            vel.x = 0.0;
            if (pos.x > 0.0) {
                pos.x = screen.x + screen.x*0.5;
            } else {
                pos.x = -screen.x - screen.x*0.5;
            }
        }

        if (std::abs(pos.y) > screen.y + screen.y*0.5) {
            vel.y = 0.0;
            if (pos.y > 0.0) {
                pos.y = screen.y + screen.y*0.5;
            } else {
                pos.y = -screen.y - screen.y*0.5;
            }
        }
    }
};