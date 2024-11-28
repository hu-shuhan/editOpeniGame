//
// Created by Sumzeek on 7/1/2024.
//

#pragma once

#include "OpenGL/GLTexture2d.h"
#include "iGameObject.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#include <cwchar>
#include <iostream>
#include <map>

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

    static FontSet& Instance() {
        static FontSet instance;
        return instance;
    }

    void RegisterWords(const wchar_t* text);

    Character& GetCharacter(wchar_t wchar);
    GLTexture2d::Pointer GetTexture(wchar_t wchar);

protected:
    FontSet();
    ~FontSet() override;

    std::map<wchar_t, Character> m_Characters;
    std::map<wchar_t, GLTexture2d::Pointer> m_Textures;

    static void FlipVertically(unsigned char* data, int width, int height);
};

IGAME_NAMESPACE_END
