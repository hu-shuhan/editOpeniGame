//
// Created by m_ky on 2024/10/6.
//

/**
 * @class   igQtOpenGLWidgetManager
 * @brief   igQtOpenGLWidgetManager's brief
 */


#include <IQCore/igQtOpenGLWidgetManager.h>

igQtOpenGLManager *igQtOpenGLManager::Instance() {
    static igQtOpenGLManager manager;
    return &manager;
}

void igQtOpenGLManager::setQtRenderWidget(igQtRenderWidget *widgetPtr) {
    m_RenderWidget = widgetPtr;
}

igQtRenderWidget *igQtOpenGLManager::getRenderWidget() {
    return m_RenderWidget;
}
