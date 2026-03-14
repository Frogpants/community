#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>

#include "game/player.hpp"
#include "game/character.hpp"
#include "game/camera.hpp"
#include "game/multiplayer.hpp"
#include "game/tile.hpp"
#include "game/task.hpp"

#include "core/manager.hpp"
#include "core/essentials.hpp"
#include "core/collision.hpp"
#include "core/images/image.hpp"
#include "core/images/stb_image.h"
#include "core/text.hpp"
#include "core/file.hpp"


// Game Assets

Player player;
Camera camera;
Character character;

std::vector<Character> characters;
std::vector<Task> objectives;

std::vector<std::string> tileTex = grabFiles("dist/assets/tiles");
std::vector<std::string> itemTex = grabFiles("dist/assets/items");
std::vector<std::string> heartTex = grabFiles("dist/assets/stats/health");

// Game Control Variables

int tick = 1;

bool editor = false;
int mode = 0;
int selectMode = 0;
int selected = 0;
int tile = 0;

float zoom = 2.0;

int running = 1;
bool inMainMenu = true;
bool multiplayerStarted = false;

std::string getEnvOrDefault(const char* key, const std::string& fallback) {
    const char* value = std::getenv(key);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    return std::string(value);
}

int getEnvIntOrDefault(const char* key, int fallback) {
    const char* value = std::getenv(key);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }

    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

std::string getRandomUsername() {
    const std::vector<std::string> adjectives = {
        "brisk", "calm", "clever", "fuzzy", "gentle", "lucky", "mellow", "nimble", "quiet", "rapid", "sunny", "zesty"
    };

    const std::vector<std::string> nouns = {
        "otter", "fox", "raven", "koala", "panda", "sparrow", "beaver", "lynx", "wolf", "badger", "falcon", "gecko"
    };

    int adjectiveIndex = randInt(0, static_cast<int>(adjectives.size()) - 1);
    int nounIndex = randInt(0, static_cast<int>(nouns.size()) - 1);
    int suffix = randInt(100, 999);

    return adjectives[adjectiveIndex] + "-" + nouns[nounIndex] + std::to_string(suffix);
}


std::vector<Tile> genWorld(vec2 dim) {
    std::vector<Tile> ts; // Nora shhhhh
    Tile t;
    for (int x = 0; x < dim.x; ++x) {
        for (int y = 0; y < dim.y; ++y) {
            if (randInt(0, 1) == 0) {
                t.id = 1;
            } else {
                t.id = randInt(2, 3);
            }
            t.pos = snap(vec2((x - dim.x/2.0)*64, (y - dim.y/2.0)*64), 64.0);
            ts.push_back(t);
        }
    }
    return ts;
}


vec2 GetMouseWorld(GLFWwindow* window) {
    vec2 world;

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    float mouseNDC_X = (float)Mouse::X() / windowWidth;
    float mouseNDC_Y = (float)Mouse::Y() / windowHeight;

    world.x = (mouseNDC_X * 2.0f - 1.0f) * screen.x / zoom + camera.pos.x;
    world.y = (1.0f - mouseNDC_Y * 2.0f) * screen.y / zoom + camera.pos.y;

    return world;
}

