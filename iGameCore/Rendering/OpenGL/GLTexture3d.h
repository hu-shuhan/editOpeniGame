#ifndef IGAMEVIS_GLTEXTURE3D_H
#define IGAMEVIS_GLTEXTURE3D_H

#include "GLObject.h"

IGAME_NAMESPACE_BEGIN

/**
 * @class   GLTexture3d
 * @brief   3D 纹理封装，用于体绘制（体积数据采样）。
 *
 * @details
 *  compute shader 体绘制需要 sampler3D 采样体数据、image3D/image2D 写出结果，
 *  本项目此前只有 2D 系列纹理封装（GLTexture2d / GLTexture2dArray 等），
 *  因此补充该封装。
 *
 *  注意：3D 纹理需要 OpenGL 3.0+，本类只在 IGAME_OPENGL_VERSION_460 后端实现，
 *  OpenGL 3.3 / GLES2（含 Web 端）后端会记录错误并跳过，不影响其它渲染路径。
 */
class GLTexture3d : public GLObject<GLTexture3d> {
public:
    I_OBJECT(GLTexture3d);
    static Pointer New() { return new GLTexture3d; }

    // GLenum internal_format(Sized Internal Format): GL_R8, GL_R16F, GL_R32F,
    // GL_RGBA8 ...
    void Storage(unsigned mip_levels, GLenum internal_format, unsigned width,
                 unsigned height, unsigned depth) const;

    // GLenum format(Base Internal Format): GL_RED, GL_RG, GL_RGB, GL_RGBA
    // GLenum type: GL_UNSIGNED_BYTE, GL_FLOAT
    void SubImage(unsigned mip_level, unsigned xoffset, unsigned yoffset,
                  unsigned zoffset, unsigned width, unsigned height,
                  unsigned depth, GLenum format, GLenum type,
                  const void* pixels);

    // GLenum pname: GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R
    // GLint param: GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE,
    // GL_CLAMP_TO_BORDER
    // -------------------------------------------------------------------------
    // GLenum pname: GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER
    // GLint param: GL_NEAREST, GL_LINEAR
    // -------------------------------------------------------------------------
    // 采样体数据时建议 WRAP_* 使用 GL_CLAMP_TO_EDGE、FILTER 使用 GL_LINEAR
    // （三线性插值）。
    void Parameteri(GLenum pname, GLint param);

    void GenerateMipmap();

    // GLenum texture: GL_TEXTURE1 - GL_TEXTURE15
    // GL_TEXTURE0 is reserved to prevent other binding operations from being
    // performed after a texture unit is activated.
    void Active(GLenum texture);

    void Bind() const;
    void Release() const;

    // 绑定为 shader 中的 image（imageStore / imageLoad 使用）
    void BindImage(unsigned int binding_index, unsigned int mip_level,
                   bool layered, int layer, GLenum access, GLenum format);

protected:
    GLTexture3d();
    ~GLTexture3d() override;

    friend class GLObject<GLTexture3d>;
    static void CreateHandle(GLsizei count, GLuint* handles);
    static void DestroyHandle(GLsizei count, GLuint* handles);
};

IGAME_NAMESPACE_END

#endif // IGAMEVIS_GLTEXTURE3D_H
