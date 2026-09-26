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

  // GUI-thread only. Request a normal repaint and acknowledge its Qt swap.
  // Scene's frame pacing and interaction LOD are unchanged: this notification
  // does not certify a new full-resolution frame or GPU completion.
  // Hidden/not-exposed windows may never swap; the caller owns the timeout.
  void RequestCompletedFrame(quint64 requestId);
  // Cancels only the matching request, without emitting CompletedFrame.
  void CancelCompletedFrame(quint64 requestId);

signals:
  void CompletedFrame(quint64 requestId, bool success, const QString& detail);
  // Emitted synchronously with this widget's GL context current, before its
  // Scene/context are released. External owners must release cached GL objects.
  void ContextAboutToBeReleased();

public:

    iGame::Interactor* getInteractor();

  protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  igm::vec3 GetWorldPositionFromDepth(const QPoint& screenPos, float depth);   

  iGame::SmartPointer<iGame::Scene> m_Scene;
  iGame::SmartPointer<iGame::Interactor> m_Interactor;

private:
  void CompleteRequestedFrame(bool success, const QString& detail);
  void OnFrameSwapped();

  bool m_CompletedFrameRequestPending = false;
  bool m_CompletedFrameAwaitingSwap = false;
  quint64 m_CompletedFrameRequestId = 0;
  QString m_CompletedFrameDetail;
};
