//
// Created by m_ky on 2024/11/27.
//

/**
 * @class   iGameWindowToImageFilter
 * @brief   iGameWindowToImageFilter's brief
 */

#include "iGameWindowToImageFilter.h"
#include <RenderWindow/iGameRenderWindow.h>
#include <iGameScene.h>
IGAME_NAMESPACE_BEGIN

void WindowToImageFilter::SetInputWindow(RenderWindow *renderWindowPtr) {
    m_RenderWindow = renderWindowPtr;
}

void WindowToImageFilter::SetInputBufferType(FrameBufferType type) {
    m_FrameBufferType = type;
}

std::vector<uint8_t> WindowToImageFilter::GetOutputData() {
    if(m_RenderWindow == nullptr) {
        std::cout << "Unset the Render window\n";
        return {};
    }
    m_RenderWindow->GetScene()->GetCamera()->GetDevicePixelRatio();
    auto wh = m_RenderWindow->GetScene()->GetCamera()->GetViewPort();
    return m_RenderWindow->GetScene()->CaptureScreen(0, 0,
                                                     wh[0] / m_RenderWindow->GetScene()->GetCamera()->GetDevicePixelRatio(),
                                                     wh[1] / m_RenderWindow->GetScene()->GetCamera()->GetDevicePixelRatio(), m_FrameBufferType);
}

IGAME_NAMESPACE_END
