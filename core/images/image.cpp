#include "image.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <limits.h>
#include <unistd.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <mach-o/dyld.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace Image {

    std::unordered_map<GLuint, std::pair<int, int>> textureSizes;

    ////////////////////////////////////////////////////////////
    // HELPER: Get directory of the running executable
    ////////////////////////////////////////////////////////////
    std::string GetExeDir()
    {
    #ifdef _WIN32
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string path(buffer);
        size_t pos = path.find_last_of("\\/");
        return (std::string::npos == pos) ? "" : path.substr(0, pos);
    #elif defined(__APPLE__)
        uint32_t size = 0;
        _NSGetExecutablePath(NULL, &size);
        std::vector<char> buf(size);
        if (_NSGetExecutablePath(buf.data(), &size) == 0) {
            std::string path(buf.data());
            size_t pos = path.find_last_of('/');
            return (std::string::npos == pos) ? "." : path.substr(0, pos);
        }
        return ".";
    #else
        // Try Linux /proc/self/exe
        char buff[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buff, sizeof(buff)-1);
        if (len != -1) {
            buff[len] = '\0';
            std::string path(buff);
            size_t pos = path.find_last_of('/');
            return (std::string::npos == pos) ? "." : path.substr(0, pos);
        }
        return ".";
    #endif
    }

    ////////////////////////////////////////////////////////////
    // INITIALIZE IMAGE SYSTEM
    ////////////////////////////////////////////////////////////
    bool Init()
    {
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        return true;
    }

    ////////////////////////////////////////////////////////////
    // LOAD TEXTURE
    ////////////////////////////////////////////////////////////
    GLuint Load(const char* filename, bool logFailure)
    {
        std::string exeDir = GetExeDir();

        std::string fullPath;
    #ifdef _WIN32
        fullPath = exeDir + "\\" + filename;
    #else
        fullPath = exeDir + "/" + filename;
    #endif

        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);

        // Try several candidate paths until one works
        std::vector<std::string> candidates;
        candidates.push_back(fullPath);
        candidates.push_back(filename);
        candidates.push_back(std::string("./") + filename);

        unsigned char* data = nullptr;
        for (const auto& p : candidates) {
            data = stbi_load(p.c_str(), &width, &height, &channels, 4);
            if (data) {
                fullPath = p;
                break;
            }
        }

        if (!data) {
            if (logFailure) {
                std::cout << "Failed to load texture: " << filename << std::endl;
            }
            return 0;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        textureSizes[texture] = { width, height };

        stbi_image_free(data);
        return texture;
    }

    bool GetTextureSize(GLuint texture, int& width, int& height)
    {
        auto it = textureSizes.find(texture);
        if (it == textureSizes.end()) {
            return false;
        }

        width = it->second.first;
        height = it->second.second;
        return true;
    }

    ////////////////////////////////////////////////////////////
    // DRAW TEXTURED QUAD
    ////////////////////////////////////////////////////////////
    void Draw(GLuint texture, vec2 pos, float size, float angle)
    {
        glPushMatrix();

        glTranslatef(pos.x, pos.y, 0);
        glRotatef(angle, 0, 0, 1);

        glBindTexture(GL_TEXTURE_2D, texture);
        glColor3f(1, 1, 1);

        glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex2f(-size, -size);
            glTexCoord2f(1, 0); glVertex2f( size, -size);
            glTexCoord2f(1, 1); glVertex2f( size,  size);
            glTexCoord2f(0, 1); glVertex2f(-size,  size);
        glEnd();

        glPopMatrix();
    }

    void Draw(GLuint texture, vec2 pos, vec2 size, float angle)
    {
        glPushMatrix();

        glTranslatef(pos.x, pos.y, 0);
        glRotatef(angle, 0, 0, 1);

        glBindTexture(GL_TEXTURE_2D, texture);
        glColor3f(1, 1, 1);

        glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex2f(-size.x, -size.y);
            glTexCoord2f(1, 0); glVertex2f( size.x, -size.y);
            glTexCoord2f(1, 1); glVertex2f( size.x,  size.y);
            glTexCoord2f(0, 1); glVertex2f(-size.x,  size.y);
        glEnd();

        glPopMatrix();
    }

    void DrawRegion(GLuint texture, vec2 pos, float size, float u0, float v0, float u1, float v1, float angle)
    {
        glPushMatrix();

        glTranslatef(pos.x, pos.y, 0);
        glRotatef(angle, 0, 0, 1);

        glBindTexture(GL_TEXTURE_2D, texture);
        glColor3f(1, 1, 1);

        glBegin(GL_QUADS);
            glTexCoord2f(u0, v0); glVertex2f(-size, -size);
            glTexCoord2f(u1, v0); glVertex2f( size, -size);
            glTexCoord2f(u1, v1); glVertex2f( size,  size);
            glTexCoord2f(u0, v1); glVertex2f(-size,  size);
        glEnd();

        glPopMatrix();
    }

    ////////////////////////////////////////////////////////////
    // DRAW COLORED RECTANGLE (NO TEXTURE)
    ////////////////////////////////////////////////////////////
    void DrawRect(vec2 pos, vec2 size, float r, float g, float b, float a, float angle)
    {
        glPushMatrix();

        glTranslatef(pos.x, pos.y, 0);
        glRotatef(angle, 0, 0, 1);

        glBindTexture(GL_TEXTURE_2D, 0);  // disable texture
        glColor4f(r, g, b, a);

        glBegin(GL_QUADS);
            glVertex2f(-size.x, -size.y);
            glVertex2f( size.x, -size.y);
            glVertex2f( size.x,  size.y);
            glVertex2f(-size.x,  size.y);
        glEnd();

        glPopMatrix();
    }

    void DrawRect(vec2 pos, vec2 size, float r, float g, float b, float angle)
    {
        glPushMatrix();

        glTranslatef(pos.x, pos.y, 0);
        glRotatef(angle, 0, 0, 1);

        glBindTexture(GL_TEXTURE_2D, 0);  // disable texture
        glColor3f(r, g, b);

        glBegin(GL_QUADS);
            glVertex2f(-size.x, -size.y);
            glVertex2f( size.x, -size.y);
            glVertex2f( size.x,  size.y);
            glVertex2f(-size.x,  size.y);
        glEnd();

        glPopMatrix();
    }

}
