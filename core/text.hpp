#pragma once

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <map>
#include <string>
#include <cctype>
#include <vector>
#include "essentials.hpp"
#include "images/image.hpp"

namespace Text {
    inline std::map<char, GLuint> charTextures;

    inline const std::vector<char>& GlyphOrder() {
        static const std::vector<char> glyphOrder = {
            'a','b','c','d','e','f','g','h','i','j','k','l','m',
            'n','o','p','q','r','s','t','u','v','w','x','y','z',
            '0','1','2','3','4','5','6','7','8','9',
            '!','(',')','*','-','?','_','.','/'
        };
        return glyphOrder;
    }

    inline std::string SymbolFileName(char c) {
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            return std::string(1, c) + ".png";
        }

        switch (c) {
            case '!': return "exclaim.png";
            case '(': return "lparen.png";
            case ')': return "rparen.png";
            case '*': return "asterisk.png";
            case '-': return "dash.png";
            case '?': return "question.png";
            case '_': return "underscore.png";
            case '.': return "dot.png";
            case '/': return "slash.png";
            default: return "";
        }
    }

    inline std::string LegacyFileName(char c) {
        switch (c) {
            case '!': return "exclaim.png";
            case '(': return "(.png";
            case ')': return ").png";
            case '*': return "asterisk.png";
            case '-': return "dash.png";
            case '?': return "question.png";
            case '_': return "underscore.png";
            case '.': return "dot.png";
            case '/': return "slashR.png";
            default: return "";
        }
    }

    inline bool LoadGlyph(char c, int index) {
        std::string symbolName = SymbolFileName(c);
        if (!symbolName.empty()) {
            std::string symbolPath = "assets/text/" + symbolName;
            GLuint tex = Image::Load(symbolPath.c_str());
            if (tex != 0) {
                charTextures[c] = tex;
                return true;
            }
        }

        std::string legacyName = LegacyFileName(c);
        if (!legacyName.empty()) {
            std::string legacyPath = "assets/text/" + legacyName;
            GLuint tex = Image::Load(legacyPath.c_str());
            if (tex != 0) {
                charTextures[c] = tex;
                return true;
            }
        }

        std::string tilePath = "assets/text/tile" + std::to_string(index + 1) + ".png";
        GLuint tileTex = Image::Load(tilePath.c_str());
        if (tileTex != 0) {
            charTextures[c] = tileTex;
            return true;
        }

        return false;
    }

    inline bool Init() {
        charTextures.clear();
        const std::vector<char>& glyphOrder = GlyphOrder();

        for (int i = 0; i < static_cast<int>(glyphOrder.size()); ++i) {
            if (!LoadGlyph(glyphOrder[i], i)) {
                return false;
            }
        }

        return true;
    }

    inline void DrawChar(char c, vec2 pos, float size, float angle = 0.0f) {
        char lowerChar = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        auto it = charTextures.find(lowerChar);
        if (it != charTextures.end()) {
            Image::Draw(it->second, pos, size, angle);
        }
    }

    inline void DrawString(const std::string& text, vec2 pos, float size, float spacing = 2.0f, float angle = 0.0f) {
        vec2 currentPos = pos;
        float charSpacing = size * spacing;

        for (char c : text) {
            if (c == ' ') {
                currentPos.x += charSpacing;
            } else if (c == '\n') {
                currentPos.x = pos.x;
                currentPos.y -= size * 2.5f;
            } else {
                DrawChar(c, currentPos, size, angle);
                currentPos.x += charSpacing;
            }
        }
    }

    inline void DrawStringCentered(const std::string& text, vec2 pos, float size, float spacing = 2.0f, float angle = 0.0f) {
        int charCount = 0;
        for (char c : text) {
            if (c != '\n') {
                charCount++;
            }
        }

        float charSpacing = size * spacing;
        float textWidth = charCount * charSpacing;

        vec2 startPos = vec2(pos.x - textWidth / 2.0f, pos.y);
        DrawString(text, startPos, size, spacing, angle);
    }

    inline void DrawStringRight(const std::string& text, vec2 pos, float size, float spacing = 2.0f, float angle = 0.0f) {
        int charCount = 0;
        for (char c : text) {
            if (c != '\n') {
                charCount++;
            }
        }

        float charSpacing = size * spacing;
        float textWidth = charCount * charSpacing;

        vec2 startPos = vec2(pos.x - textWidth, pos.y);
        DrawString(text, startPos, size, spacing, angle);
    }
}
