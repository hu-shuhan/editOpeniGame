/**
 * @class   igQtOpenGLWidgetManager
 * @brief   igQtOpenGLWidgetManager's brief
 */
#pragma once

#include <IQCore/igQtExportModule.h>
#include <IQWidgets/igQtRenderWidget.h>

class igQtOpenGLManager{
public:
    static igQtOpenGLManager* Instance();

    void setQtRenderWidget(igQtRenderWidget* widgetPtr);

    igQtRenderWidget* getRenderWidget();
protected:
    igQtRenderWidget* m_RenderWidget{nullptr};
};