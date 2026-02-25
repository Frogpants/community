#pragma once

#include "../core/essentials.hpp"
#include "../core/manager.hpp"
#include "../core/images/image.hpp"

struct Tile {
    vec2 pos = vec2(0.0);

    GLuint texture = Image::Load("../assets/null.png");

    int layer = 0;
};

inline vec2 snap(vec2 p, float n) {
    return floor(p / n) * n;
};