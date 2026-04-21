#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>

#include "menu.hpp"
#include "../mouse/mouse.hpp"

namespace UI {

    inline GLFWwindow* gWindow = nullptr;
    inline vec2 mousePos = vec2(0.0f);
    inline std::vector<Menu> menus;

    inline vec2 ToCenteredUiSpace(GLFWwindow* window) {
        if (window == nullptr) {
            return vec2(0.0f);
        }

        int width = 0;
        int height = 0;
        glfwGetWindowSize(window, &width, &height);

        float x = static_cast<float>(Mouse::X()) - static_cast<float>(width) * 0.5f;
        float y = static_cast<float>(height) * 0.5f - static_cast<float>(Mouse::Y());
        return vec2(x, y);
    }

    inline void Init(GLFWwindow* window) {
        gWindow = window;
        mousePos = ToCenteredUiSpace(gWindow);
        menus.clear();
    }

    inline void Update() {
        mousePos = ToCenteredUiSpace(gWindow);
        for (Menu& menu : menus) {
            menu.update(mousePos);
        }
    }

    inline void Draw() {
        for (const Menu& menu : menus) {
            menu.draw();
        }
    }

    inline void Clear() {
        menus.clear();
    }

    inline bool RemoveMenu(const std::string& id) {
        auto oldSize = menus.size();
        menus.erase(
            std::remove_if(
                menus.begin(),
                menus.end(),
                [&id](const Menu& menu) {
                    return menu.id == id;
                }
            ),
            menus.end()
        );
        return menus.size() != oldSize;
    }

    inline Menu& CreateMenu(const std::string& id) {
        Menu menu;
        menu.id = id;
        menus.push_back(menu);
        return menus.back();
    }

    inline Menu* FindMenu(const std::string& id) {
        for (Menu& menu : menus) {
            if (menu.id == id) {
                return &menu;
            }
        }
        return nullptr;
    }

    inline Button& AddButton(Menu& menu,
                             const std::string& id,
                             const std::string& label,
                             vec2 pos,
                             vec2 dim,
                             GLuint texture = 0) {
        Button button;
        button.id = id;
        button.label = label;
        button.pos = pos;
        button.dim = dim;
        button.bg = texture;
        menu.buttons.push_back(button);
        return menu.buttons.back();
    }

    inline UiImage& AddImage(Menu& menu,
                             const std::string& id,
                             GLuint texture,
                             vec2 pos,
                             vec2 dim) {
        UiImage image;
        image.id = id;
        image.texture = texture;
        image.pos = pos;
        image.dim = dim;
        menu.images.push_back(image);
        return menu.images.back();
    }

    inline UiLabel& AddLabel(Menu& menu,
                             const std::string& id,
                             const std::string& text,
                             vec2 pos,
                             float size,
                             bool centered = false) {
        UiLabel label;
        label.id = id;
        label.text = text;
        label.pos = pos;
        label.size = size;
        label.centered = centered;
        menu.labels.push_back(label);
        return menu.labels.back();
    }

    inline UiPanel& AddPanel(Menu& menu,
                             const std::string& id,
                             vec2 pos,
                             vec2 dim,
                             vec4 color = vec4(0.0f, 0.0f, 0.0f, 0.5f)) {
        UiPanel panel;
        panel.id = id;
        panel.pos = pos;
        panel.dim = dim;
        panel.color = color;
        menu.panels.push_back(panel);
        return menu.panels.back();
    }
}
