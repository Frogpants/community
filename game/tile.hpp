#pragma once

#include "../core/essentials.hpp"
#include "../core/manager.hpp"
#include "../core/images/image.hpp"

struct Tile {
    vec2 pos = vec2(0.0);

    int id = 0;

    int layer = 0;

    int room = 0;
};

inline vec2 snap(vec2 p, float n) {
    return floor(p / n) * n;
};

inline int indexOf(const std::vector<Tile>& tiles, const vec2& pos)
{
    for (int i = 0; i < (int)tiles.size(); i++)
    {
        if (tiles[i].pos.x == pos.x && tiles[i].pos.y == pos.y)
        {
            return i;
        }
    }
    return -1;
}