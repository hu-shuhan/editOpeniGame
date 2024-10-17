//
// Created by Sumzeek on 7/1/2024.
//

#include "iGameFontSet.h"

IGAME_NAMESPACE_BEGIN

FontSet::FontSet() {}

FontSet::~FontSet() {
    //for (const auto& pair: m_Textures) {
    //    const GLTexture2d& texture = pair.second;
    //    auto handle = texture.getTextureHandle();
    //    handle.makeNonResident();
    //}
}

void FontSet::RegisterWords(const wchar_t* text) {
    std::string fontPath =
            std::string(ASSESTS_DIR) + "/Fonts/SourceHanSansCN-Normal.otf";

    FT_Library ft;
    if (FT_Init_FreeType(&ft))
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library"
                  << std::endl;

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face))
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    FT_Set_Pixel_Sizes(face, 0, 1024);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    int lew_w = wcslen(text);

    for (GLubyte i = 0; i < lew_w; i++) {
        auto wchar = text[i];

        // Skip registered word
        auto it = m_Characters.find(wchar);
        if (it != m_Characters.end()) { continue; };

        // Loading the glyphs for characters
        if (FT_Load_Char(face, wchar, FT_LOAD_RENDER)) {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }

        // Font size
        int font_width = face->glyph->bitmap.width;
        int font_rows = face->glyph->bitmap.rows;
        // Offset from the baseline to the left/top of the glyph
        int font_left = face->glyph->bitmap_left;
        int font_top = face->glyph->bitmap_top;
        // The distance from the origin to the next glyph origin
        int font_x = face->glyph->advance.x;
        // Flip the bitmap vertically
        auto data = face->glyph->bitmap.buffer;
        FlipVertically(data, font_width, font_rows);

        // Generate Texture
        GLTexture2d::Pointer texture = GLTexture2d::New();
        texture->Create();
        texture->Bind();
        texture->Storage(1, GL_R8, font_width, font_rows);
        texture->SubImage(0, 0, 0, font_width, font_rows, GL_RED,
                          GL_UNSIGNED_BYTE, data);
        texture->Parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        texture->Parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        texture->Parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        texture->Parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Store textures for later use
        m_Textures.insert(
                std::pair<wchar_t, GLTexture2d::Pointer>(wchar, texture));

        // Store characters for later use
        Character character = {
                texture->Handle(), igm::ivec2(font_width, font_rows),
                igm::ivec2(font_left, font_top), static_cast<uint32_t>(font_x)};
        m_Characters.insert(std::pair<wchar_t, Character>(wchar, character));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void FontSet::FlipVertically(unsigned char* data, int width, int height) {
    int rowSize = width * sizeof(unsigned char);

    for (int i = 0; i < height / 2; ++i) {
        unsigned char* currentRow = data + i * rowSize;
        unsigned char* reverseRow = data + (height - 1 - i) * rowSize;

        unsigned char* tempRow = new unsigned char[rowSize];
        std::memcpy(tempRow, currentRow, rowSize);
        std::memcpy(currentRow, reverseRow, rowSize);
        std::memcpy(reverseRow, tempRow, rowSize);
        delete[] tempRow;
    }
}


Character& FontSet::GetCharacter(const wchar_t wchar) {
    auto it = m_Characters.find(wchar);
    assert(it != m_Characters.end());
    return it->second;
}
GLTexture2d::Pointer FontSet::GetTexture(const wchar_t wchar) {
    auto it = m_Textures.find(wchar);
    assert(it != m_Textures.end());
    return it->second;
}

IGAME_NAMESPACE_END