vec2 GetMouseUI(GLFWwindow* window) {
    vec2 ui;

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    float mouseNDC_X = static_cast<float>(Mouse::X()) / static_cast<float>(windowWidth);
    float mouseNDC_Y = static_cast<float>(Mouse::Y()) / static_cast<float>(windowHeight);

    ui.x = (mouseNDC_X * 2.0f - 1.0f) * screen.x / zoom;
    ui.y = (1.0f - mouseNDC_Y * 2.0f) * screen.y / zoom;

    return ui;
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    // Update the screen variable with new dimensions
    screen = vec2(width, height);
    
    // Update the OpenGL viewport
    glViewport(0, 0, width, height);
    
    // Update the projection matrix with new dimensions
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-screen.x / zoom, screen.x / zoom, -screen.y / zoom, screen.y / zoom, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

int addTask(const std::vector<std::string>& ts, const std::vector<std::string>& ownedTasks) {
    if (ts.empty()) {
        return -1;
    }

    std::vector<int> availableTaskIds;
    for (int i = 0; i < static_cast<int>(ts.size()); ++i) {
        bool alreadyOwned = false;
        for (const std::string& owned : ownedTasks) {
            if (owned == ts[i]) {
                alreadyOwned = true;
                break;
            }
        }

        if (!alreadyOwned) {
            availableTaskIds.push_back(i);
        }
    }

    if (availableTaskIds.empty()) {
        return -1;
    }

    int choice = randInt(0, static_cast<int>(availableTaskIds.size()) - 1);
    int t = availableTaskIds[choice];
    Task task;
    task.id = t;
    task.pos = GetTaskSpawnPosition(ts[t], t);
    objectives.push_back(task);
    return t;
}


int main()
{
    srand((unsigned int)time(nullptr));
    std::vector<Tile> tiles;
    //tiles = genWorld(vec2(120,100));
    if (!isEmpty("game/data/map.dat")) {
        tiles = load("game/data/map.dat");
    }

    if (!glfwInit())
        return -1;

    GLFWwindow* window = glfwCreateWindow(screen.x, screen.y, "Community", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Load and set window icon
    int iconWidth, iconHeight, iconChannels;
    unsigned char* iconPixels = stbi_load("assets/icon.png", &iconWidth, &iconHeight, &iconChannels, 4);
    if (iconPixels) {
        GLFWimage icon;
        icon.width = iconWidth;
        icon.height = iconHeight;
        icon.pixels = iconPixels;
        glfwSetWindowIcon(window, 1, &icon);
        stbi_image_free(iconPixels);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Register window resize callback
    glfwSetWindowSizeCallback(window, windowResizeCallback);
    
    // Initialize screen variable with actual window size
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    screen = vec2(windowWidth, windowHeight);

    Manager::Init(window);
    Image::Init();
    if (!Text::Init()) {
        std::cout << "Failed to initialize text glyph textures." << std::endl;
    }

    std::vector<GLuint> tileTextures = loadTextures(tileTex);
    std::vector<GLuint> itemTextures = loadTextures(itemTex);
    std::vector<GLuint> heartTextures = loadTextures(heartTex);


    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-screen.x / zoom, screen.x / zoom, -screen.y / zoom, screen.y / zoom, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    player.texture = Image::Load("assets/agent-bullet.png");
    character.texture = Image::Load("assets/npcs/character.png");
    character.pos = vec2(300.0, 0.0);
    character.roamCenter = character.pos; // Set roam center to initial position
    
    
    GLuint taskTex = Image::Load("assets/box.png");
    GLuint pause = Image::Load("assets/pause-improved.png");
    GLuint deleteTex = Image::Load("assets/delete.png");
    GLuint selectTex = Image::Load("assets/interact-select.png");

    MultiplayerClient multiplayer;
    multiplayer.setServer(getEnvOrDefault("COMMUNITY_BACKEND_HOST", "localhost"), getEnvIntOrDefault("COMMUNITY_BACKEND_PORT", 8080));
    multiplayer.setPlayerName(getEnvOrDefault("COMMUNITY_PLAYER_NAME", getRandomUsername()));
    std::string requestedRoomCode = getEnvOrDefault("COMMUNITY_ROOM_CODE", "");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glLoadIdentity();

        if (inMainMenu) {
            vec2 mouseUI = GetMouseUI(window);
            vec2 playCenter = vec2(0.0f, -80.0f);
            vec2 playHalfSize = vec2(220.0f, 70.0f);
            bool hoveringPlay = BoxCollide(playCenter, playHalfSize, mouseUI, vec2(0.0f));

            Image::DrawRect(vec2(0.0f, 0.0f), vec2(screen.x / zoom, screen.y / zoom), 0.94f, 0.97f, 0.92f);
            if (hoveringPlay) {
                Image::DrawRect(playCenter, playHalfSize, 0.25f, 0.70f, 0.43f);
            } else {
                Image::DrawRect(playCenter, playHalfSize, 0.20f, 0.58f, 0.38f);
            }

            Text::DrawStringCentered("community", vec2(0.0f, 180.0f), 42.0f / zoom, 1.35f);
            Text::DrawStringCentered("play", playCenter - vec2(0.0), 28.0f / zoom, 1.3f);
            Text::DrawStringCentered("click play to start", vec2(0.0f, -220.0f), 18.0f / zoom, 1.3f);

            if (hoveringPlay && Mouse::IsPressed(0)) {
                inMainMenu = false;
                if (!multiplayerStarted) {
                    multiplayer.initOrJoin(requestedRoomCode);
                    multiplayerStarted = true;
                }
            }

            Manager::Update();
            glfwSwapBuffers(window);
            continue;
        }

        glTranslatef(-camera.pos.x, -camera.pos.y, 0);

        vec2 mouse = GetMouseWorld(window);
        int i = 0;
        selected = -1;
        for (const Tile& t : tiles) {
            if (abs(t.pos.x - camera.pos.x) < (screen.x + 64.0) / zoom) {
                if (abs(t.pos.y - camera.pos.y) < (screen.y + 64.0) / zoom) {
                    Image::Draw(tileTextures[t.id], t.pos, 32, 0.0);
                    if (BoxCollide(mouse, vec2(0.0), t.pos, vec2(32.0))) {
                        selected = i;
                    }
                }
            }
            ++i;
        }

        if (running) {
            tick += 1;

            if(Input::IsPressed("0")) {
                mode = 1 - mode;
            }

            if (mode) {
                camera.controls();

                if (Input::IsPressed("1")) {
                    selectMode = 0;
                }

                if (Input::IsPressed("2")) {
                    selectMode = 1;
                }

                Tile t;
                t.pos = snap(mouse + 32, 64.0);

                if (Input::IsPressed("x")) {
                    tile += 1;
                }

                if (Input::IsPressed("z")) {
                    tile -= 1;
                }

                tile = std::clamp(tile, 0, (int)tileTextures.size() - 1);
                t.id = tile;

                if (selectMode == 0) {
                    Image::Draw(tileTextures[tile], t.pos, 32);
                } else {
                    Image::Draw(deleteTex, t.pos, 32);
                }
                    
                if (Mouse::IsPressed(0)) {
                    if (selectMode == 0) {
                        tiles.push_back(t);
                    } else {
                        if (selected != -1) {
                            tiles.erase(tiles.begin() + selected);
                        }
                    }

                    save(tiles, "game/data/map.dat");
                } else if (Mouse::IsDown(1)) {
                    if (selectMode == 0) {
                        tiles.push_back(t);
                    } else {
                        if (selected != -1) {
                            tiles.erase(tiles.begin() + selected);
                        }
                    }

                    save(tiles, "game/data/map.dat");
                }

            } else {
                camera.target = player.pos;
                camera.follow();

                player.controls();
                player.pos = player.pos + player.vel;
                multiplayer.sync(player.pos);

                if (BoxCollide(player.pos, player.dim, character.pos, character.dim) && Input::IsPressed("e")) {
                    int taskId = addTask(character.tasks, player.tasks);
                    if (taskId != -1) {
                        const std::string& taskName = character.tasks[taskId];
                        player.tasks.push_back(taskName);
                        std::cout << "Task added: " << taskName << std::endl;
                    } else {
                        std::cout << "All tasks completed. No more tasks available." << std::endl;
                    }
                }

                // Character starts roaming when at max level (90)
                if (character.level >= 90) {
                    character.isRoaming = true;
                }

                if (character.isRoaming) {
                    character.roam();
                }

                Image::Draw(selectTex, snap(mouse + 32, 64.0), 32);
            }

            player.health = clamp(0.0, 100.0, player.health);
        } else {
            // Game Paused

            //Image::Draw(pause, camera.pos, 1500);
        }

        int id = 0;
        for (const Task& t : objectives) {
            if (BoxCollide(player.pos, player.dim, t.pos, t.dim)) {
                if (Input::IsPressed("e")) {
                    objectives.erase(objectives.begin() + id);
                    player.tasks.erase(player.tasks.begin() + id);
                    character.level += 30;
                }
            }
            ++id;
            Image::Draw(taskTex, t.pos, 45.0);
        }

        Image::Draw(player.texture, player.pos, 150);
        multiplayer.drawRemotePlayers(player.texture);
        Image::Draw(character.texture, character.pos, 150);

        // UI
        glLoadIdentity();
        
        for (int i = 0; i < 9; ++i) {
            float perc = (character.level/90.0) - (i * 0.1);
            GLuint tex;
            if (perc >= 0.1) {
                tex = heartTextures[1];
            } else if (perc >= 0.05) {
                tex = heartTextures[2];
            } else {
                tex = heartTextures[0];
            }
            Image::Draw(tex, vec2(-screen.x + 64*i + 48, screen.y - 48) / zoom, 16);
        }

        std::string levelText = "level " + std::to_string(character.level);
        Text::DrawString(levelText, vec2(-screen.x + 40, screen.y - 130) / zoom, 24.0f / zoom, 1.5f);

        std::string taskText = "objectives " + std::to_string(player.tasks.size());
        Text::DrawString(taskText, vec2(screen.x - 600, screen.y - 48) / zoom, 24.0f / zoom, 1.5f);

        Text::DrawString(multiplayer.getStatusText(), vec2(-screen.x + 40, screen.y - 180) / zoom, 20.0f / zoom, 1.5f);

        float y = 112.0;
        for (const std::string& t : player.tasks) {
            taskText = "- " + t;
            Text::DrawString(taskText, vec2(screen.x - 600, screen.y - y) / zoom, 24.0f / zoom, 1.5f);
            y += 64.0;
        }

        if (!running) {
            Text::DrawStringCentered("paused", vec2(0.0f, screen.y - 90.0f) / zoom, 40.0f / zoom, 1.5f);
        }

        if(Input::IsPressed("escape")) {
            running = 1 - running;
        }

        Manager::Update();

        glfwSwapBuffers(window);
    }

    multiplayer.shutdown();
    glfwTerminate();
    return 0;
}
