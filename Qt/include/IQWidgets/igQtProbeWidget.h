// ============================================================================
// igQtProbeWidget — Probe 探测（点定位 + 插值）交互面板
//
// 仿“查找数据”右侧 DockWidget 形态：
//   - 参数：Center(X/Y/Z)、Radius、点数 N（默认 1）、容差（留空自动）；
//   - 打开窗口/切换模型时按模型包围盒重置参数，并立即画线框球（Painter3D，
//     不进模型树）；修改 Center / Radius 时实时重画球；
//   - 点击算法子菜单“探测 (probe)”打开窗口时，同时创建一个空点集
//     “Probe 查询点”挂到模型树（不切换当前模型），该点集作为 ProbeFilter
//     的输入 1 与输出 0，之后一直复用；
//   - 点击[探测]：原地重生成球体内随机查询点 → 画线框球（Painter3D，不进模型树）
//     → 执行 ProbeFilter → 插值属性 + ValidPointMask 原地写回查询点集 → 刷新视图，
//     并在窗口“查询结果”表格中展示点数据属性（仿“查找数据”）。
// ============================================================================
#pragma once

#include <IQCore/igQtExportModule.h>
#include <QWidget>
#include <functional>

#include <iGamePointSet.h>
#include "igm/igm.h"

class QCloseEvent;
class QEvent;
class QShowEvent;
class QHideEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace iGame {
class Scene;
}

class igQtRenderWidget;

class igQtModelDialogWidget;

class IG_QT_MODULE_EXPORT igQtProbeWidget : public QWidget {
    Q_OBJECT
public:
    explicit igQtProbeWidget(QWidget* parent = nullptr);
    ~igQtProbeWidget() override;

    // 关联场景获取回调/模型树/渲染刷新回调（由主窗口创建时传入）。
    // 场景在 OpenGL 初始化后才创建，因此这里传“取场景的回调”，使用时实时获取。
    void setContext(std::function<iGame::Scene*()> sceneGetter,
                    igQtModelDialogWidget* modelTree,
                    std::function<void()> requestRender);

    // 打开窗口或切换模型时调用：按当前模型重置默认参数
    void refreshFromCurrentModel();

    // 关联渲染视图：用于事件过滤（拖球交互）与光标反馈。由主窗口创建后调用。
    void setRenderWidget(igQtRenderWidget* renderWidget);

    // 打开窗口时调用：首次创建一个空点集并挂到模型树（不切换当前模型），
    // 该点集同时作为 ProbeFilter 的输入 1 与输出 0，之后一直复用。
    void ensureQueryPointSet();

protected:
    void closeEvent(QCloseEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onProbeClicked();
    void onSphereParamsEdited();

private:
    void buildUi();
    void initConnections();
    iGame::Scene* currentScene() const;
    bool parseParams(iGame::Point& center, float& radius, int& count,
                     double& tolerance, bool& autoTolerance) const;
    void drawSphere(const iGame::Point& center, float radius);
    void clearSphere();
    void drawCenterCross(const iGame::Point& center);
    void clearCenterCross();
    void clearOverlays();
    void updateResultTable();

    // ---- 球体拖拽交互（事件过滤器，仅在面板显示且球存在时生效）----
    enum class DragMode { None, MoveCenter, ResizeRadius };
    bool isPointOnSphereWireframe(const QPoint& pos, float tolerance) const;
    bool isPointOnSphereSurface(const QPoint& pos) const;
    bool beginDrag(const QPoint& pos, DragMode mode);
    void updateDrag(const QPoint& pos);
    void endDrag();
    void updateHoverCursor(const QPoint& pos);
    void resetDragState();
    void applySphereParams(const iGame::Point& center, float radius);
    igm::vec3 unproject(const QPoint& pos, float ndcZ) const;

    QLineEdit* m_centerX{nullptr};
    QLineEdit* m_centerY{nullptr};
    QLineEdit* m_centerZ{nullptr};
    QLineEdit* m_radius{nullptr};
    QLineEdit* m_count{nullptr};
    QLineEdit* m_tolerance{nullptr};
    QPushButton* m_probeButton{nullptr};
    QLabel* m_statusLabel{nullptr};
    QTableWidget* m_resultTable{nullptr};

    std::function<iGame::Scene*()> m_sceneGetter;
    igQtModelDialogWidget* m_modelTree{nullptr};
    std::function<void()> m_requestRender;
    bool m_updatingParams{false};
    float m_crossHalfSize{0.0f};

    iGame::PointSet::Pointer m_queryPoints;
    unsigned int m_sphereHandle{0};
    unsigned int m_crossHandle[3]{0, 0, 0};

    igQtRenderWidget* m_renderWidget{nullptr};
    bool m_filterInstalled{false};

    DragMode m_dragMode{DragMode::None};
    bool m_dragging{false};
    QPoint m_lastDragPos;
    iGame::Point m_dragCenter;
    float m_dragRadius{0.0f};
    float m_centerNDCZ{0.0f};
    igm::mat4 m_dragMVP;
    igm::mat4 m_dragInvMVP;
};

