/**
 * @class   iGameWindowToImageFilter
 * @brief   Transfer Render Window's FrameBuffer into s
 */

#pragma once

#include "GLFrameBuffer.h"
#include <iGameFilter.h>
#include <iGameType.h>

IGAME_NAMESPACE_BEGIN
class RenderWindow;
class WindowToImageFilter : public Filter {
public:
    I_OBJECT(WindowToImageFilter)

    static Pointer New() { return new WindowToImageFilter; }
    void SetInputWindow(RenderWindow* renderWindowPtr);
    void SetInputBufferType(GLFramebuffer::Type type);

public:
    std::vector<uint8_t> GetOutputData();

protected:
    RenderWindow* m_RenderWindow{nullptr};
    GLFramebuffer::Type m_FrameBufferType{GLFramebuffer::Type::RGBA};

protected:
    WindowToImageFilter() = default;
    ~WindowToImageFilter() = default;
};
IGAME_NAMESPACE_END
