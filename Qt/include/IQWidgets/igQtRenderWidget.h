/**
 * @class   iGameQtGLFWWindow
 * @brief   Provides Qt window and context support  for external renderers
 */

#pragma once

#ifdef __APPLE__
#define __gl3_h_
#define __glext_h_
#define __glext3_h_
#endif

#include "iGameScene.h"
#include <IQCore/igQtExportModule.h>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLWidget>


class IG_QT_MODULE_EXPORT igQtRenderWidget : public QOpenGLWidget {
  Q_OBJECT
public:
  igQtRenderWidget(QWidget *parent = nullptr);
  ~igQtRenderWidget() override;
  static igQtRenderWidget* Instance(){
      static igQtRenderWidget instance;
      return &instance;
  }

    iGame::Scene *GetScene();

  void AddDataObject(iGame::SmartPointer<iGame::DataObject> obj);
  void ChangeInteractor(iGame::SmartPointer<iGame::Interactor> it);
  void ChangeInteractorStyle(IGenum style);
  void update() { QOpenGLWidget::update(); }

    iGame::Interactor* getInteractor();

  protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  iGame::SmartPointer<iGame::Scene> m_Scene;
  iGame::SmartPointer<iGame::Interactor> m_Interactor;
};
