#pragma once

#include <iostream>

#include "../essentials.hpp"
#include "../items/item.hpp"

struct Slot {
    Item item;

    int id;

    std::string section;

    bool locked = false;
};