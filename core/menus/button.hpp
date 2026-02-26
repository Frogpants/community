#pragma once

#include "../images/image.hpp"
#include "../mouse/mouse.hpp"
#include "../essentials.hpp"
#include "../collision.hpp"

struct Button {
    GLuint bg;

    vec2 pos;
    vec2 dim;

    bool touching = false;
    bool clicked = false;

    void update(vec2 mouse) {
        touching = false;
        clicked = false;
        
        if (BoxCollide(pos, dim, mouse, vec2(0.0))) {
            touching = true;
            if (Mouse::IsPressed(0)) {
                clicked = true;
            }
        }
    }
};