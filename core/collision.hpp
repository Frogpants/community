#pragma once

#include "essentials.hpp"

inline bool BoxCollide(vec2 obj1, vec2 dim1, vec2 obj2, vec2 dim2) {
    vec2 dif = obj2 - obj1;
    vec2 sum = dim1 + dim2;
    if (abs(dif.x) < sum.x) {
        if (abs(dif.y) < sum.y) {
            return true;
        }
    }
    return false;
}

inline bool BallCollide(vec2 obj1, vec2 dim1, vec2 obj2, vec2 dim2) {
    float dist = length(obj2 - obj1);
    float r = length(dim1 + dim2);
    if (dist < r) {
        return true;
    }
    return false;
}

inline bool BallCollide(vec2 obj1, float r1, vec2 obj2, float r2) {
    float dist = length(obj2 - obj1);
    float r = r1 + r2;
    if (dist < r) {
        return true;
    }
    return false;
}