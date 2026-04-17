#pragma once

#include "../core/essentials.hpp"
#include "../core/collision.hpp"
#include "../core/images/image.hpp"
#include "../core/text.hpp"
#include "../core/mouse/mouse.hpp"
#include "task.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iostream>

namespace Minigames {

    enum class WashPhase {
        MovePlates,
        PopBubbles,
        Won,
        Lost
    };

    struct Bubble {
        vec2 pos = vec2(0.0f);
        float radius = 24.0f;
        int lifeFrames = 120;
    };

    struct WashDishesState {
        bool initialized = false;
        GLuint plateTexture = 0;
        GLuint tableTexture = 0;
        GLuint sinkTexture = 0;
        GLuint bubbleTexture = 0;
        int plateCount = 6;

        std::vector<vec2> platePositions;
        std::vector<vec2> plateHomePositions;
        std::vector<bool> plateInSink;

        int draggingPlate = -1;
        vec2 dragOffset = vec2(0.0f);

        WashPhase phase = WashPhase::MovePlates;

        std::vector<Bubble> bubbles;
        int bubbleSpawnTimer = 0;
        int bubbleSpawned = 0;
        int bubblePopped = 0;
        int bubbleMissed = 0;
        int bubbleTarget = 14;

        bool completionSent = false;
    };

    inline bool taskOpen = false;
    inline bool taskCompleteRequested = false;
    inline int activeTaskIndex = -1;
    inline std::string activeTaskName;
    inline WashDishesState washDishes;

    inline bool IsWashDishesTask() {
        return activeTaskName == "wash dishes";
    }

    inline void ResetWashDishesState() {
        washDishes.initialized = false;
        washDishes.draggingPlate = -1;
        washDishes.dragOffset = vec2(0.0f);
        washDishes.phase = WashPhase::MovePlates;
        washDishes.bubbles.clear();
        washDishes.bubbleSpawnTimer = 0;
        washDishes.bubbleSpawned = 0;
        washDishes.bubblePopped = 0;
        washDishes.bubbleMissed = 0;
        washDishes.completionSent = false;
        washDishes.platePositions.clear();
        washDishes.plateHomePositions.clear();
        washDishes.plateInSink.clear();
    }

    inline void InitializeWashDishesAssets() {
        auto loadMiniGameTexture = [](const std::string& relativePath) {
            std::vector<std::string> candidates = {
                "dist/assets/minigames/wash dishes/" + relativePath,
                "assets/minigames/wash dishes/" + relativePath,
                "dist/assets/minigames/wash_dishes/" + relativePath,
                "assets/minigames/wash_dishes/" + relativePath,
                "dist/assets/minigames/wash disehs/" + relativePath,
                "assets/minigames/wash disehs/" + relativePath,
                "dist/assets/mini games/wash dishes/" + relativePath,
                "assets/mini games/wash dishes/" + relativePath
            };

            for (const std::string& path : candidates) {
                GLuint tex = Image::Load(path.c_str());
                if (tex != 0) {
                    return tex;
                }
            }

            static std::vector<std::string> warnedMissing;
            if (std::find(warnedMissing.begin(), warnedMissing.end(), relativePath) == warnedMissing.end()) {
                warnedMissing.push_back(relativePath);
                std::cout << "Failed to load wash dishes texture: " << relativePath << std::endl;
            }
            return 0u;
        };

        if (washDishes.plateTexture == 0) {
            washDishes.plateTexture = loadMiniGameTexture("plate.png");
        }
        if (washDishes.tableTexture == 0) {
            washDishes.tableTexture = loadMiniGameTexture("table.png");
        }
        if (washDishes.sinkTexture == 0) {
            washDishes.sinkTexture = loadMiniGameTexture("sink.png");
        }
        if (washDishes.bubbleTexture == 0) {
            washDishes.bubbleTexture = loadMiniGameTexture("bubble.png");
        }
    }

    inline void BuildPlateSlots(vec2 tableCenter, vec2 tableHalf) {
        washDishes.plateHomePositions.clear();
        washDishes.platePositions.clear();
        washDishes.plateInSink.clear();

        const int cols = 3;
        const float spacingX = 84.0f;
        const float spacingY = 78.0f;
        const float startX = tableCenter.x - spacingX;
        const float startY = tableCenter.y + tableHalf.y - 90.0f;

        for (int i = 0; i < washDishes.plateCount; ++i) {
            int col = i % cols;
            int row = i / cols;
            vec2 slotPos = vec2(startX + static_cast<float>(col) * spacingX, startY - static_cast<float>(row) * spacingY);
            washDishes.plateHomePositions.push_back(slotPos);
            washDishes.platePositions.push_back(slotPos);
            washDishes.plateInSink.push_back(false);
        }

        washDishes.initialized = true;
    }

