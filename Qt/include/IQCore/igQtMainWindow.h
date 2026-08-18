/**
 * @class   igQtMainWindow
 * @brief   igQtMainWindow's brief
 */

#ifndef IGAMEVIS_IGQTMAINWINDOW_H
#define IGAMEVIS_IGQTMAINWINDOW_H

#define QT_NO_OPENGL
#include <ui_iGameQtMainWindow.h>
#if __linux__
#include <QTabWidget>
#include <QtGui>
#else
#include <QtGUI/QtGui>
#include <QtWidgets/Qtabwidget.h>
#endif
#include <IQCore/igQtExportModule.h>
#include <QtWidgets/QMainWindow>
#include <QResizeEvent>
#include <QShowEvent>
#include <QRect>
#include <QTimer>
#include <array>
#undef QT_NO_OPENGL

class igQtModelDrawWidget;
class igQtFileLoader;
class igQtColorManagerWidget;
class igQtFilterDialogDockWidget;
class igQtSliceWidget;
class igQtProgressBarWidget;
class igQtModelDialogWidget;
class igQtModelClipWidget;
class igQtDeformationWidget;
class igQtAiChatWidget;
class igQtCommandManager;
class QFontMetrics;
class igQtChromeFramelessDialog;
class igQtPartFocusWidget;

class IG_QT_MODULE_EXPORT igQtMainWindow : public QMainWindow {
    Q_OBJECT
public:
    /// 左侧「工具」Tab 面板项（扩展：追加枚举值 + cpp 中 switch + 菜单/工具栏连接）
    enum class LeftToolPanelId : int {
        Scalar = 0,
        Vector,
        Tensor,
        Flow,
        ContourExtract,
        Slice,
        Deformation,
        Selection,
        VariableDensity,
        DataChange,
        Count
    };

    igQtMainWindow(QWidget* parent = Q_NULLPTR);
    ~igQtMainWindow() override;

public:
    void initAllUnDefinedComponents();
    void initToolbarComponent();
    void initAllComponents();
    void initAllDockWidgetConnectWithAction();
    void initAllMySignalConnections();
    void initAllFilters();
    void initAllSources();
    void initAllInteractor();
    void initArgs(const QStringList& args);
    void updateVortexMetricsLabelPos();

    /** 将已登记的面板迁入左侧 QTabWidget 并显示（已存在则仅切换 Tab） */
    void openLeftToolPanel(LeftToolPanelId id);
    /** 从左侧 Tab 移除并还原到原 QDockWidget（供可勾选动作关闭等） */
    void closeLeftToolPanel(LeftToolPanelId id);

public:
    igQtModelDrawWidget* rendererWidget;
    igQtFileLoader* fileLoader;
    igQtModelDialogWidget* modelTreeWidget;

    igQtColorManagerWidget* ColorManagerWidget;
    igQtFilterDialogDockWidget* filterDialogDockWidget;
    QDockWidget* SliceDockWidget;
    QDockWidget* ContourDockWidget;
    igQtModelClipWidget* SliceWidget;
    QDockWidget* DeformationDockWidget;
    igQtDeformationWidget* DeformationWidget;

    igQtProgressBarWidget* progressBarWidget;
    QComboBox* viewStyleCombox;
    QComboBox* attributeViewIndexCombox;
    QComboBox* attributeViewDimCombox;
    
    // AI Chat DockWidget
    QDockWidget* aiChatDockWidget;
    igQtAiChatWidget* aiChatWidget;

    // Command Manager for MCP Server (端口 12345)
    igQtCommandManager* commandManager;

    // 零件聚焦弹窗
    igQtChromeFramelessDialog* partFocusDialog{nullptr};
    igQtPartFocusWidget* partFocusWidget{nullptr};

private slots:
    void updateRecentFilePaths();
    void updateColorBarShow();

    //void ChangeViewStyle();
    //void ChangeScalarView();
    //void ChangeScalarViewDim();
    //void updateViewStyleAndCloudPicture();
    //void updateCurrentDataObject();
    //void updateCurrentSceneWidget();

