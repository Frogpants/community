#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../images/image.hpp"
#include "../text.hpp"
#include "button.hpp"

struct UiImage {
    std::string id;
    GLuint texture = 0;
    vec2 pos = vec2(0.0f);
    vec2 dim = vec2(64.0f);
    std::function<vec2()> dynamicPos;
    std::function<vec2()> dynamicDim;
    bool visible = true;

    void applyDynamic() {
        if (dynamicPos) {
            pos = dynamicPos();
        }
        if (dynamicDim) {
            dim = dynamicDim();
        }
    }

    void draw() const {
        if (!visible || texture == 0) {
            return;
        }
        Image::Draw(texture, pos, dim);
    }
};

struct UiLabel {
    std::string id;
    std::string text;
    vec2 pos = vec2(0.0f);
    std::function<vec2()> dynamicPos;
    float size = 14.0f;
    float spacing = 1.4f;
    bool centered = false;
    bool visible = true;

    void applyDynamic() {
        if (dynamicPos) {
            pos = dynamicPos();
        }
    }

    void draw() const {
        if (!visible || text.empty()) {
            return;
        }

        if (centered) {
            Text::DrawStringCentered(text, pos, size, spacing);
        } else {
            Text::DrawString(text, pos, size, spacing);
        }
    }
};

struct UiPanel {
    std::string id;
    vec2 pos = vec2(0.0f);
    vec2 dim = vec2(100.0f, 60.0f);
    std::function<vec2()> dynamicPos;
    std::function<vec2()> dynamicDim;
    vec4 color = vec4(0.0f, 0.0f, 0.0f, 0.5f);
    bool visible = true;

    void applyDynamic() {
        if (dynamicPos) {
            pos = dynamicPos();
        }
        if (dynamicDim) {
            dim = dynamicDim();
        }
    }

    void draw() const {
        if (!visible) {
            return;
        }
        Image::DrawRect(pos, dim, color.x, color.y, color.z, color.w, 0.0f);
    }
};

struct Menu {
    std::string id;
    bool visible = true;
    bool enabled = true;

    GLuint bg = 0;
    vec2 bgPos = vec2(0.0f);
    vec2 bgDim = vec2(320.0f, 180.0f);

    std::vector<Button> buttons;
    std::vector<UiImage> images;
    std::vector<UiLabel> labels;
    std::vector<UiPanel> panels;

    void update(vec2 mousePos) {
        if (!visible || !enabled) {
            return;
        }

        for (UiPanel& panel : panels) {
            panel.applyDynamic();
        }

        for (UiImage& image : images) {
            image.applyDynamic();
        }

        for (UiLabel& label : labels) {
            label.applyDynamic();
        }

        for (Button& button : buttons) {
            button.update(mousePos);
        }
    }

    void draw() const {
        if (!visible) {
            return;
        }

        if (bg != 0) {
            Image::Draw(bg, bgPos, bgDim);
        }

        for (const UiPanel& panel : panels) {
            panel.draw();
        }

        for (const UiImage& image : images) {
            image.draw();
        }

        for (const Button& button : buttons) {
            button.draw();
        }

        for (const UiLabel& label : labels) {
            label.draw();
        }
    }
};