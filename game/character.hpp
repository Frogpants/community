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

    GLuint texture = 0;

    float cooldown = 0.0;

    float speed = 0.4;

    int room = 0;

    vec2 target;

    std::string name = "Character";

    int level = 0;

    bool isRoaming = false;
    float roamTimer = 0.0;
    vec2 roamCenter = vec2(300.0, 0.0);
    float roamRadius = 500.0;

    std::vector<std::string> tasks = {"wash dishes", "take out trash", "do laundry"}; //Tasks might be changed later!

    int tasksGiven = 0;
    int tasksCompleted = 0;

    bool nextStageSpawned = false;

    std::vector<vec2> positions = {};

    void follow() {
        vec2 dir = target - pos;
        dir = normalize(dir);
        pos = pos + dir * speed;
    }

    void roam() {
        roamTimer -= 1.0;
        
        if (roamTimer <= 0.0) {
            // Pick a new random target within roamRadius of roamCenter
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            float dist = (rand() % 100) / 100.0f * roamRadius;
            target = roamCenter + vec2(cos(angle) * dist, sin(angle) * dist);
            roamTimer = 60.0 + (rand() % 120); // Wait 60-180 frames before picking new target
        }
        
        // Move toward target
        vec2 dir = target - pos;
        float distance = length(dir);
        if (distance > 5.0) {
            dir = normalize(dir);
            pos = pos + dir * speed;
        }
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