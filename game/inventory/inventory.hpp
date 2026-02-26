#pragma once

#include <vector>

#include "slot.hpp"
#include "../items/item.hpp"

struct Inventory {
    std::vector<Slot> mainSlots;
    std::vector<Slot> invSlots;

    void init() {
        Slot empty;
        empty.section = "main";
        for (int i = 0; i < 9; ++i) {
            empty.id = i;
            mainSlots.push_back(empty);
        }

        empty.section = "inventory";
        for (int i = 0; i < 27; ++i) {
            empty.id = i;
            mainSlots.push_back(empty);
        }
    }

};