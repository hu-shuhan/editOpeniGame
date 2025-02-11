/**
 * @class    FontManager
 * @brief    FontManager类用于管理字体资源，包括加载、注册和获取字体字符纹理。
 *
 * FontManager利用FreeType库加载字体文件，生成字符纹理并存储在内存中。
 * 它支持注册特定字符集，并提供方法获取单个字符的纹理和相关信息。
 *
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "OpenGL/GLTexture2d.h"
#include "iGameObject.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#include <cwchar>
#include <iostream>
#include <map>

IGAME_NAMESPACE_BEGIN

class FontManager : public Object {
public:
    I_OBJECT(FontManager)
    static Pointer New() { return new FontManager; }

    /**
     * @struct Character
     * @brief 存储单个字符的纹理及其相关的字体信息。
     *
     * - TextureID: 字符纹理的OpenGL ID。
     * - Size: 字符纹理的大小（宽度和高度）。
     * - Bearing: 从基线到字符左上角的偏移量。
     * - Advance: 从当前字符原点到下一个字符原点的水平偏移量。
     */
    struct Character {
        GLuint TextureID;   ///< 字符纹理的OpenGL ID。
        igm::ivec2 Size;    ///< 字符纹理的大小（宽度和高度）。
        igm::ivec2 Bearing; ///< 从基线到字符左上角的偏移量。
        GLuint Advance; ///< 从当前字符原点到下一个字符原点的水平偏移量。
    };

    /**
     * @brief 注册一组字符到FontManager中。
     * @param text 要注册的宽字符文本。
     */
    void RegisterWords(const wchar_t* text);

    /**
     * @brief 获取特定宽字符的字体信息。
     * @param wchar 要查询的宽字符。
     * @return 返回该字符对应的Character结构体。
     */
    Character& GetCharacter(wchar_t wchar);

    /**
     * @brief 获取特定宽字符的纹理。
     * @param wchar 要查询的宽字符。
     * @return 返回该字符对应的GLTexture2d纹理。
     */
    SmartPointer<GLTexture2d> GetTexture(wchar_t wchar);

protected:
    FontManager();
    ~FontManager() override;

    /**
     * @brief 垂直翻转字符纹理数据。
     * @param data 字符纹理的原始数据指针。
     * @param width 纹理宽度。
     * @param height 纹理高度。
     */
    static void FlipVertically(unsigned char* data, int width, int height);

    std::map<wchar_t, Character> m_Characters;
    std::map<wchar_t, SmartPointer<GLTexture2d>> m_Textures;
};

IGAME_NAMESPACE_END
