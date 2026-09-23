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
  static void setGlobalStyleMode(int mode);
  static int globalStyleMode();
  static bool globalLightBackground();
  static QString adaptQssToLightPalette(const QString& qss);

  enum class UiRole {
      PanelBg,
      PanelBg2,
      CardBg,
      Border,
      BorderStrong,
      Text,
      TextDim,
      TextStrong,
      Accent,
      HoverBg,
      SelectionBg
  };
  static QColor uiRole(UiRole role);
  static QString uiRoleCss(UiRole role);
  static QString themeRemapQss(const QString& baseQss);
  void applyThemeBackground();
  void setCornerCover(int radius, const QColor& coverColor);

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
  static int s_styleMode;

  int m_cornerCoverRadius{0};
  QColor m_cornerCoverColor{0x1E, 0x1E, 0x1E};

private:
  void CompleteRequestedFrame(bool success, const QString& detail);
  void OnFrameSwapped();

  bool m_CompletedFrameRequestPending = false;
  bool m_CompletedFrameAwaitingSwap = false;
  quint64 m_CompletedFrameRequestId = 0;
  QString m_CompletedFrameDetail;
};

struct igQtPanelTheme {
    static void attach(QWidget* panel) {
        if (!panel) return;
        if (!panel->property("igPanelBaseQss").toString().isEmpty()) {
            refresh(panel);
            return;
        }
        panel->setProperty("igPanelBaseQss", panel->styleSheet());
        refresh(panel);
    }
    static void refresh(QWidget* panel) {
        if (!panel) return;
        const QString base = panel->property("igPanelBaseQss").toString();
        if (base.isEmpty()) return;
        const QString mapped = igQtRenderWidget::themeRemapQss(base);
        if (panel->styleSheet() == mapped) return;
        panel->setStyleSheet(mapped);
    }

    static void attachDeep(QWidget* root) {
        const QList<QWidget*> all = styledWidgets(root);
        for (QWidget* w : all) { attach(w); }
    }
    static void refreshDeep(QWidget* root) {
        const QList<QWidget*> all = styledWidgets(root);
        for (QWidget* w : all) { refresh(w); }
    }

private:
    static QList<QWidget*> styledWidgets(QWidget* root) {
        QList<QWidget*> out;
        if (!root) return out;
        out << root;
        const QList<QWidget*> kids = root->findChildren<QWidget*>();
        for (QWidget* w : kids) {
            if (!w->styleSheet().isEmpty() || !w->property("igPanelBaseQss").toString().isEmpty()) {
                out << w;
            }
        }
        return out;
    }
};
