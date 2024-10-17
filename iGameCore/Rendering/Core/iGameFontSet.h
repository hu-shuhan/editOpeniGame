//
// Created by Sumzeek on 7/1/2024.
//

#ifndef OPENIGAME_FONTSET_H
#define OPENIGAME_FONTSET_H

#include "OpenGL/GLTexture2d.h"
#include "iGameObject.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <map>
#include <wchar.h>

IGAME_NAMESPACE_BEGIN

struct Character {
    GLuint TextureID;   // font texture id
    igm::ivec2 Size;    // font size
    igm::ivec2 Bearing; // Offset from the baseline to the left/top of the glyph
    GLuint Advance;     // The distance from the origin to the next glyph origin
};

class FontSet : public Object {
public:
    I_OBJECT(FontSet)

private:
    FontSet();
    ~FontSet();

public:
    static FontSet& Instance() {
        static FontSet instance;
        return instance;
    }

    void RegisterWords(const wchar_t* text);

    Character& GetCharacter(const wchar_t wchar);
    GLTexture2d::Pointer GetTexture(const wchar_t wchar);

private:
    void FlipVertically(unsigned char* data, int width, int height);

private:
    std::map<wchar_t, Character> m_Characters;
    std::map<wchar_t, GLTexture2d::Pointer> m_Textures;
};

IGAME_NAMESPACE_END

#endif //OPENIGAME_FONTSET_H