    void UpdateRenderingWidget();
    //void changePointSelectionInteractor();
    //void changePointsSelectionInteractor();
    //void changeFaceSelectionInteractor();
    //void changeFacesSelectionInteractor();
    void UpdateIcons();
    QString LoadExternalFonts();


private:
    Ui::MainWindow* ui;
    QLabel* vortexMetricsLabel = nullptr;
    // 自定义标题栏相关
    QWidget* m_titleBar = nullptr;
    QLabel* m_titleLabel = nullptr;
    QPushButton* m_btnMinimize = nullptr;
    QPushButton* m_btnMaximize = nullptr;
    QPushButton* m_btnClose = nullptr;
    bool m_titleBarDragging = false;
    QPoint m_dragOffset;
    bool m_isMinimizing = false;
    bool m_isRestoringFromMaximized = false;
    QRect m_geometryBeforeMinimize;
    QRect m_normalGeometry;

    // 左侧工具 Tab（按需添加；下方 Properties 常驻）
    QDockWidget* m_leftFieldDock = nullptr;
    QTabWidget* m_leftFieldTabs = nullptr;
    std::array<int, static_cast<size_t>(LeftToolPanelId::Count)> m_leftToolTabByPanel{{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

    void relocateContentToLeftTab(QDockWidget* shell, QWidget* inner, const QString& title, LeftToolPanelId id,
                                  bool centerFlowField);
    QWidget* wrapContentInScrollArea(QWidget* content, QWidget* parent, bool centerFlowField);
    QDockWidget* shellDockForLeftPanel(LeftToolPanelId id) const;
    void onLeftToolTabCloseRequested(int index);
    /** 工具面板与 Properties 垂直比例（需在工具 Dock 已 show 后调用） */
    void applyLeftToolStackVerticalSplit();

    /** 与菜单「算法处理 / 特征提取」等一致：无边框 QMessageBox + 暗色圆角边框。 */
    void showDarkFramelessMessage(const QString& title, const QString& text, bool useInformationIcon = false);

    void rebuildActionsAsTwoRowWidget(QToolBar* toolbar, const QList<QAction*>& targetActions, int columns,
                                      QAction* insertBefore = nullptr);
    void addToolbarTitle(QToolBar* toolbar, const QString& title, int iconSizePx);
    void relayoutToolbarWrappers();
    void initCustomTitleBar();

    // ---- 工具栏单排适配（宽度拟合 + 文字自动换行）----
    /** 按指定 iconSize 重建 3×2 轴网格与 4 组「按钮行 + 标题」容器 */
    void rebuildToolbarRow(int iconSize);
    /** 删除旧的 wrapper_* 工具栏（连带其容器） */
    void removeToolbarWrappers();
    /** 测量当前 4 组 wrapper 的实际总宽度（含组间距） */
    int measureToolbarRowWidth() const;
    /** 文字超宽时断成最多两行，第二行仍超宽则省略号收尾 */
    QString wrapToolbarButtonText(const QString& text, int maxWidth, const QFontMetrics& fm) const;

    int m_currentToolbarIconSize = 40;
    bool m_toolbarRebuilding = false;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void changeEvent(QEvent* event) override;
    int streamTreeIndex = -1;

private:
    // 响应式 toolbar 布局：把耗时的重排合并（避免每 1px 拖动都触发全量刷新）
    QTimer* m_ResizeDebounceTimer{nullptr};
    bool m_ResponsiveHooked{false};
    void hookResponsiveEvents();
    // 依当前窗口宽度/屏幕 DPI 挑选最大能一行装下的 iconSize，然后应用到全部工具栏
    void applyResponsiveToolbarLayout();
private:
    void minimizeWithAnimation();
    void toggleMaximizeRestore();
    void updateMaximizeButtonIcon();
};


#endif //IGAMEVIS_IGQTMAINWINDOW_H
