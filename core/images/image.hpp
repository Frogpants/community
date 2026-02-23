#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <GL/gl.h>
#include "../essentials.hpp"

namespace Image {

    bool Init();
    GLuint Load(const char* path);
    void Draw(GLuint texture, vec2 pos, float size, float angle = 0.0);
    void DrawRect(vec2 pos, vec2 size, float r, float g, float b, float angle = 0.0);

}

#endif