    inline void DrawPlate(const vec2& pos, float size) {
        if (washDishes.plateTexture != 0) {
            Image::Draw(washDishes.plateTexture, pos, size, 0.0f);
            return;
        }

        Image::DrawRect(pos, vec2(size), 0.92f, 0.92f, 0.95f, 1.0f, 0.0f);
        Image::DrawRect(pos, vec2(size - 7.0f), 0.78f, 0.78f, 0.84f, 1.0f, 0.0f);
    }

    inline void SpawnBubble(vec2 sinkCenter, vec2 sinkHalf) {
        Bubble bubble;
        bubble.radius = static_cast<float>(30 + (std::rand() % 23));
        bubble.lifeFrames = 220 + (std::rand() % 120);

        float minX = sinkCenter.x - sinkHalf.x + bubble.radius + 12.0f;
        float maxX = sinkCenter.x + sinkHalf.x - bubble.radius - 12.0f;
        float minY = sinkCenter.y - sinkHalf.y + bubble.radius + 12.0f;
        float maxY = sinkCenter.y + sinkHalf.y - bubble.radius - 12.0f;

        float rangeX = std::max(1.0f, maxX - minX);
        float rangeY = std::max(1.0f, maxY - minY);
        bubble.pos.x = minX + static_cast<float>(std::rand() % 1000) / 1000.0f * rangeX;
        bubble.pos.y = minY + static_cast<float>(std::rand() % 1000) / 1000.0f * rangeY;

        washDishes.bubbles.push_back(bubble);
        washDishes.bubbleSpawned += 1;
    }

    inline void DrawBubble(const Bubble& bubble) {
        if (washDishes.bubbleTexture != 0) {
            Image::Draw(washDishes.bubbleTexture, bubble.pos, bubble.radius, 0.0f);
            return;
        }

        float lifeRatio = clamp(static_cast<float>(bubble.lifeFrames) / 190.0f, 0.25f, 1.0f);
        float brightness = 0.45f + 0.45f * lifeRatio;
        Image::DrawRect(bubble.pos, vec2(bubble.radius), 0.55f, 0.78f, brightness, 0.72f, 0.0f);
        Image::DrawRect(bubble.pos + vec2(6.0f, 6.0f), vec2(bubble.radius * 0.22f), 0.92f, 0.98f, 1.0f, 0.9f, 0.0f);
    }

    inline void DrawGenericTaskContent(float zoom) {
        const float bodySpacing = 2.2f;
        Text::DrawStringCentered("minigame placeholder", vec2(0.0f, 24.0f), 18.0f / zoom, bodySpacing);
        Text::DrawStringCentered("add task-specific logic later", vec2(0.0f, -18.0f), 14.0f / zoom, bodySpacing);
        Text::DrawStringCentered("press e near a task to open", vec2(0.0f, -60.0f), 14.0f / zoom, bodySpacing);
        Text::DrawStringCentered("click x to exit", vec2(0.0f, -90.0f), 14.0f / zoom, bodySpacing);
    }

