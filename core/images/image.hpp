#ifndef IMAGE_HPP
#define IMAGE_HPP

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "../essentials.hpp"

namespace Image {

    bool Init();
    GLuint Load(const char* path, bool logFailure = true);
    bool GetTextureSize(GLuint texture, int& width, int& height);
    void Draw(GLuint texture, vec2 pos, float size, float angle = 0.0);
    void Draw(GLuint texture, vec2 pos, vec2 size, float angle = 0.0);
    void DrawRegion(GLuint texture, vec2 pos, float size, float u0, float v0, float u1, float v1, float angle = 0.0);
    void DrawRect(vec2 pos, vec2 size, float r, float g, float b, float angle = 0.0);
    void DrawRect(vec2 pos, vec2 size, float r, float g, float b, float a, float angle = 0.0);

}

#endif
