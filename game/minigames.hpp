#pragma once

#include "../core/essentials.hpp"
#include "../core/collision.hpp"
#include "../core/images/image.hpp"
#include "../core/text.hpp"
#include "../core/mouse/mouse.hpp"
#include "../core/ui/ui.hpp"
#include "task.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <numeric>

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
        int bubbleTarget = 10;

        bool completionSent = false;
    };

    enum class TrashPhase {
        CollectFloorTrash,
        GoOutsidePrompt
    };

    struct TrashItem {
        vec2 pos = vec2(0.0f);
        vec2 home = vec2(0.0f);
        vec2 size = vec2(24.0f);
        bool inBin = false;
        int textureIndex = 0;
    };

    struct TakeOutTrashState {
        bool initialized = false;
        GLuint binTexture = 0;
        GLuint outsideTrashcanTexture = 0;
        std::vector<GLuint> trashTextures;
        int activeTrashCount = 4;

        std::vector<TrashItem> items;
        int draggingItem = -1;
        vec2 dragOffset = vec2(0.0f);

        TrashPhase phase = TrashPhase::CollectFloorTrash;
    };

    struct PendingTrashDropoff {
        std::string taskName;
        int taskRoom = -1;
        vec2 worldPos = vec2(0.0f);
        bool hasWorldPos = false;
        bool active = true;
        bool readyForInteract = false;
    };

    struct LaundryState {
        bool initialized = false;
        GLuint outsideMachineTexture = 0;
        GLuint insideMachineTexture = 0;
        float washProgress = 0.0f;
        bool started = false;
        bool completionSent = false;
    };

    inline bool taskOpen = false;
    inline bool taskCompleteRequested = false;
    inline int activeTaskIndex = -1;
    inline int activeTaskRoom = -1;
    inline std::string activeTaskName;
    inline int completionTaskIndex = -1;
    inline int completionTaskRoom = -1;
    inline std::string completionTaskName;
    inline WashDishesState washDishes;
    inline TakeOutTrashState takeOutTrash;
    inline LaundryState laundry;
    inline std::vector<PendingTrashDropoff> pendingTrashDropoffs;
    inline bool takeOutTrashDropoffPlacementRequested = false;
    inline int takeOutTrashDropoffRequestedRoom = -1;
    inline std::string takeOutTrashDropoffRequestedTaskName;

    inline bool taskPopupUiInitialized = false;
    inline bool taskPopupCloseRequested = false;
    inline vec2 taskPopupFullSize = vec2(0.0f);
    inline vec2 taskPopupPanelHalf = vec2(0.0f);
    inline float taskPopupZoom = 1.0f;
    inline std::string taskPopupTitle;
    inline Menu* taskPopupControlsMenu = nullptr;
    inline int taskCompletionDisplayFrames = 0;

    inline Menu* EnsureTaskPopupUiMenu() {
        Menu* menu = UI::FindMenu("minigame-task-popup");
        if (menu != nullptr && taskPopupUiInitialized) {
            return menu;
        }

        if (menu == nullptr) {
            menu = &UI::CreateMenu("minigame-task-popup");
        }

        menu->panels.clear();
        menu->images.clear();
        menu->labels.clear();
        menu->buttons.clear();

        UiPanel& overlay = UI::AddPanel(*menu, "overlay", vec2(0.0f), vec2(10.0f), vec4(0.0f, 0.0f, 0.0f, 0.65f));
        overlay.dynamicDim = []() {
            return taskPopupFullSize;
        };

        UiPanel& panelShadow = UI::AddPanel(*menu, "panel-shadow", vec2(0.0f), vec2(100.0f), vec4(0.12f, 0.12f, 0.14f, 1.0f));
        panelShadow.dynamicDim = []() {
            return taskPopupPanelHalf + vec2(12.0f);
        };

        UiPanel& panelBody = UI::AddPanel(*menu, "panel-body", vec2(0.0f), vec2(100.0f), vec4(0.96f, 0.96f, 0.98f, 1.0f));
        panelBody.dynamicDim = []() {
            return taskPopupPanelHalf;
        };

        UiPanel& panelHeader = UI::AddPanel(*menu, "panel-header", vec2(0.0f), vec2(100.0f), vec4(0.18f, 0.18f, 0.22f, 1.0f));
        panelHeader.dynamicPos = []() {
            return vec2(0.0f, taskPopupPanelHalf.y - 46.0f);
        };
        panelHeader.dynamicDim = []() {
            return vec2(taskPopupPanelHalf.x - 14.0f, 34.0f);
        };

        UiLabel& title = UI::AddLabel(*menu, "panel-title", "", vec2(0.0f), 24.0f, true);
        title.dynamicPos = []() {
            return vec2(0.0f, taskPopupPanelHalf.y - 56.0f);
        };

        Button& close = UI::AddButton(*menu, "panel-close", "x", vec2(0.0f), vec2(18.0f), 0);
        close.labelSize = 18.0f;
        close.labelSpacing = 2.4f;
        close.fallbackColor = vec4(0.74f, 0.20f, 0.20f, 1.0f);
        close.fallbackHoverColor = vec4(0.88f, 0.20f, 0.20f, 1.0f);
        close.fallbackPressedColor = vec4(0.62f, 0.14f, 0.14f, 1.0f);
        close.dynamicPos = []() {
            return vec2(taskPopupPanelHalf.x - 30.0f, taskPopupPanelHalf.y - 30.0f);
        };
        close.onClick = []() {
            taskPopupCloseRequested = true;
        };

        taskPopupUiInitialized = true;
        return menu;
    }

    inline bool IsWashDishesTask() {
        return activeTaskName == "wash dishes";
    }

    inline bool IsTakeOutTrashTask() {
        return activeTaskName == "take out trash";
    }

    inline bool IsLaundryTask() {
        return activeTaskName == "do laundry";
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

    inline void ResetTakeOutTrashState() {
        takeOutTrash.initialized = false;
        takeOutTrash.draggingItem = -1;
        takeOutTrash.dragOffset = vec2(0.0f);
        takeOutTrash.phase = TrashPhase::CollectFloorTrash;
        takeOutTrash.items.clear();
    }

    inline void ResetLaundryState() {
        laundry.initialized = false;
        laundry.washProgress = 0.0f;
        laundry.started = false;
        laundry.completionSent = false;
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
                GLuint tex = Image::Load(path.c_str(), false);
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

    inline void InitializeTakeOutTrashAssets() {
        auto loadTrashTexture = [](const std::string& relativePath) {
            std::vector<std::string> candidates = {
                "dist/assets/minigames/take out trash/" + relativePath,
                "assets/minigames/take out trash/" + relativePath,
                "dist/assets/minigames/take_out_trash/" + relativePath,
                "assets/minigames/take_out_trash/" + relativePath
            };

            for (const std::string& path : candidates) {
                GLuint tex = Image::Load(path.c_str(), false);
                if (tex != 0) {
                    return tex;
                }
            }

            return 0u;
        };

        if (takeOutTrash.binTexture == 0) {
            takeOutTrash.binTexture = loadTrashTexture("insideTrash.png");
            if (takeOutTrash.binTexture == 0) {
                takeOutTrash.binTexture = loadTrashTexture("bin.png");
            }
        }

        if (takeOutTrash.outsideTrashcanTexture == 0) {
            takeOutTrash.outsideTrashcanTexture = loadTrashTexture("outdoorTrash.png");
            if (takeOutTrash.outsideTrashcanTexture == 0) {
                takeOutTrash.outsideTrashcanTexture = loadTrashTexture("outside_trashcan.png");
            }
        }

        if (takeOutTrash.trashTextures.empty()) {
            for (int index = 1; index <= 40; ++index) {
                GLuint tex = loadTrashTexture("trash" + std::to_string(index) + ".png");
                if (tex != 0) {
                    takeOutTrash.trashTextures.push_back(tex);
                }
            }

            if (takeOutTrash.trashTextures.empty()) {
                std::vector<std::string> fallbackNames = {
                    "trash_item_placeholder_1.png",
                    "trash_item_placeholder_2.png",
                    "trash_item_placeholder_3.png"
                };

                for (const std::string& name : fallbackNames) {
                    GLuint tex = loadTrashTexture(name);
                    if (tex != 0) {
                        takeOutTrash.trashTextures.push_back(tex);
                    }
                }
            }
        }
    }

    inline void InitializeLaundryAssets() {
        auto loadLaundryTexture = [](const std::string& relativePath) {
            std::vector<std::string> candidates = {
                "dist/assets/minigames/laundry/" + relativePath,
                "assets/minigames/laundry/" + relativePath,
                "dist/assets/mini games/laundry/" + relativePath,
                "assets/mini games/laundry/" + relativePath
            };

            for (const std::string& path : candidates) {
                GLuint tex = Image::Load(path.c_str(), false);
                if (tex != 0) {
                    return tex;
                }
            }

            return 0u;
        };

        if (laundry.outsideMachineTexture == 0) {
            laundry.outsideMachineTexture = loadLaundryTexture("outsideMachine.png");
        }
        if (laundry.insideMachineTexture == 0) {
            laundry.insideMachineTexture = loadLaundryTexture("insideMachine.png");
        }

        laundry.initialized = true;
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

    inline void BuildTrashItems(vec2 areaCenter, vec2 areaHalf, vec2 binCenter, vec2 binHalf) {
        takeOutTrash.items.clear();

        std::vector<int> textureIndices;
        textureIndices.reserve(takeOutTrash.trashTextures.size());
        for (int index = 0; index < static_cast<int>(takeOutTrash.trashTextures.size()); ++index) {
            textureIndices.push_back(index);
        }

        for (int index = static_cast<int>(textureIndices.size()) - 1; index > 0; --index) {
            int swapIndex = std::rand() % (index + 1);
            std::swap(textureIndices[index], textureIndices[swapIndex]);
        }

        const int totalItems = takeOutTrash.activeTrashCount;
        const float minX = areaCenter.x - areaHalf.x + 42.0f;
        const float maxX = areaCenter.x + areaHalf.x - 42.0f;
        const float minY = areaCenter.y - areaHalf.y + 38.0f;
        const float maxY = areaCenter.y + areaHalf.y - 52.0f;

        for (int itemIndex = 0; itemIndex < totalItems; ++itemIndex) {
            TrashItem item;
            item.size = vec2(24.0f + static_cast<float>((itemIndex % 2) * 4));
            item.inBin = false;

            vec2 chosenPos = areaCenter;
            bool foundSpot = false;
            for (int attempt = 0; attempt < 80; ++attempt) {
                float x = minX + static_cast<float>(std::rand() % 1000) / 1000.0f * std::max(1.0f, maxX - minX);
                float y = minY + static_cast<float>(std::rand() % 1000) / 1000.0f * std::max(1.0f, maxY - minY);
                vec2 candidate = vec2(x, y);

                if (BoxCollide(candidate, item.size + vec2(34.0f), binCenter, binHalf + vec2(52.0f))) {
                    continue;
                }

                bool overlapping = false;
                for (const TrashItem& existing : takeOutTrash.items) {
                    if (BoxCollide(candidate, item.size + vec2(16.0f), existing.home, existing.size + vec2(16.0f))) {
                        overlapping = true;
                        break;
                    }
                }

                if (overlapping) {
                    continue;
                }

                chosenPos = candidate;
                foundSpot = true;
                break;
            }

            if (!foundSpot) {
                chosenPos = vec2(
                    minX + static_cast<float>(std::rand() % 1000) / 1000.0f * std::max(1.0f, maxX - minX),
                    minY + static_cast<float>(std::rand() % 1000) / 1000.0f * std::max(1.0f, maxY - minY)
                );
            }

            item.pos = chosenPos;
            item.home = chosenPos;

            if (!textureIndices.empty()) {
                item.textureIndex = textureIndices[itemIndex % static_cast<int>(textureIndices.size())];
            } else {
                item.textureIndex = -1;
            }

            takeOutTrash.items.push_back(item);
        }

        takeOutTrash.initialized = true;
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

    inline void DrawTrashItem(const TrashItem& item) {
        if (item.textureIndex >= 0 && item.textureIndex < static_cast<int>(takeOutTrash.trashTextures.size())) {
            GLuint tex = takeOutTrash.trashTextures[item.textureIndex];
            if (tex != 0) {
                Image::Draw(tex, item.pos, item.size.x, 0.0f);
                return;
            }
        }

        Image::DrawRect(item.pos, item.size, 0.45f, 0.47f, 0.42f, 1.0f, 0.0f);
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

        if (washDishes.plateCount <= 0) {
            washDishes.plateCount = 1;
        }

        if (!washDishes.initialized ||
            static_cast<int>(washDishes.platePositions.size()) != washDishes.plateCount ||
            static_cast<int>(washDishes.plateHomePositions.size()) != washDishes.plateCount ||
            static_cast<int>(washDishes.plateInSink.size()) != washDishes.plateCount) {
            BuildPlateSlots(tableCenter, zoneHalf);
        }

        const int plateSlots = std::min(
            washDishes.plateCount,
            static_cast<int>(std::min(washDishes.platePositions.size(), std::min(washDishes.plateHomePositions.size(), washDishes.plateInSink.size())))
        );

        if (plateSlots <= 0) {
            BuildPlateSlots(tableCenter, zoneHalf);
            return;
        }

        if (washDishes.draggingPlate < -1 || washDishes.draggingPlate >= plateSlots) {
            washDishes.draggingPlate = -1;
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
                for (int i = plateSlots - 1; i >= 0; --i) {
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
            for (int i = 0; i < plateSlots; ++i) {
                if (washDishes.plateInSink[i]) {
                    int col = sinkIndex % 2;
                    int row = sinkIndex / 2;
                    washDishes.platePositions[i] = sinkCenter + vec2(-30.0f + static_cast<float>(col) * 68.0f, 46.0f - static_cast<float>(row) * 62.0f);
                    sinkIndex += 1;
                }
            }

            for (int i = 0; i < plateSlots; ++i) {
                DrawPlate(washDishes.platePositions[i], 28.0f);
            }

            if (sinkIndex >= plateSlots) {
                washDishes.phase = WashPhase::PopBubbles;
                washDishes.bubbleSpawnTimer = 20;
                washDishes.bubbles.clear();
            }

            std::string plateGoalText = "drag all " + std::to_string(plateSlots) + " plates from table to sink";
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
                    if (washDishes.bubblePopped >= washDishes.bubbleTarget) {
                        washDishes.phase = WashPhase::Won;
                    }
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

            if (washDishes.phase == WashPhase::PopBubbles && washDishes.bubbleMissed >= 3) {
                washDishes.phase = WashPhase::Lost;
            }

            std::string progressText = "popped " + std::to_string(washDishes.bubblePopped) + " / " + std::to_string(washDishes.bubbleTarget);
            std::string missText = "missed " + std::to_string(washDishes.bubbleMissed) + " of 3";
            Text::DrawStringCentered("pop 10 bubbles before they disappear", vec2(0.0f, -panelHalf.y + 48.0f), 14.0f / zoom, 2.1f);
            Text::DrawStringCentered(progressText, vec2(0.0f, -panelHalf.y + 82.0f), 13.0f / zoom, 2.0f);
            Text::DrawStringCentered(missText, vec2(0.0f, -panelHalf.y + 112.0f), 13.0f / zoom, 2.0f);
        } else if (washDishes.phase == WashPhase::Won) {
            Image::DrawRect(vec2(0.0f, -20.0f), vec2(panelHalf.x * 0.70f, 90.0f), 0.76f, 0.90f, 0.78f, 1.0f, 0.0f);
            Text::DrawStringCentered("dishes are clean", vec2(0.0f, 0.0f), 20.0f / zoom, 2.2f);
            Text::DrawStringCentered("task complete", vec2(0.0f, -36.0f), 16.0f / zoom, 2.1f);

            if (!washDishes.completionSent) {
                completionTaskIndex = activeTaskIndex;
                completionTaskName = activeTaskName;
                taskCompleteRequested = true;
                washDishes.completionSent = true;
            }
        } else {
            Image::DrawRect(vec2(0.0f, -20.0f), vec2(panelHalf.x * 0.72f, 96.0f), 0.93f, 0.80f, 0.80f, 1.0f, 0.0f);
            Text::DrawStringCentered("too many bubbles missed", vec2(0.0f, 12.0f), 18.0f / zoom, 2.2f);
            Text::DrawStringCentered("click retry to redo", vec2(0.0f, -22.0f), 14.0f / zoom, 2.1f);

            vec2 retryCenter = vec2(0.0f, -78.0f);
            vec2 retryHalf = vec2(82.0f, 24.0f);
            if (taskPopupControlsMenu != nullptr) {
                Button& retry = UI::AddButton(*taskPopupControlsMenu, "wash-retry", "retry", retryCenter, retryHalf, 0);
                retry.labelSize = 16.0f / zoom;
                retry.labelSpacing = 2.2f;
                retry.fallbackColor = vec4(0.25f, 0.50f, 0.84f, 1.0f);
                retry.fallbackHoverColor = vec4(0.18f, 0.58f, 0.84f, 1.0f);
                retry.fallbackPressedColor = vec4(0.14f, 0.44f, 0.66f, 1.0f);
                retry.onClick = [tableCenter, zoneHalf]() {
                    ResetWashDishesState();
                    BuildPlateSlots(tableCenter, zoneHalf);
                };
            }
        }
    }

    inline void DrawTakeOutTrashContent(vec2 panelHalf, float zoom, vec2 mouseUI) {
        InitializeTakeOutTrashAssets();

        vec2 scatterCenter = vec2(0.0f, -10.0f);
        vec2 scatterHalf = vec2(panelHalf.x * 0.84f, panelHalf.y * 0.62f);
        vec2 binCenter = vec2(0.0f, -20.0f);
        vec2 binHalf = vec2(72.0f, 88.0f);

        if (!takeOutTrash.initialized) {
            BuildTrashItems(scatterCenter, scatterHalf, binCenter, binHalf);
        }

        if (takeOutTrash.phase == TrashPhase::CollectFloorTrash) {
            if (takeOutTrash.binTexture != 0) {
                Image::Draw(takeOutTrash.binTexture, binCenter, vec2(binHalf.x, binHalf.y), 0.0f);
            } else {
                Image::DrawRect(binCenter, binHalf, 0.22f, 0.28f, 0.22f, 1.0f, 0.0f);
                Image::DrawRect(binCenter + vec2(0.0f, binHalf.y + 8.0f), vec2(binHalf.x + 10.0f, 10.0f), 0.14f, 0.20f, 0.14f, 1.0f, 0.0f);
            }
            Text::DrawStringCentered("bin", binCenter + vec2(0.0f, binHalf.y + 30.0f), 14.0f / zoom, 2.1f);

            if (takeOutTrash.draggingItem == -1 && Mouse::IsPressed(0)) {
                for (int i = static_cast<int>(takeOutTrash.items.size()) - 1; i >= 0; --i) {
                    if (takeOutTrash.items[i].inBin) {
                        continue;
                    }

                    if (BoxCollide(mouseUI, vec2(0.0f), takeOutTrash.items[i].pos, takeOutTrash.items[i].size)) {
                        takeOutTrash.draggingItem = i;
                        takeOutTrash.dragOffset = takeOutTrash.items[i].pos - mouseUI;
                        break;
                    }
                }
            }

            if (takeOutTrash.draggingItem != -1) {
                TrashItem& dragged = takeOutTrash.items[takeOutTrash.draggingItem];
                if (Mouse::IsDown(0)) {
                    dragged.pos = mouseUI + takeOutTrash.dragOffset;
                } else {
                    bool droppedInBin = BoxCollide(dragged.pos, dragged.size, binCenter, binHalf);
                    if (droppedInBin) {
                        dragged.inBin = true;
                    } else {
                        dragged.pos = dragged.home;
                    }
                    takeOutTrash.draggingItem = -1;
                }
            }

            int placedCount = 0;
            for (TrashItem& item : takeOutTrash.items) {
                if (item.inBin) {
                    int row = placedCount / 3;
                    int col = placedCount % 3;
                    item.pos = binCenter + vec2(-26.0f + static_cast<float>(col) * 26.0f, 28.0f - static_cast<float>(row) * 26.0f);
                    placedCount += 1;
                }
                DrawTrashItem(item);
            }

            std::string progress = "trash in bin " + std::to_string(placedCount) + " / " + std::to_string(static_cast<int>(takeOutTrash.items.size()));
            Text::DrawStringCentered("drag trash into the center bin", vec2(0.0f, -panelHalf.y + 58.0f), 14.0f / zoom, 2.1f);
            Text::DrawStringCentered(progress, vec2(0.0f, -panelHalf.y + 92.0f), 13.0f / zoom, 2.0f);

            if (placedCount >= static_cast<int>(takeOutTrash.items.size())) {
                takeOutTrash.phase = TrashPhase::GoOutsidePrompt;
            }
        }

        if (takeOutTrash.phase == TrashPhase::GoOutsidePrompt) {
            Text::DrawStringCentered("all trash collected", vec2(0.0f, 8.0f), 18.0f / zoom, 2.1f);
            Text::DrawStringCentered("go outside and press e at trashcan", vec2(0.0f, -30.0f), 14.0f / zoom, 2.0f);

            vec2 continueCenter = vec2(0.0f, -84.0f);
            vec2 continueHalf = vec2(140.0f, 24.0f);
            if (taskPopupControlsMenu != nullptr) {
                Button& goOutside = UI::AddButton(*taskPopupControlsMenu, "trash-go-outside", "go outside", continueCenter, continueHalf, 0);
                goOutside.labelSize = 15.0f / zoom;
                goOutside.labelSpacing = 2.1f;
                goOutside.fallbackColor = vec4(0.29f, 0.52f, 0.28f, 1.0f);
                goOutside.fallbackHoverColor = vec4(0.22f, 0.60f, 0.28f, 1.0f);
                goOutside.fallbackPressedColor = vec4(0.18f, 0.44f, 0.22f, 1.0f);
                goOutside.onClick = []() {
                    bool alreadyPending = false;
                    for (const PendingTrashDropoff& dropoff : pendingTrashDropoffs) {
                        if (dropoff.active && dropoff.taskRoom == activeTaskRoom && dropoff.taskName == activeTaskName) {
                            alreadyPending = true;
                            break;
                        }
                    }

                    if (!alreadyPending) {
                        PendingTrashDropoff dropoff;
                        dropoff.taskName = activeTaskName;
                        dropoff.taskRoom = activeTaskRoom;
                        pendingTrashDropoffs.push_back(dropoff);
                    }

                    takeOutTrashDropoffPlacementRequested = true;
                    takeOutTrashDropoffRequestedRoom = activeTaskRoom;
                    takeOutTrashDropoffRequestedTaskName = activeTaskName;
                    taskOpen = false;
                    activeTaskIndex = -1;
                    activeTaskRoom = -1;
                    activeTaskName.clear();
                };
            }
        }
    }

    inline void DrawLaundryContent(vec2 panelHalf, float zoom, vec2 mouseUI) {
        if (!laundry.initialized) {
            InitializeLaundryAssets();
        }

        vec2 machineCenter = vec2(0.0f, -6.0f);
        vec2 machineHalf = vec2(panelHalf.x * 0.21f, panelHalf.y * 0.52f);

        if (laundry.outsideMachineTexture != 0) {
            Image::Draw(laundry.outsideMachineTexture, machineCenter, machineHalf, 0.0f);
        } else {
            Image::DrawRect(machineCenter, machineHalf, 0.74f, 0.76f, 0.80f, 1.0f, 0.0f);
            Image::DrawRect(machineCenter + vec2(0.0f, 46.0f), vec2(machineHalf.x * 0.54f, machineHalf.y * 0.34f), 0.50f, 0.53f, 0.58f, 1.0f, 0.0f);
        }

        vec2 insideCenter = machineCenter + vec2(0.0f, 8.0f);
        vec2 insideSize = vec2(machineHalf.x * 0.96f, machineHalf.x * 0.96f);
        if (laundry.insideMachineTexture != 0) {
            Image::Draw(laundry.insideMachineTexture, insideCenter, insideSize, laundry.started ? laundry.washProgress * 360.0f : 0.0f);
        } else {
            Image::DrawRect(insideCenter, insideSize, 0.92f, 0.93f, 0.96f, 0.75f, laundry.started ? laundry.washProgress * 360.0f : 0.0f);
        }

        bool holdingToWash = Mouse::IsDown(0) && BoxCollide(mouseUI, vec2(0.0f), machineCenter, machineHalf);
        const float fillRate = 0.0105f;
        const float drainRate = 0.0022f;

        if (holdingToWash) {
            laundry.started = true;
            laundry.washProgress += fillRate;
        } else if (laundry.started && laundry.washProgress > 0.0f) {
            laundry.washProgress -= drainRate;
        }

        if (laundry.washProgress < 0.0f) {
            laundry.washProgress = 0.0f;
        } else if (laundry.washProgress > 1.0f) {
            laundry.washProgress = 1.0f;
        }

        vec2 barCenter = vec2(0.0f, -panelHalf.y + 102.0f);
        vec2 barHalf = vec2(panelHalf.x * 0.44f, 16.0f);
        Image::DrawRect(barCenter, barHalf, 0.22f, 0.24f, 0.28f, 1.0f, 0.0f);

        float fillRatio = laundry.washProgress;
        if (fillRatio > 0.0f) {
            float leftX = barCenter.x - barHalf.x;
            float fillWidth = barHalf.x * 2.0f * fillRatio;
            vec2 fillHalf = vec2(fillWidth * 0.5f, barHalf.y - 3.0f);
            vec2 fillCenter = vec2(leftX + fillHalf.x, barCenter.y);
            Image::DrawRect(fillCenter, fillHalf, 0.24f, 0.66f, 0.30f, 1.0f, 0.0f);
        }

        int percent = static_cast<int>(laundry.washProgress * 100.0f + 0.5f);
        Text::DrawStringCentered("wash progress " + std::to_string(percent) + "%", vec2(0.0f, -panelHalf.y + 56.0f), 13.0f / zoom, 2.0f);
        Text::DrawStringCentered("click and hold on washer to start", vec2(0.0f, -panelHalf.y + 134.0f), 14.0f / zoom, 2.1f);

        if (laundry.started && laundry.washProgress >= 1.0f) {
            Image::DrawRect(vec2(0.0f, -20.0f), vec2(panelHalf.x * 0.70f, 90.0f), 0.76f, 0.90f, 0.78f, 1.0f, 0.0f);
            Text::DrawStringCentered("laundry is done", vec2(0.0f, 0.0f), 20.0f / zoom, 2.2f);
            Text::DrawStringCentered("task complete", vec2(0.0f, -36.0f), 16.0f / zoom, 2.1f);

            if (!laundry.completionSent) {
                completionTaskIndex = activeTaskIndex;
                completionTaskRoom = activeTaskRoom;
                completionTaskName = activeTaskName;
                taskCompleteRequested = true;
                laundry.completionSent = true;
            }
        }
    }

    inline void DrawTakeOutTrashWorldPrompt(int playerRoom, float zoom) {
        if (playerRoom != 0) {
            return;
        }

        for (const PendingTrashDropoff& dropoff : pendingTrashDropoffs) {
            if (!dropoff.active || !dropoff.hasWorldPos) {
                continue;
            }

            if (takeOutTrash.outsideTrashcanTexture != 0) {
                Image::Draw(takeOutTrash.outsideTrashcanTexture, dropoff.worldPos, vec2(48.0f, 64.0f), 0.0f);
            } else {
                Image::DrawRect(dropoff.worldPos, vec2(42.0f, 58.0f), 0.18f, 0.24f, 0.20f, 1.0f, 0.0f);
            }

            Text::DrawStringCentered("press e", dropoff.worldPos + vec2(0.0f, 78.0f), 13.0f / zoom, 2.0f);
        }
    }

    inline void TryTakeOutTrashOutsideDropoff(vec2 playerPos, vec2 playerDim, int playerRoom, bool interactPressed) {
        if (playerRoom != 0) {
            for (PendingTrashDropoff& dropoff : pendingTrashDropoffs) {
                if (dropoff.active) {
                    dropoff.readyForInteract = false;
                }
            }
            return;
        }

        for (PendingTrashDropoff& dropoff : pendingTrashDropoffs) {
            if (!dropoff.active) {
                continue;
            }

            if (!interactPressed) {
                dropoff.readyForInteract = true;
            }
        }

        if (!interactPressed) {
            return;
        }

        for (PendingTrashDropoff& dropoff : pendingTrashDropoffs) {
            if (!dropoff.active || !dropoff.hasWorldPos) {
                continue;
            }

            if (!dropoff.readyForInteract) {
                continue;
            }

            if (!BoxCollide(playerPos, playerDim, dropoff.worldPos, vec2(64.0f, 82.0f))) {
                continue;
            }

            dropoff.active = false;
            completionTaskIndex = -1;
            completionTaskRoom = dropoff.taskRoom;
            completionTaskName = dropoff.taskName;
            taskCompleteRequested = true;
            taskOpen = true;
            break;
        }
    }

    inline bool ConsumeTakeOutTrashDropoffPlacementRequest(int& taskRoom, std::string& taskName) {
        if (!takeOutTrashDropoffPlacementRequested) {
            return false;
        }

        taskRoom = takeOutTrashDropoffRequestedRoom;
        taskName = takeOutTrashDropoffRequestedTaskName;
        takeOutTrashDropoffPlacementRequested = false;
        takeOutTrashDropoffRequestedRoom = -1;
        takeOutTrashDropoffRequestedTaskName.clear();
        return true;
    }

    inline void SetTakeOutTrashDropoffWorldPos(const std::string& taskName, int taskRoom, vec2 worldPos) {
        for (PendingTrashDropoff& dropoff : pendingTrashDropoffs) {
            if (!dropoff.active) {
                continue;
            }

            if (dropoff.taskRoom == taskRoom && dropoff.taskName == taskName && !dropoff.hasWorldPos) {
                dropoff.worldPos = worldPos;
                dropoff.hasWorldPos = true;
                dropoff.readyForInteract = false;
                return;
            }
        }
    }

    inline void OpenTask(int taskIndex, const std::string& taskName, int taskRoom) {
        taskOpen = true;
        taskCompleteRequested = false;
        activeTaskIndex = taskIndex;
        activeTaskRoom = taskRoom;
        activeTaskName = taskName;

        if (IsWashDishesTask()) {
            ResetWashDishesState();
        }

        if (IsTakeOutTrashTask()) {
            ResetTakeOutTrashState();
        }

        if (IsLaundryTask()) {
            ResetLaundryState();
        }
    }

    inline void CloseTask() {
        taskOpen = false;
        taskCompleteRequested = false;
        activeTaskIndex = -1;
        activeTaskRoom = -1;
        taskPopupCloseRequested = false;
        taskPopupUiInitialized = false;
        taskPopupControlsMenu = nullptr;
        taskCompletionDisplayFrames = 0;

        if (IsWashDishesTask()) {
            ResetWashDishesState();
        }

        if (IsTakeOutTrashTask()) {
            ResetTakeOutTrashState();
        }

        if (IsLaundryTask()) {
            ResetLaundryState();
        }

        activeTaskName.clear();
    }

    inline void DeleteTaskUI() {
        UI::RemoveMenu("minigame-task-popup");
        UI::RemoveMenu("minigame-task-controls");
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
            completionTaskIndex = activeTaskIndex;
            completionTaskRoom = activeTaskRoom;
            completionTaskName = activeTaskName;
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

    inline int GetCompletionTaskIndex() {
        return completionTaskIndex;
    }

    inline int GetCompletionTaskRoom() {
        return completionTaskRoom;
    }

    inline const std::string& GetCompletionTaskName() {
        return completionTaskName;
    }

    inline bool DrawTaskPopup(vec2 screen, float zoom, vec2 mouseUI) {
        if (!taskOpen) {
            // Hide menus if not completed, only remove if task was completed
            Menu* popupMenu = UI::FindMenu("minigame-task-popup");
            Menu* controlsMenu = UI::FindMenu("minigame-task-controls");
            
            if (taskCompletionDisplayFrames > 0) {
                // Task was completed, so remove the UI
                taskPopupUiInitialized = false;
                taskPopupControlsMenu = nullptr;
                taskCompletionDisplayFrames = 0;
                UI::RemoveMenu("minigame-task-popup");
                UI::RemoveMenu("minigame-task-controls");
            } else {
                // Task was closed without completing, just hide the UI
                if (popupMenu != nullptr) {
                    popupMenu->visible = false;
                    popupMenu->enabled = false;
                }
                if (controlsMenu != nullptr) {
                    controlsMenu->visible = false;
                    controlsMenu->enabled = false;
                }
            }
            return false;
        }

        taskPopupFullSize = screen / zoom;
        taskPopupPanelHalf = vec2(taskPopupFullSize.x * 0.56f, taskPopupFullSize.y * 0.50f);
        taskPopupZoom = zoom;
        taskPopupTitle = activeTaskName;

        // Auto-close task after completion is shown for a brief time
        if (taskCompleteRequested) {
            taskCompletionDisplayFrames++;
            if (taskCompletionDisplayFrames >= 120) {  // ~2 seconds at 60fps
                DeleteTaskUI();
                CloseTask();
                return true;
            }
        } else {
            taskCompletionDisplayFrames = 0;
        }

        Menu* popupMenu = EnsureTaskPopupUiMenu();
        if (popupMenu != nullptr) {
            popupMenu->visible = true;
            popupMenu->enabled = true;

            for (UiLabel& label : popupMenu->labels) {
                if (label.id == "panel-title") {
                    label.text = taskPopupTitle;
                    label.size = 24.0f / zoom;
                    break;
                }
            }

            for (Button& button : popupMenu->buttons) {
                if (button.id == "panel-close") {
                    button.labelSize = 18.0f / zoom;
                    break;
                }
            }

            popupMenu->update(mouseUI);
            popupMenu->draw();
        }

        Menu* controlsMenu = UI::FindMenu("minigame-task-controls");
        if (controlsMenu == nullptr) {
            controlsMenu = &UI::CreateMenu("minigame-task-controls");
        }
        controlsMenu->panels.clear();
        controlsMenu->images.clear();
        controlsMenu->labels.clear();
        controlsMenu->buttons.clear();
        controlsMenu->visible = true;
        controlsMenu->enabled = true;
        taskPopupControlsMenu = controlsMenu;

        if (IsWashDishesTask()) {
            DrawWashDishesContent(taskPopupPanelHalf, zoom, mouseUI);
        } else if (IsTakeOutTrashTask()) {
            DrawTakeOutTrashContent(taskPopupPanelHalf, zoom, mouseUI);
        } else if (IsLaundryTask()) {
            DrawLaundryContent(taskPopupPanelHalf, zoom, mouseUI);
        } else {
            DrawGenericTaskContent(zoom);
        }

        if (controlsMenu != nullptr) {
            controlsMenu->update(mouseUI);
            controlsMenu->draw();
        }
        taskPopupControlsMenu = nullptr;

        if (taskPopupCloseRequested) {
            taskPopupCloseRequested = false;
            CloseTask();
            return true;
        }

        return false;
    }

}
