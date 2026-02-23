#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

#include "game/player.hpp"
#include "game/enemy.hpp"
#include "game/camera.hpp"
#include "game/bullet.hpp"

#include "core/manager.hpp"
#include "core/essentials.hpp"
#include "core/collision.hpp"
#include "core/images/image.hpp"

Player player;
Camera camera;
std::vector<Enemy> enemies;
std::vector<Bullet> bullets;

int tick = 0;


void DrawQuad(vec2 pos, float size, float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(pos.x - size, pos.y - size);
        glVertex2f(pos.x + size, pos.y - size);
        glVertex2f(pos.x + size, pos.y + size);
        glVertex2f(pos.x - size, pos.y + size);
    glEnd();
}

void DrawCircle(vec2 pos, float radius, float r, float g, float b)
{
    const int segments = 32; // more = smoother

    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);

        // Center
        glVertex2f(pos.x, pos.y);

        // Outer ring
        for (int i = 0; i <= segments; ++i)
        {
            float angle = 2.0f * 3.1415926f * i / segments;
            float x = pos.x + std::cos(angle) * radius;
            float y = pos.y + std::sin(angle) * radius;
            glVertex2f(x, y);
        }

    glEnd();
}


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
    srand((unsigned int)time(nullptr));

    if (!glfwInit())
        return -1;

    GLFWwindow* window = glfwCreateWindow(screen.x, screen.y, "Roguelike Demo", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable VSync (locks to monitor refresh, usually 60 FPS)

    // Setup 2D projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //gluPerspective(70.0, screen.x/screen.y, 0.1, 1000.0);
    //glFrustum(-screen.x, screen.x, -screen.y, screen.y, 0.1, 1000.0);
    glOrtho(-screen.x, screen.x, -screen.y, screen.y, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    
    for (int i = 0; i < 25; i++) {
        Enemy e;
        vec2 pos = vec2(0.0);
        if (randInt(0, 1) == 1) {
            pos.x = randInt(-screen.x, screen.x);
            if (randInt(0,1)) {
                pos.y = screen.y + randInt(0, 100);
            } else {
                pos.y = -(screen.y + randInt(0, 100));
            }
        } else {
            pos.y = randInt(-screen.y, screen.y);
            if (randInt(0,1)) {
                pos.x = screen.x + randInt(0, 100);
            } else {
                pos.x = -(screen.x + randInt(0, 100));
            }
        }
        e.pos = pos + camera.pos;
        enemies.push_back(e);
    }
    

    Manager::Init(window);
    Image::Init();

    GLuint playerTex = Image::Load("assets/agent-bullet.png");
    GLuint enemyTex = Image::Load("assets/agent-enemy.png");
    GLuint bulletTex = Image::Load("assets/bullet.png");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glLoadIdentity();

        tick += 1;


        camera.target = player.pos;
        camera.follow();
        glTranslatef(-camera.pos.x, -camera.pos.y, 0);

        player.controls();
        player.pos = player.pos + player.vel;

        if (Mouse::IsDown(0)) {
            player.cooldown = clamp(0.0, 100.0, player.cooldown);
            if (player.cooldown == 0.0) {
                Bullet b;
                b.pos = player.pos;

                vec2 mouseWorld = GetMouseWorld(window);

                b.dir = pointAt(player.pos, mouseWorld);

                bullets.push_back(b);
                player.cooldown = 100.0;
            }
            if (tick % 50 == 0) {
                player.cooldown -= 100.0;
            }
        }

        vec2 force = vec2(0.0);

        for (auto& b : bullets) {
            b.move();
            if (abs(b.pos.x - camera.pos.x) >= screen.x) {
                bullets.erase(bullets.begin() + findIndex(bullets, b));
            }

            if (abs(b.pos.y - camera.pos.y) >= screen.y) {
                bullets.erase(bullets.begin() + findIndex(bullets, b));
            }
            Image::Draw(bulletTex, b.pos, 100, b.dir);
        }


        for (auto& e : enemies) {
            e.target = player.pos;

            e.follow();
            vec2 separation = vec2(0.0f);

            for (auto& t : enemies) {
                if (&e == &t) continue;

                vec2 diff = e.pos - t.pos;
                float dist = length(diff) * 0.1;
                if (BallCollide(e.pos, e.dim * 12.0, t.pos, t.dim * 12.0))
                    separation = separation + normalize(diff) / dist;
            }

            e.pos = e.pos + separation * e.speed * 4.0;

            if (BoxCollide(player.pos, player.dim, e.pos, e.dim)) {
                vec2 diff = player.pos - e.pos;
                float dist = length(diff) * 0.01;

                force = force + normalize(diff) / dist;

                if (tick % 100 == 0) {
                    player.health -= 10.0;
                }
            }

            //e.pos = e.pos + force * e.speed * 4.0;

            for (auto& b : bullets) {
                if (BallCollide(e.pos, e.dim, b.pos, b.dim)) {
                    player.coins += 1.0;

                    enemies.erase(enemies.begin() + findIndex(enemies, e));
                    bullets.erase(bullets.begin() + findIndex(bullets, b));
                }
            }

            Image::Draw(enemyTex, e.pos, 150);
        }

        player.pos = player.pos + force * 2.0;
        Image::Draw(playerTex, player.pos, 150, 0.0);

        player.health = clamp(0.0, 100.0, player.health);

        // UI
        glLoadIdentity();
        DrawHealthBar(player.health);

        if(Input::IsDown("escape")) {
            // change to pause at some point
            glfwTerminate();
            return 0;
        }

        Manager::Update();

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
