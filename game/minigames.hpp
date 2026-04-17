#pragma once

#include "../core/essentials.hpp"
#include "../core/collision.hpp"
#include "../core/images/image.hpp"
#include "../core/text.hpp"
#include "../core/mouse/mouse.hpp"
#include "task.hpp"

#include <string>

namespace Minigames {

    inline bool taskOpen = false;
    inline bool taskCompleteRequested = false;
    inline int activeTaskIndex = -1;
    inline std::string activeTaskName;

    inline void OpenTask(int taskIndex, const std::string& taskName) {
        taskOpen = true;
        taskCompleteRequested = false;
        activeTaskIndex = taskIndex;
        activeTaskName = taskName;
    }

    inline void CloseTask() {
        taskOpen = false;
        taskCompleteRequested = false;
        activeTaskIndex = -1;
        activeTaskName.clear();
    }

    inline bool IsTaskOpen() {
        return taskOpen;
    }

    inline int GetActiveTaskIndex() {
        return activeTaskIndex;
    }

    inline const std::string& GetActiveTaskName() {
        return activeTaskName;
    }

    inline void RequestTaskComplete() {
        if (taskOpen) {
            taskCompleteRequested = true;
        }
    }

    inline bool ConsumeTaskCompleteRequest() {
        if (!taskCompleteRequested) {
            return false;
        }

        taskCompleteRequested = false;
        return true;
    }

    inline bool DrawTaskPopup(vec2 screen, float zoom, vec2 mouseUI) {
        if (!taskOpen) {
            return false;
        }

        const float titleSpacing = 2.4f;
        const float bodySpacing = 2.2f;

        vec2 fullSize = screen / zoom;
        vec2 overlayHalf = fullSize;
        Image::DrawRect(vec2(0.0f, 0.0f), overlayHalf, 0.0f, 0.0f, 0.0f, 0.65f, 0.0f);

        vec2 panelHalf = vec2(fullSize.x * 0.56f, fullSize.y * 0.50f);
        Image::DrawRect(vec2(0.0f, 0.0f), panelHalf + vec2(12.0f), 0.12f, 0.12f, 0.14f, 1.0f, 0.0f);
        Image::DrawRect(vec2(0.0f, 0.0f), panelHalf, 0.96f, 0.96f, 0.98f, 1.0f, 0.0f);

        vec2 headerHalf = vec2(panelHalf.x - 14.0f, 34.0f);
        vec2 headerPos = vec2(0.0f, panelHalf.y - 46.0f);
        Image::DrawRect(headerPos, headerHalf, 0.18f, 0.18f, 0.22f, 1.0f, 0.0f);

        vec2 closeCenter = vec2(panelHalf.x - 30.0f, panelHalf.y - 30.0f);
        vec2 closeHalf = vec2(18.0f);
        bool hoveringClose = BoxCollide(mouseUI, vec2(0.0f), closeCenter, closeHalf);
        Image::DrawRect(closeCenter, closeHalf, hoveringClose ? 0.88f : 0.74f, 0.20f, 0.20f, 1.0f, 0.0f);
        Text::DrawStringCentered("x", closeCenter - vec2(0.0f, 5.0f), 18.0f / zoom, titleSpacing);

        Text::DrawStringCentered(activeTaskName, vec2(0.0f, panelHalf.y - 56.0f), 24.0f / zoom, titleSpacing);
        Text::DrawStringCentered("minigame placeholder", vec2(0.0f, 24.0f), 18.0f / zoom, bodySpacing);
        Text::DrawStringCentered("add task-specific logic later", vec2(0.0f, -18.0f), 14.0f / zoom, bodySpacing);
        Text::DrawStringCentered("press e near a task to open", vec2(0.0f, -60.0f), 14.0f / zoom, bodySpacing);
        Text::DrawStringCentered("click x to exit", vec2(0.0f, -90.0f), 14.0f / zoom, bodySpacing);

        if (hoveringClose && Mouse::IsPressed(0)) {
            CloseTask();
            return true;
        }

        return false;
    }

}
