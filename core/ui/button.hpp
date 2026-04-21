#pragma once

#include <functional>
#include <string>

#include "../images/image.hpp"
#include "../text.hpp"
#include "../mouse/mouse.hpp"
#include "../essentials.hpp"
#include "../collision.hpp"

struct Button {
    std::string id;
    std::string label;

    GLuint bg = 0;
    GLuint bgHover = 0;
    GLuint bgPressed = 0;

    vec2 pos = vec2(0.0f);
    vec2 dim = vec2(64.0f, 24.0f);
    std::function<vec2()> dynamicPos;
    std::function<vec2()> dynamicDim;

    vec4 fallbackColor = vec4(0.2f, 0.2f, 0.24f, 0.9f);
    vec4 fallbackHoverColor = vec4(0.3f, 0.3f, 0.36f, 0.95f);
    vec4 fallbackPressedColor = vec4(0.15f, 0.15f, 0.2f, 1.0f);

    float labelSize = 14.0f;
    float labelSpacing = 1.4f;

    bool visible = true;
    bool enabled = true;

    bool touching = false;
    bool held = false;
    bool clicked = false;

    std::function<void()> onClick;

    bool contains(vec2 point) const {
        return BoxCollide(pos, dim, point, vec2(0.0f));
    }

    void applyDynamic() {
        if (dynamicPos) {
            pos = dynamicPos();
        }
        if (dynamicDim) {
            dim = dynamicDim();
        }
    }

    void update(vec2 mouse) {
        touching = false;
        held = false;
        clicked = false;

        if (!visible || !enabled) {
            return;
        }

        applyDynamic();
        
        if (contains(mouse)) {
            touching = true;
            held = Mouse::IsDown(0);
            if (Mouse::IsPressed(0)) {
                clicked = true;
                if (onClick) {
                    onClick();
                }
            }
        }
    }

    void draw() const {
        if (!visible) {
            return;
        }

        auto drawRectRgba = static_cast<void(*)(vec2, vec2, float, float, float, float, float)>(&Image::DrawRect);

        GLuint texture = bg;
        if (held && bgPressed != 0) {
            texture = bgPressed;
        } else if (touching && bgHover != 0) {
            texture = bgHover;
        }

        if (texture != 0) {
            Image::Draw(texture, pos, dim);
        } else if (held) {
            drawRectRgba(pos, dim, fallbackPressedColor.x, fallbackPressedColor.y, fallbackPressedColor.z, fallbackPressedColor.w, 0.0f);
        } else if (touching) {
            drawRectRgba(pos, dim, fallbackHoverColor.x, fallbackHoverColor.y, fallbackHoverColor.z, fallbackHoverColor.w, 0.0f);
        } else {
            drawRectRgba(pos, dim, fallbackColor.x, fallbackColor.y, fallbackColor.z, fallbackColor.w, 0.0f);
        }

        if (!label.empty()) {
            Text::DrawStringCentered(label, pos - vec2(dim.x * 0.4f, labelSize * 0.3f), labelSize, labelSpacing);
        }
    }
};