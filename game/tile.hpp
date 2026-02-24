#pragma once

#include "../core/essentials.hpp"
#include "../core/images/image.hpp"

struct Tile {
    vec2 pos = vec2(0.0);

    GLuint enemyTex = Image::Load("assets/null.png");

    int layer = 0;
};

inline vec2 snap(vec2 p, float n) {
    return floor(p / n) * n;
};