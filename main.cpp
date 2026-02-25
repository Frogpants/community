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

#include "game/player.hpp"
#include "game/character.hpp"
#include "game/camera.hpp"
#include "game/tile.hpp"

#include "core/manager.hpp"
#include "core/essentials.hpp"
#include "core/collision.hpp"
#include "core/images/image.hpp"
#include "core/file.hpp"


// Game Assets

Player player;
Camera camera;
std::vector<Character> characters;

std::vector<std::string> tileTex = grabFiles("dist/assets/tiles");

// Game Control Variables

int tick = 0;

bool editor = false;
int mode = 0;
int selectMode = 0;
int selected = 0;
int tile = 0;

int running = 1;


void DrawHealthBar(float health)
{
    float width = 200.0f;
    float height = 20.0f;

    float hpPercent = health / 100.0f;

    Image::DrawRect(vec2(-screen.x + 20, screen.y - 40), vec2(width, height), 1, 0, 0);
    Image::DrawRect(vec2(-screen.x + 20, screen.y - 40), vec2(width * hpPercent, height), 0, 1, 0);
}


vec2 GetMouseWorld(GLFWwindow* window) {
    vec2 world;

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    // Normalize mouse coordinates to -screen.x → screen.x and -screen.y → screen.y
    float mouseNDC_X = (float)Mouse::X() / windowWidth;  // 0 → 1
    float mouseNDC_Y = (float)Mouse::Y() / windowHeight; // 0 → 1

    // Convert to ortho coordinates
    world.x = (mouseNDC_X * 2.0f - 1.0f) * screen.x + camera.pos.x;
    world.y = (1.0f - mouseNDC_Y * 2.0f) * screen.y + camera.pos.y;

    return world;
}


int main()
{
    std::vector<Tile> tiles;
    if (!isEmpty("game/data/map.dat")) {
        tiles = load("game/data/map.dat");
    }
    
    srand((unsigned int)time(nullptr));

    if (!glfwInit())
        return -1;

    GLFWwindow* window = glfwCreateWindow(screen.x, screen.y, "Oasis", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    std::vector<GLuint> tileTextures;

    for (const std::string& path : tileTex)
    {
        GLuint img = Image::Load(path.c_str());
        tileTextures.push_back(img);
    }

    // Setup 2D projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-screen.x, screen.x, -screen.y, screen.y, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    Manager::Init(window);
    Image::Init();

    player.texture = Image::Load("assets/agent-bullet.png");

    GLuint pause = Image::Load("assets/pause.png");
    GLuint deleteTex = Image::Load("assets/delete.png");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glLoadIdentity();

        glTranslatef(-camera.pos.x, -camera.pos.y, 0);

        vec2 mouse = GetMouseWorld(window);
        int i = 0;
        for (const Tile& t : tiles) {
            if (abs(t.pos.x - camera.pos.x) < screen.x) {
                if (abs(t.pos.y - camera.pos.y) < screen.y) {
                    Image::Draw(tileTextures[t.id], t.pos, 32, 0.0);
                    if (BoxCollide(mouse, vec2(0.0), t.pos, vec2(32.0))) {
                        selected = i;
                    }
                }
            }
            ++i;
        }

        Image::Draw(player.texture, player.pos, 150, 0.0);

        if (running) {
            // Game Running
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
                    Image::Draw(tileTextures[tile], t.pos, 32, 0.0);
                } else {
                    Image::Draw(deleteTex, t.pos, 32, 0.0);
                }
                    

                if (Mouse::IsPressed(0)) {
                    if (selectMode == 0) {
                        tiles.push_back(t);
                    } else {
                        tiles.erase(tiles.begin() + selected);
                    }

                    save(tiles, "game/data/map.dat");
                }
            } else {
                camera.target = player.pos;
                camera.follow();

                player.controls();
                player.pos = player.pos + player.vel;
            }

            player.health = clamp(0.0, 100.0, player.health);
        } else {
            // Game Paused

            Image::Draw(pause, camera.pos, 250, 0.0);

        }

        // UI
        glLoadIdentity();
        DrawHealthBar(player.health);

        if(Input::IsPressed("escape")) {
            running = 1 - running;
            //break;
        }

        Manager::Update();

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
