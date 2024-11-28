/**
 * @class   iGameWindowToImageFilter
 * @brief   Transfer Render Window's FrameBuffer into s
 */

#pragma once

#include <iGameFilter.h>
#include <iGameType.h>
IGAME_NAMESPACE_BEGIN
class RenderWindow;
class WindowToImageFilter : public Filter{
public:
    I_OBJECT(WindowToImageFilter)

    static Pointer New(){return new WindowToImageFilter;}
    void SetInputWindow(RenderWindow* renderWindowPtr);
    void SetInputBufferType(FrameBufferType type);

public:
    std::vector<uint8_t> GetOutputData();

protected:
    RenderWindow* m_RenderWindow {nullptr};
    FrameBufferType m_FrameBufferType {RGBA};
protected:
    WindowToImageFilter() = default;
    ~WindowToImageFilter() = default;
};
IGAME_NAMESPACE_END
