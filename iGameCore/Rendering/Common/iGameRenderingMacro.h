#pragma once

#define MSAA
#define MAX_FRAMEBUFFER_WIDTH 3840
#define MAX_FRAMEBUFFER_HEIGHT 2160

/*
    GL_NO_ERROR: (0)
    GL_INVALID_ENUM: (1280)
    GL_INVALID_VALUE: (1281)
    GL_INVALID_OPERATION: (1282)
    GL_STACK_OVERFLOW: (1283)
    GL_STACK_UNDERFLOW: (1284)
    GL_OUT_OF_MEMORY: (1285)
    GL_INVALID_FRAMEBUFFER_OPERATION: (1286)
    GL_CONTEXT_LOST: (1287)
    */

#ifdef IGAME_OPENGL_VERSION_460
#define GLCheckError()                                                         \
    {                                                                          \
        GLenum err;                                                            \
        while ((err = glGetError()) != GL_NO_ERROR) {                          \
            std::string error;                                                 \
            switch (err) {                                                     \
                case GL_INVALID_ENUM:                                          \
                    error = "INVALID_ENUM";                                    \
                    break;                                                     \
                case GL_INVALID_VALUE:                                         \
                    error = "INVALID_VALUE";                                   \
                    break;                                                     \
                case GL_INVALID_OPERATION:                                     \
                    error = "INVALID_OPERATION";                               \
                    break;                                                     \
                case GL_STACK_OVERFLOW:                                        \
                    error = "STACK_OVERFLOW";                                  \
                    break;                                                     \
                case GL_STACK_UNDERFLOW:                                       \
                    error = "STACK_UNDERFLOW";                                 \
                    break;                                                     \
                case GL_OUT_OF_MEMORY:                                         \
                    error = "OUT_OF_MEMORY";                                   \
                    break;                                                     \
                case GL_INVALID_FRAMEBUFFER_OPERATION:                         \
                    error = "INVALID_FRAMEBUFFER_OPERATION";                   \
                    break;                                                     \
                default:                                                       \
                    error = "UNKNOWN_ERROR";                                   \
                    break;                                                     \
            }                                                                  \
            std::cerr << "OpenGL error: " << error << " (" << err << ")"       \
                      << " in file " << __FILE__ << " at line " << __LINE__    \
                      << std::endl;                                            \
        }                                                                      \
    }
#else
#define GLCheckError()                                                         \
    {                                                                          \
        GLenum err;                                                            \
        while ((err = glGetError()) != GL_NO_ERROR) {                          \
            std::string error;                                                 \
            switch (err) {                                                     \
                case GL_INVALID_ENUM:                                          \
                    error = "INVALID_ENUM";                                    \
                    break;                                                     \
                case GL_INVALID_VALUE:                                         \
                    error = "INVALID_VALUE";                                   \
                    break;                                                     \
                case GL_INVALID_OPERATION:                                     \
                    error = "INVALID_OPERATION";                               \
                    break;                                                     \
                case GL_OUT_OF_MEMORY:                                         \
                    error = "OUT_OF_MEMORY";                                   \
                    break;                                                     \
                case GL_INVALID_FRAMEBUFFER_OPERATION:                         \
                    error = "INVALID_FRAMEBUFFER_OPERATION";                   \
                    break;                                                     \
                default:                                                       \
                    error = "UNKNOWN_ERROR";                                   \
                    break;                                                     \
            }                                                                  \
            std::cerr << "OpenGL error: " << error << " (" << err << ")"       \
                      << " in file " << __FILE__ << " at line " << __LINE__    \
                      << std::endl;                                            \
        }                                                                      \
    }
#endif