    inline void DrawWashDishesContent(vec2 panelHalf, float zoom, vec2 mouseUI) {
        InitializeWashDishesAssets();

        vec2 tableCenter = vec2(-panelHalf.x * 0.50f, -20.0f);
        vec2 sinkCenter = vec2(panelHalf.x * 0.50f, -20.0f);
        vec2 zoneHalf = vec2(panelHalf.x * 0.36f, panelHalf.y * 0.58f);

        if (!washDishes.initialized || static_cast<int>(washDishes.platePositions.size()) != washDishes.plateCount) {
            BuildPlateSlots(tableCenter, zoneHalf);
        }

        if (washDishes.tableTexture != 0) {
            Image::Draw(washDishes.tableTexture, tableCenter, vec2(zoneHalf.x + 36.0f, zoneHalf.y + 16.0f), 0.0f);
        } else {
            Image::DrawRect(tableCenter, zoneHalf, 0.56f, 0.45f, 0.33f, 1.0f, 0.0f);
            Image::DrawRect(tableCenter, vec2(zoneHalf.x, 18.0f), 0.47f, 0.35f, 0.23f, 1.0f, 0.0f);
        }
        Text::DrawStringCentered("table", tableCenter + vec2(0.0f, zoneHalf.y + 24.0f), 14.0f / zoom, 2.1f);

        if (washDishes.sinkTexture != 0) {
            Image::Draw(washDishes.sinkTexture, sinkCenter, vec2(zoneHalf.x + 36.0f, zoneHalf.y + 16.0f), 0.0f);
        } else {
            Image::DrawRect(sinkCenter, zoneHalf, 0.62f, 0.75f, 0.84f, 1.0f, 0.0f);
            Image::DrawRect(sinkCenter, zoneHalf - vec2(18.0f), 0.42f, 0.56f, 0.66f, 1.0f, 0.0f);
        }
        Text::DrawStringCentered("sink", sinkCenter + vec2(0.0f, zoneHalf.y + 24.0f), 14.0f / zoom, 2.1f);

        if (washDishes.phase == WashPhase::MovePlates) {
            const vec2 plateHalf = vec2(28.0f);

            if (washDishes.draggingPlate == -1 && Mouse::IsPressed(0)) {
                for (int i = washDishes.plateCount - 1; i >= 0; --i) {
                    if (washDishes.plateInSink[i]) {
                        continue;
                    }

                    if (BoxCollide(mouseUI, vec2(0.0f), washDishes.platePositions[i], plateHalf)) {
                        washDishes.draggingPlate = i;
                        washDishes.dragOffset = washDishes.platePositions[i] - mouseUI;
                        break;
                    }
                }
            }

            if (washDishes.draggingPlate != -1) {
                int dragged = washDishes.draggingPlate;
                if (Mouse::IsDown(0)) {
                    washDishes.platePositions[dragged] = mouseUI + washDishes.dragOffset;
                } else {
                    bool droppedInSink = BoxCollide(washDishes.platePositions[dragged], plateHalf, sinkCenter, zoneHalf);
                    if (droppedInSink) {
                        washDishes.plateInSink[dragged] = true;
                    } else {
                        washDishes.platePositions[dragged] = washDishes.plateHomePositions[dragged];
                    }
                    washDishes.draggingPlate = -1;
                }
            }

            int sinkIndex = 0;
            for (int i = 0; i < washDishes.plateCount; ++i) {
                if (washDishes.plateInSink[i]) {
                    int col = sinkIndex % 2;
                    int row = sinkIndex / 2;
                    washDishes.platePositions[i] = sinkCenter + vec2(-30.0f + static_cast<float>(col) * 68.0f, 46.0f - static_cast<float>(row) * 62.0f);
                    sinkIndex += 1;
                }
            }

            for (int i = 0; i < washDishes.plateCount; ++i) {
                DrawPlate(washDishes.platePositions[i], 28.0f);
            }

            if (sinkIndex >= washDishes.plateCount) {
                washDishes.phase = WashPhase::PopBubbles;
                washDishes.bubbleSpawnTimer = 20;
                washDishes.bubbles.clear();
            }

            std::string plateGoalText = "drag all " + std::to_string(washDishes.plateCount) + " plates from table to sink";
            Text::DrawStringCentered(plateGoalText, vec2(0.0f, -panelHalf.y + 58.0f), 14.0f / zoom, 2.1f);
        } else if (washDishes.phase == WashPhase::PopBubbles) {
            washDishes.bubbleSpawnTimer -= 1;
            if (washDishes.bubbleSpawnTimer <= 0) {
                SpawnBubble(sinkCenter, zoneHalf - vec2(16.0f));
                washDishes.bubbleSpawnTimer = 28 + (std::rand() % 34);
            }

            if (Mouse::IsPressed(0)) {
                int hitIndex = -1;
                for (int i = static_cast<int>(washDishes.bubbles.size()) - 1; i >= 0; --i) {
                    vec2 delta = mouseUI - washDishes.bubbles[i].pos;
                    float distSq = delta.x * delta.x + delta.y * delta.y;
                    float radius = washDishes.bubbles[i].radius;
                    if (distSq <= radius * radius) {
                        hitIndex = i;
                        break;
                    }
                }

                if (hitIndex != -1) {
                    washDishes.bubbles.erase(washDishes.bubbles.begin() + hitIndex);
                    washDishes.bubblePopped += 1;
                }
            }

            for (Bubble& bubble : washDishes.bubbles) {
                bubble.lifeFrames -= 1;
                bubble.pos.y += 0.04f;
            }

            int before = static_cast<int>(washDishes.bubbles.size());
            washDishes.bubbles.erase(
                std::remove_if(
                    washDishes.bubbles.begin(),
                    washDishes.bubbles.end(),
                    [](const Bubble& bubble) { return bubble.lifeFrames <= 0; }
                ),
                washDishes.bubbles.end()
            );
            int after = static_cast<int>(washDishes.bubbles.size());
            washDishes.bubbleMissed += (before - after);

            for (const Bubble& bubble : washDishes.bubbles) {
                DrawBubble(bubble);
            }

            vec2 finishCenter = vec2(0.0f, -panelHalf.y + 148.0f);
            vec2 finishHalf = vec2(120.0f, 24.0f);
            bool hoveringFinish = BoxCollide(mouseUI, vec2(0.0f), finishCenter, finishHalf);
            Image::DrawRect(finishCenter, finishHalf, hoveringFinish ? 0.22f : 0.30f, hoveringFinish ? 0.70f : 0.62f, 0.28f, 1.0f, 0.0f);
            Text::DrawStringCentered("finish wash", finishCenter - vec2(0.0f, 6.0f), 15.0f / zoom, 2.1f);
            if (hoveringFinish && Mouse::IsPressed(0)) {
                washDishes.phase = WashPhase::Won;
            }

            if (washDishes.bubbleMissed >= 3) {
                washDishes.phase = WashPhase::Lost;
            }

            std::string progressText = "popped " + std::to_string(washDishes.bubblePopped);
            std::string missText = "missed " + std::to_string(washDishes.bubbleMissed) + " of 3";
            Text::DrawStringCentered("click bubbles before they disappear", vec2(0.0f, -panelHalf.y + 48.0f), 14.0f / zoom, 2.1f);
            Text::DrawStringCentered(progressText, vec2(0.0f, -panelHalf.y + 82.0f), 13.0f / zoom, 2.0f);
            Text::DrawStringCentered(missText, vec2(0.0f, -panelHalf.y + 112.0f), 13.0f / zoom, 2.0f);
        } else if (washDishes.phase == WashPhase::Won) {
            Image::DrawRect(vec2(0.0f, -20.0f), vec2(panelHalf.x * 0.70f, 90.0f), 0.76f, 0.90f, 0.78f, 1.0f, 0.0f);
            Text::DrawStringCentered("dishes are clean", vec2(0.0f, 0.0f), 20.0f / zoom, 2.2f);
            Text::DrawStringCentered("task complete", vec2(0.0f, -36.0f), 16.0f / zoom, 2.1f);

            if (!washDishes.completionSent) {
                taskCompleteRequested = true;
                washDishes.completionSent = true;
            }
        } else {
            Image::DrawRect(vec2(0.0f, -20.0f), vec2(panelHalf.x * 0.72f, 96.0f), 0.93f, 0.80f, 0.80f, 1.0f, 0.0f);
            Text::DrawStringCentered("too many bubbles missed", vec2(0.0f, 12.0f), 18.0f / zoom, 2.2f);
            Text::DrawStringCentered("click retry to redo", vec2(0.0f, -22.0f), 14.0f / zoom, 2.1f);

            vec2 retryCenter = vec2(0.0f, -78.0f);
            vec2 retryHalf = vec2(82.0f, 24.0f);
            bool hoveringRetry = BoxCollide(mouseUI, vec2(0.0f), retryCenter, retryHalf);
            Image::DrawRect(retryCenter, retryHalf, hoveringRetry ? 0.18f : 0.25f, hoveringRetry ? 0.58f : 0.50f, 0.84f, 1.0f, 0.0f);
            Text::DrawStringCentered("retry", retryCenter - vec2(0.0f, 6.0f), 16.0f / zoom, 2.2f);

            if (hoveringRetry && Mouse::IsPressed(0)) {
                ResetWashDishesState();
                BuildPlateSlots(tableCenter, zoneHalf);
            }
        }
    }

    inline void OpenTask(int taskIndex, const std::string& taskName) {
        taskOpen = true;
        taskCompleteRequested = false;
        activeTaskIndex = taskIndex;
        activeTaskName = taskName;

        if (IsWashDishesTask()) {
            ResetWashDishesState();
        }
    }

    inline void CloseTask() {
        taskOpen = false;
        taskCompleteRequested = false;
        activeTaskIndex = -1;

        if (IsWashDishesTask()) {
            ResetWashDishesState();
        }

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

        if (IsWashDishesTask()) {
            DrawWashDishesContent(panelHalf, zoom, mouseUI);
        } else {
            DrawGenericTaskContent(zoom);
        }

        if (hoveringClose && Mouse::IsPressed(0)) {
            CloseTask();
            return true;
        }

        return false;
    }

}
