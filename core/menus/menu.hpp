#pragma once

#include <vector>

#include "../images/image.hpp"
#include "button.hpp"

struct Menu {
    GLuint bg;

    std::vector<Button> buttons;
};