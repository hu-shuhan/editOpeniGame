#include "IQCore/igQtMainWindow.h"
//
// Created by m_ky on 2024/4/10.
//

#include "MeshMetrics/iGameVolumeMeshMetricsFilter.h"
#include "Deformation/iGameStressDeformationFilterCode.h"

#include "DataProcessing/Tests/iGameGradient.h"
#include "DataProcessing/Tests/iGameSimplification2.h"
#include "DataProcessing/Tests/iGameSurfaceSimplification.h"
#include "DataProcessing/Tests/meshsimplifier/meshsimplifier.h"
#include "DataProcessing/Tests/simplifier.h"
#include "DataProcessing/iGameMeshSimplificationFilter.h"
#include "DataProcessing/iGameMeshSimplificationFilterPro.h"
#include "DataProcessing/iGameMeshTriangulationFilter.h"
#include "DataProcessing/Simplification/iGameMeshSaliency.h"
#include "DataProcessing/Simplification/iGameMeshSimplificationWithAttributes.h"

#include "Convert/iGameConvertPolyhedralCellsFilter.h"
#include "Convert/iGameConvertToCellDataFilter.h"
#include "Convert/iGameConvertToLagrangeUnstructuredMeshFilter.h"
#include "Convert/iGameConvertToPointCloudFilter.h"
#include "Convert/iGameConvertToPointDataFilter.h"
#include "Convert/iGameConvertToSurfaceMeshFilter.h"
#include "Convert/iGameConvertToVolumeMeshFilter.h"

#include "Interactor/iGameInteractor.h"

#include "Tests/iGameARAPTest.h"

#include "iGameFileIO.h"
#include "iGameFilterIncludes.h"
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQComponents/igQtProgressBarWidget.h>
#include <IQCore/igQtFileLoader.h>
#include <IQCore/igQtOpenGLWidgetManager.h>
#include <IQWidgets/ColorManager/igQtColorManagerWidget.h>
#include <IQWidgets/igQtAiChat/igQtAiChatWidget.h>
#include <IQWidgets/igQtAiChat/igQtCommandManager.h>
#include <IQWidgets/igQtCharts.h>
#include <IQWidgets/igQtDeformationWidget.h>
#include <IQWidgets/igQtModelClipWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQWidgets/igQtModelInformationWidget.h>
#include <IQWidgets/igQtParallelCoordinatesWidget.h>
#include <IQWidgets/igQtTensorWidget.h>
#include <IQWidgets/igQtVariableCorrelationWidget.h>
#include <IQComponents/Dialog/igQtBoxSettingDialog.h>
#include <IQComponents/Dialog/igQtChromeFramelessDialog.h>
#include <iGameBlockMapping.h>
#include <P3SAM/iGameP3SAMSegmenter.h>
#include <QDebug>
#include <QLabel>
#include <QMessageBox>
#include <QSplitter>
#include <QStyleFactory>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QPushButton>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <Sources/iGameLineTypePointsSourceFilter.h>
#include <Tests/iGameVolumeMeshFilterTest.h>
#include <VolumeMeshAlgorithm/iGameVolumeMeshClipper.h>
#include <fcntl.h>
#include <iGameBoxStyle.h>
#include <iGameCtxPresObjData.h>
#include <iGameDataSource.h>
#include <iGameDynamicBox.h>
#include <iGamePointFinder.h>
#include <iGameSelectionParameter.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <include/IQComponents/Dialog/igQtChangeBackGroundDialog.h>
#include <include/IQComponents/Dialog/igQtMeshCodecDialog.h>
#include <include/IQComponents/Dialog/igQtDarkFramelessMessage.h>
#include <include/IQComponents/Dialog/igQtScreenShotOptionDialog.h>
#include <BuildAdjacencyRelation/iGameBuildAdjacencyRelationFilter.h>
#include <meshoptimizer.h>
#include <stdio.h>

#include <QDebug>
#include <QMessageBox>
#include <QSplitter>
#include <QPointer>
#include <QTimer>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QApplication>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QStyle>
#include <QFontMetrics>


#include "ui_igQtVariableCorrelationWidget.h"

namespace {
struct ToolbarSpacingMetrics {
    int btnGap;
    int edgeMargin;
    int buttonPadding;
    int verticalGap;
    int bottomMargin;
    int groupGap;
    int rowGap;
};

ToolbarSpacingMetrics metricsForIconSize(int iconSize) {
    iconSize = qMax(24, iconSize);
    ToolbarSpacingMetrics metrics;
    metrics.btnGap = qMax(2, iconSize / 12);
    metrics.edgeMargin = qMax(1, iconSize / 22);
    metrics.buttonPadding = qMax(1, iconSize / 20);
    metrics.verticalGap = qMax(4, iconSize / 5);
    metrics.bottomMargin = qMax(8, iconSize / 3);
    metrics.groupGap = qMax(6, iconSize / 6);
    metrics.rowGap = qMax(2, iconSize / 8);
    return metrics;
}

// 工具栏标题（每个 toolbar 下方那行小灰字，"文件与输出" 之类）字号 → iconSize 的映射；
// 让文字大小随着窗口/屏幕响应式变化。
int titlePointSizeForIcon(int iconSize) {
    return qBound(8, iconSize / 4, 14); // 32->8, 40->10, 46->11, 50->12, 52->13
}

int resolveToolbarIconSize(int availableWidth, qreal dpiScale) {
    int iconSize = 52;
    if (availableWidth <= 1366) {
        iconSize = 32;
    } else if (availableWidth <= 1600) {
        iconSize = 36;
    } else if (availableWidth <= 1920) {
        iconSize = 40;
    } else if (availableWidth <= 2560) {
        iconSize = 46;
    } else if (availableWidth <= 2880) {
        iconSize = 50;
    }

    const qreal scale = qMax<qreal>(1.0, dpiScale);
    return qBound(24, static_cast<int>(qRound(static_cast<qreal>(iconSize) / scale)), 52);
}

int resolveToolbarIconSizeForWidget(const QWidget* widget) {
    QScreen* screen = widget ? widget->screen() : nullptr;
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    const qreal dpiScale = screen ? screen->devicePixelRatio() : 1.0;
    // 响应式：优先按窗口自身可视宽度分档，而不是整个屏幕；
    // 窗口还没显示（width==0）或过窄时才回退到屏幕宽度。
    int refWidth = widget ? widget->width() : 0;
    if (refWidth < 400) {
        refWidth = screen ? screen->availableGeometry().width() : 1920;
    }
    return resolveToolbarIconSize(refWidth, dpiScale);
}

const char* kGlobalSpinBoxDarkQss = R"(
QSpinBox, QDoubleSpinBox {
    background-color: #252526;
    color: #D4D4D4;
    border: 1px solid #3C3C3C;
    border-radius: 4px;
    padding: 4px 24px 4px 8px;
    selection-background-color: #094771;
}
QSpinBox:hover, QDoubleSpinBox:hover {
    border: 1px solid #4A4A4A;
}
QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid #0E639C;
}
QSpinBox::up-button, QDoubleSpinBox::up-button {
    subcontrol-origin: border;
    subcontrol-position: top right;
    width: 18px;
    border-left: 1px solid #3C3C3C;
    border-top-right-radius: 4px;
    background-color: #2D2D30;
}
QSpinBox::down-button, QDoubleSpinBox::down-button {
    subcontrol-origin: border;
    subcontrol-position: bottom right;
    width: 18px;
    border-left: 1px solid #3C3C3C;
    border-top: 1px solid #3C3C3C;
    border-bottom-right-radius: 4px;
    background-color: #2D2D30;
}
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
    background-color: #3A3A3D;
}
QSpinBox::up-button:pressed, QDoubleSpinBox::up-button:pressed,
QSpinBox::down-button:pressed, QDoubleSpinBox::down-button:pressed {
    background-color: #45454A;
}
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
    image: url(:/Ticon/Icons/spin_up_silver.svg);
    width: 9px;
    height: 9px;
}
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
    image: url(:/Ticon/Icons/spin_down_silver.svg);
    width: 9px;
    height: 9px;
}
QComboBox::drop-down {
    border-left: 1px solid #3C3C3C;
    width: 20px;
}
QComboBox::down-arrow {
    image: url(:/Ticon/Icons/spin_down_silver.svg);
    width: 10px;
    height: 10px;
}
QComboBox QAbstractItemView {
    background-color: #252526;
    color: #CCCCCC;
    border: 1px solid #3C3C3C;
    outline: 0;
    selection-background-color: #3A3A3A;
    selection-color: #FFFFFF;
}
QScrollBar:vertical {
    background-color: #1B1B1B;
    border: none;
    width: 12px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #9E9E9E, stop:0.5 #BEBEBE, stop:1 #989898);
    border: 1px solid #7C7C7C;
    border-radius: 6px;
    min-height: 20px;
}
QScrollBar::handle:vertical:hover {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #ABABAB, stop:0.5 #CBCBCB, stop:1 #A5A5A5);
}
QScrollBar::handle:vertical:pressed {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #8B8B8B, stop:0.5 #A9A9A9, stop:1 #858585);
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background-color: #242424;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar:horizontal {
    background-color: #1B1B1B;
    border: none;
    height: 12px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                stop:0 #9E9E9E, stop:0.5 #BEBEBE, stop:1 #989898);
    border: 1px solid #7C7C7C;
    border-radius: 6px;
    min-width: 20px;
}
QScrollBar::handle:horizontal:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                stop:0 #ABABAB, stop:0.5 #CBCBCB, stop:1 #A5A5A5);
}
QScrollBar::handle:horizontal:pressed {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                stop:0 #8B8B8B, stop:0.5 #A9A9A9, stop:1 #858585);
}
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    background-color: #242424;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}
)";
}

igQtMainWindow::igQtMainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    qApp->setStyleSheet(qApp->styleSheet() + QString::fromUtf8(kGlobalSpinBoxDarkQss));
    // 设置窗口标题为iGameVis
    this->setWindowTitle("iGameVis");
    // 使用无边框窗口并自定义标题栏
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    initCustomTitleBar();
    initAllUnDefinedComponents();
    UpdateIcons();
    initAllComponents();
    initAllFilters();
    initAllSources();
    initAllInteractor();
    updateRecentFilePaths();
    // 将 toolBar_4 的 +X -X +Y -Y +Z -Z 六个按钮分为两行、每行三个展示
    rebuildActionsAsTwoRowWidget(
            ui->toolBar_4,
            {
                    ui->action_setViewToPositiveX,
                    ui->action_setViewToNegativeX,
                    ui->action_setViewToPositiveY,
                    ui->action_setViewToNegativeY,
                    ui->action_setViewToPositiveZ,
                    ui->action_setViewToNegativeZ
            },
            3,
            ui->action_rotateNinetyCounterClockwise
    );
    initToolbarComponent();  // 在 rebuild 之后：用 QToolButton 行+标题替代 QToolBar，避免 QToolBar 进 layout 导致图标不渲染
    connect(modelTreeWidget, &igQtModelDialogWidget::Update, rendererWidget, &igQtRenderWidget::update);

    // 初始化命令管理器并建立与 MCP Tool Server 的连接
    commandManager = new igQtCommandManager(this);
    if (!commandManager->startConnection("localhost", 12345)) {
        qWarning() << "iGameVis 与 MCP Tool Server 连接失败！";
    }

    ThreadPool::Instance();
}

void igQtMainWindow::initCustomTitleBar() {
    if (m_titleBar) return;

    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName("CustomTitleBar");
    // 调高标题栏整体高度
    m_titleBar->setFixedHeight(72);
    // 标题栏 QSS 见 iGameQtMainWindow.ui 中 MainWindow.styleSheet（QWidget#CustomTitleBar 等）

    // 垂直布局：第一行标题栏，第二行菜单栏
    auto* mainLayout = new QVBoxLayout(m_titleBar);
    mainLayout->setContentsMargins(8, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 顶部一行：图标 + 标题 + 按钮
    QWidget* topRow = new QWidget(m_titleBar);
    auto* topLayout = new QHBoxLayout(topRow);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(4);

    // 图标
    QLabel* iconLabel = new QLabel(topRow);
    iconLabel->setFixedSize(18, 18);
    QPixmap pm = windowIcon().pixmap(18, 18);
    iconLabel->setPixmap(pm);
    iconLabel->setScaledContents(true);
    topLayout->addWidget(iconLabel);

    // 标题（样式见 .ui 中 QLabel#CustomTitleLabel）
    m_titleLabel = new QLabel(topRow);
    m_titleLabel->setObjectName(QStringLiteral("CustomTitleLabel"));
    m_titleLabel->setText(this->windowTitle());
    topLayout->addWidget(m_titleLabel, 1);

    // 按钮区域（尺寸与 Windows 标题栏按钮比例相近：较宽、易点）
    m_btnMinimize = new QPushButton(topRow);
    m_btnMinimize->setObjectName(QStringLiteral("MinimizeButton"));
    m_btnMaximize = new QPushButton(topRow);
    m_btnMaximize->setObjectName(QStringLiteral("MaximizeButton"));
    m_btnClose = new QPushButton(QStringLiteral("×"), topRow);
    m_btnClose->setObjectName(QStringLiteral("CloseButton"));

    const QSize captionBtnSize(46, 30);
    m_btnMinimize->setFixedSize(captionBtnSize);
    m_btnMaximize->setFixedSize(captionBtnSize);
    m_btnClose->setFixedSize(captionBtnSize);

    m_btnMinimize->setIcon(QIcon(QStringLiteral(":/Ticon/Icons/window_minimize_white.svg")));
    m_btnMinimize->setIconSize(QSize(12, 12));
    m_btnMaximize->setIconSize(QSize(12, 12));
    m_btnMaximize->setText(QString());
    m_btnMaximize->setFlat(true);
    m_btnMinimize->setFlat(true);
    m_btnClose->setFlat(true);

    topLayout->addWidget(m_btnMinimize, 0);
    topLayout->addWidget(m_btnMaximize, 0);
    topLayout->addWidget(m_btnClose, 0);

    // 添加顶部行到主布局
    mainLayout->addWidget(topRow, 0);

    // 第二行：原来的菜单栏整行显示
    if (ui->menuBar) {
        ui->menuBar->setParent(m_titleBar);
        ui->menuBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        mainLayout->addWidget(ui->menuBar, 0);
    }

    // 放到 QMainWindow 的菜单栏区域，相当于自定义标题栏
    this->setMenuWidget(m_titleBar);

    // 拖动事件用 eventFilter 处理（只对标题栏整体和标题文本生效，不干扰按钮点击）
    m_titleBar->installEventFilter(this);
    m_titleLabel->installEventFilter(this);

    // 按钮功能
    connect(m_btnMinimize, &QPushButton::clicked, this, [this]() {
        minimizeWithAnimation();
    });

    connect(m_btnMaximize, &QPushButton::clicked, this, [this]() {
        toggleMaximizeRestore();
    });

    connect(m_btnClose, &QPushButton::clicked, this, [this]() {
        this->close();
    });

    // 监听全局鼠标释放，防止拖动状态在某些场景下卡住
    qApp->installEventFilter(this);
    updateMaximizeButtonIcon();
}

bool igQtMainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (!m_titleBar) return QMainWindow::eventFilter(watched, event);

    // 全局兜底：只要左键释放就结束拖动，避免窗口“黏在鼠标上”
    if (m_titleBarDragging) {
        if (event->type() == QEvent::MouseButtonRelease) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_titleBarDragging = false;
            }
        } else if (event->type() == QEvent::WindowDeactivate) {
            m_titleBarDragging = false;
        }
    }

    // 按钮自身的事件交给 Qt 处理，保证 clicked() 能正常触发
    if (qobject_cast<QPushButton*>(watched)) {
        return QMainWindow::eventFilter(watched, event);
    }

    // 只对标题栏本身或标题文本处理拖动，不拦截按钮
    if (watched == m_titleBar || watched == m_titleLabel) {
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                auto* me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::LeftButton) {
                    m_titleBarDragging = true;
                    m_dragOffset = me->globalPos() - frameGeometry().topLeft();
                    return true;
                }
                break;
            }
            case QEvent::MouseMove: {
                auto* me = static_cast<QMouseEvent*>(event);
                if (m_titleBarDragging && (me->buttons() & Qt::LeftButton)) {
                    if (isMaximized()) {
                        const qreal ratioX = qBound<qreal>(0.0, static_cast<qreal>(me->pos().x()) / qMax(1, m_titleBar->width()), 1.0);
                        showNormal();
                        const int newX = me->globalPos().x() - static_cast<int>(width() * ratioX);
                        const int newY = me->globalPos().y() - m_titleBar->height() / 2;
                        m_dragOffset = me->globalPos() - QPoint(newX, newY);
                        move(newX, newY);
                        return true;
                    }
                    move(me->globalPos() - m_dragOffset);
                    return true;
                }
                if (!(me->buttons() & Qt::LeftButton)) {
                    m_titleBarDragging = false;
                }
                break;
            }
            case QEvent::MouseButtonDblClick: {
                auto* me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::LeftButton) {
                    toggleMaximizeRestore();
                    return true;
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                auto* me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::LeftButton) {
                    m_titleBarDragging = false;
                    return true;
                }
                break;
            }
            default:
                break;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void igQtMainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        updateMaximizeButtonIcon();
    }
    QMainWindow::changeEvent(event);
}

void igQtMainWindow::minimizeWithAnimation() {
    if (m_isMinimizing || isMinimized()) {
        return;
    }

    m_isMinimizing = true;
    m_geometryBeforeMinimize = geometry();

    const QRect startRect = m_geometryBeforeMinimize;
    const QPoint center = startRect.center();
    const int endW = qMax(20, startRect.width() / 8);
    const int endH = qMax(20, startRect.height() / 8);
    const QRect endRect(center.x() - endW / 2, center.y() - endH / 2, endW, endH);

    auto* anim = new QPropertyAnimation(this, "geometry");
    anim->setDuration(160);
    anim->setStartValue(startRect);
    anim->setEndValue(endRect);
    anim->setEasingCurve(QEasingCurve::InCubic);

    connect(anim, &QPropertyAnimation::finished, this, [this, anim]() {
        this->showMinimized();
        this->setGeometry(m_geometryBeforeMinimize);
        m_isMinimizing = false;
        anim->deleteLater();
    });

    anim->start();
}

void igQtMainWindow::toggleMaximizeRestore() {
    if (isMaximized()) {
        if (m_isRestoringFromMaximized) {
            return;
        }

        m_isRestoringFromMaximized = true;
        QRect targetRect = m_normalGeometry;
        if (!targetRect.isValid() || targetRect.width() < 100 || targetRect.height() < 100) {
            QRect workArea = QGuiApplication::primaryScreen()->availableGeometry();
            targetRect = QRect(workArea.x() + workArea.width() / 10,
                               workArea.y() + workArea.height() / 10,
                               workArea.width() * 8 / 10,
                               workArea.height() * 8 / 10);
        }

        QRect startRect = QGuiApplication::primaryScreen()->availableGeometry();
        if (windowHandle() && windowHandle()->screen()) {
            startRect = windowHandle()->screen()->availableGeometry();
        }

        showNormal();
        setGeometry(startRect);

        auto* anim = new QPropertyAnimation(this, "geometry");
        anim->setDuration(170);
        anim->setStartValue(startRect);
        anim->setEndValue(targetRect);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, &QPropertyAnimation::finished, this, [this, anim]() {
            m_isRestoringFromMaximized = false;
            updateMaximizeButtonIcon();
            anim->deleteLater();
        });
        anim->start();
    } else {
        m_normalGeometry = geometry();
        showMaximized();
        updateMaximizeButtonIcon();
    }
}

void igQtMainWindow::updateMaximizeButtonIcon() {
    if (!m_btnMaximize) return;
    m_btnMaximize->setIcon(QIcon(isMaximized() ? QStringLiteral(":/Ticon/Icons/window_restore_white.svg")
                                               : QStringLiteral(":/Ticon/Icons/window_maximize_white.svg")));
    m_btnMaximize->setIconSize(isMaximized() ? QSize(15, 15) : QSize(12, 12));
    m_btnMaximize->setText(QString());
}

void igQtMainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    // 立刻决定要不要换行（视觉上跟手）；同时 100ms 防抖跑一次全量重排（含 iconSize 重算）
    relayoutToolbarWrappers();
    if (m_ResizeDebounceTimer) { m_ResizeDebounceTimer->start(100); }
}

igQtMainWindow::~igQtMainWindow() {
    // 清理命令管理器
    if (commandManager) {
        commandManager->stopConnection();
        delete commandManager;
        commandManager = nullptr;
    }
}
void igQtMainWindow::initArgs(const QStringList& args) {
    int argc = args.size();
    for (int i = 1; i < argc; ++i) {
        const QString& cur_arg = args[i].toLower();
        if (cur_arg == "--filepath" && ++i < argc) {
            const QString& filePath = args[i];
            fileLoader->OpenFile(filePath.toStdString());
        }
    }
}
void igQtMainWindow::initAllUnDefinedComponents() {
    rendererWidget = new igQtModelDrawWidget(this);
    igQtOpenGLManager::Instance()->setQtRenderWidget(rendererWidget);
    //    rendererWidget->setParent(this);
    fileLoader = new igQtFileLoader(this);
    this->setCentralWidget(rendererWidget);
    this->ColorManagerWidget = new igQtColorManagerWidget;
    ColorManagerWidget->setGeometry(400, 500, 780, 1000);

    // 初始化AI聊天DockWidget
    aiChatDockWidget = new QDockWidget(this);
    aiChatDockWidget->setWindowTitle("AI聊天助手");
    aiChatWidget = new igQtAiChatWidget(aiChatDockWidget, this);
    aiChatDockWidget->setWidget(aiChatWidget);
    aiChatDockWidget->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    // AI 聊天窗口：不允许拖动/悬浮（只保留可关闭）
    aiChatDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    aiChatDockWidget->hide(); // 初始隐藏
    this->addDockWidget(Qt::RightDockWidgetArea, aiChatDockWidget);

    // 设置DockWidget的默认大小
    aiChatDockWidget->resize(400, 600);

    // 将原本右侧的 dockwidget 移到左侧（后续统一加入左侧 tab 组）
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_ScalarField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_VectorField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_FlowField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_TensorField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_ParallelCoordinatesField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_VariableCorrelationField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_VariableDensityField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_DataChangeField);
    // SelectionField 改為停靠在左側，並放在 Properties 視窗上方
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_SelectionField);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_ContextPreservingShowField);
    this->addDockWidget(Qt::RightDockWidgetArea, ui->dockWidget_SearchInfo);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_QualityDetection);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_EditMode);
    this->addDockWidget(Qt::BottomDockWidgetArea, ui->dockWidget_Animation);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_ModelList);
    this->addDockWidget(Qt::LeftDockWidgetArea, ui->dockWidget_ContourExtract);

    // 禁止所有 dock 悬浮：去掉 DockWidgetFloatable
    // 同时为了防止“拖拽标题栏就被扯成系统浮动窗”，这里也把 Movable 去掉（只保留可关闭）。
    // 如果你仍希望允许在 dock 区域内重新排列位置，可以把 DockWidgetMovable 加回去，但必须保持不包含 DockWidgetFloatable。
    ui->dockWidget_ScalarField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_VectorField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_FlowField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_TensorField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_ParallelCoordinatesField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_VariableCorrelationField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_VariableDensityField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_DataChangeField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_SelectionField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_ContextPreservingShowField->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_SearchInfo->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_QualityDetection->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_EditMode->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_Animation->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_ModelList->setFeatures(QDockWidget::DockWidgetClosable);
    ui->dockWidget_ContourExtract->setFeatures(QDockWidget::DockWidgetClosable);

    QDockWidget* dockWidget_null = new QDockWidget("", this);
    this->addDockWidget(Qt::RightDockWidgetArea, dockWidget_null);
    dockWidget_null->hide();
    ui->dockWidget_ScalarField->hide();
    ui->dockWidget_VectorField->hide();
    ui->dockWidget_FlowField->hide();
    ui->dockWidget_TensorField->hide();
    ui->dockWidget_ParallelCoordinatesField->hide();
    ui->dockWidget_VariableCorrelationField->hide();
    ui->dockWidget_VariableDensityField->hide();
    ui->dockWidget_DataChangeField->hide();
    ui->dockWidget_SelectionField->hide();
    ui->dockWidget_ContextPreservingShowField->hide();
    ui->dockWidget_SearchInfo->hide();
    ui->dockWidget_QualityDetection->hide();
    ui->dockWidget_EditMode->hide();
    ui->dockWidget_Animation->hide();
    ui->dockWidget_ModelList->hide();
    ui->dockWidget_ContourExtract->hide();
    
    // Setup default GUI layout.
    // 启用左侧区域的 tab 功能，使左侧 dockwidget 可以通过 tab 切换
    this->setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);
    this->setTabPosition(Qt::RightDockWidgetArea, QTabWidget::North);
    //this->setTabPosition(Qt::BottomDockWidgetArea, QTabWidget::North);
    // Set up the dock window corners to give the vertical docks more room.
    this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    modelTreeWidget = new igQtModelDialogWidget(this);

    auto makeWidgetScrollable = [&](QWidget* content, QWidget* parent) -> QWidget* {
        if (!content) return nullptr;
        if (qobject_cast<QScrollArea*>(content)) return content;
        content->setMinimumHeight(0);
        content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        auto* scroll = new QScrollArea(parent);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scroll->setWidget(content);
        return scroll;
    };

    auto makeDockWidgetScrollable = [&](QDockWidget* dock) {
        if (!dock) return;
        QWidget* content = dock->widget();
        if (!content || qobject_cast<QScrollArea*>(content)) return;
        dock->setWidget(makeWidgetScrollable(content, dock));
    };

    // 左侧「工具面板」：QTabWidget 内按需加入各面板；下方 Properties 常驻
    m_leftFieldDock = new QDockWidget(this);
    m_leftFieldDock->setObjectName("LeftFieldDock");
    m_leftFieldDock->setWindowTitle(QStringLiteral("工具面板"));
    m_leftFieldDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    m_leftFieldDock->setFeatures(QDockWidget::DockWidgetClosable);
    m_leftFieldTabs = new QTabWidget(m_leftFieldDock);
    m_leftFieldTabs->setObjectName("LeftFieldTabs");
    m_leftFieldTabs->setTabPosition(QTabWidget::North);
    m_leftFieldTabs->setDocumentMode(true);
    m_leftFieldTabs->setTabsClosable(true);
    connect(m_leftFieldTabs, &QTabWidget::tabCloseRequested, this, &igQtMainWindow::onLeftToolTabCloseRequested);
    m_leftFieldDock->setWidget(m_leftFieldTabs);
    this->addDockWidget(Qt::LeftDockWidgetArea, m_leftFieldDock);

    // 属性窗口停靠在左侧（常驻），图层树悬浮在 OpenGL 右下角
    this->addDockWidget(Qt::LeftDockWidgetArea, modelTreeWidget->getPropertiesDock());
    // 上方为工具 Tab，下方为 Properties
    this->splitDockWidget(m_leftFieldDock,
                          modelTreeWidget->getPropertiesDock(),
                          Qt::Vertical);
    modelTreeWidget->getPropertiesDock()->show();
    // 无 Tab 时不占工具区（仅 Properties 可见）
    m_leftFieldDock->hide();
    // 将其他 dockwidget 以 SelectionField 为基准组织成上方的 tab 组
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ParallelCoordinatesField);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_VariableCorrelationField);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_VariableDensityField);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_DataChangeField);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ContextPreservingShowField);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_QualityDetection);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_EditMode);
    this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ModelList);
    // 轮廓提取 / 网格切面 / 结构形变 改由左侧「工具面板」Tab 按需打开，不再叠在 Selection 组

    // 左侧扩展面板统一采用可滚动内容，避免 dock 过多时撑高主窗口
    makeDockWidgetScrollable(ui->dockWidget_SelectionField);
    makeDockWidgetScrollable(ui->dockWidget_ParallelCoordinatesField);
    makeDockWidgetScrollable(ui->dockWidget_VariableCorrelationField);
    makeDockWidgetScrollable(ui->dockWidget_VariableDensityField);
    makeDockWidgetScrollable(ui->dockWidget_DataChangeField);
    makeDockWidgetScrollable(ui->dockWidget_ContextPreservingShowField);
    makeDockWidgetScrollable(ui->dockWidget_QualityDetection);
    makeDockWidgetScrollable(ui->dockWidget_EditMode);
    makeDockWidgetScrollable(ui->dockWidget_ModelList);
    makeDockWidgetScrollable(ui->dockWidget_ContourExtract);
    makeDockWidgetScrollable(modelTreeWidget->getPropertiesDock());

    // 设置左侧 dock 区域的初始宽度（不锁死，用户仍可拖拽调整）
    QTimer::singleShot(0, this, [this]() {
        if (m_leftFieldDock) {
            const int curW = m_leftFieldDock->width();
            const int targetW = qMax(curW + 60, 360); // 比默认稍宽一点
            this->resizeDocks({m_leftFieldDock}, {targetW}, Qt::Horizontal);
        }
    });

    // 延迟定位图层树悬浮窗口到OpenGL渲染窗口右下角
    QTimer::singleShot(100, this, [this]() {
        if (rendererWidget && modelTreeWidget) {
            modelTreeWidget->positionTreeDockToRendererCorner(rendererWidget);
        }
    });


    SliceDockWidget = new QDockWidget(this);
    SliceDockWidget->setObjectName("dockWidget_Slice");
    SliceDockWidget->setWindowTitle("网格切割");
    SliceWidget = new igQtModelClipWidget(nullptr);
    SliceWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    SliceWidget->setMinimumWidth(300);
    SliceDockWidget->setWidget(SliceWidget);
    SliceDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea);
    SliceDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    this->addDockWidget(Qt::LeftDockWidgetArea, SliceDockWidget);
    makeDockWidgetScrollable(SliceDockWidget);
    SliceDockWidget->hide();

    DeformationDockWidget = new QDockWidget(this);
    DeformationDockWidget->setWindowTitle("结构形变");
    DeformationWidget = new igQtDeformationWidget(DeformationDockWidget);
    DeformationDockWidget->setWidget(DeformationWidget);
    DeformationDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    DeformationDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    DeformationDockWidget->hide();
    this->addDockWidget(Qt::RightDockWidgetArea, DeformationDockWidget);

}
void igQtMainWindow::initToolbarComponent() {
    // 为每个工具栏在下方添加居中文字标题（顺序：文件与输出、操作、选择与编辑、视图设置）
    addToolbarTitle(ui->toolBar_meshfile, "文件与输出");
    addToolbarTitle(ui->toolBar_3, "可视化");
    addToolbarTitle(ui->toolBar_2, "选择与编辑");
    addToolbarTitle(ui->toolBar_4, "视图设置");
    relayoutToolbarWrappers();
}

void igQtMainWindow::initAllComponents() {
    connect(ui->action_ShowOrientationAxes, &QAction::triggered, this, [&](bool checked){
        iGame::SceneManager::Instance()->GetCurrentScene()->ToggleAxes();
        iGame::SceneManager::Instance()->GetCurrentScene()->Update();
   });
    connect(ui->action_ChangeBackground, &QAction::triggered, this, [&]() {
        igQtChangeBackGroundDialog dialog(this);
        dialog.setWindowTitle("Change BackGround Color.");
      int R = 0, G = 0, B = 0;
      if (dialog.exec() == QDialog::Accepted) {
          auto input = dialog.getInput();
          R = input[0], G = input[1], B = input[2];
          iGame::SceneManager::Instance()->GetCurrentScene()->SetBackGround(R, G, B);
      }
    });
    connect(ui->action_VolumeRendering, &QAction::triggered, this,
            [&](bool toggled) { iGame::SceneManager::Instance()->GetCurrentScene()->SetVolumeRendering(toggled); });
    // init ProgressBar
    progressBarWidget = new igQtProgressBarWidget(this);
    this->statusBar()->addPermanentWidget(progressBarWidget);

    // vortexMetricsLabel
    vortexMetricsLabel = new QLabel(rendererWidget);
    vortexMetricsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    vortexMetricsLabel->setStyleSheet(
        "QLabel { color: rgb(230,230,230); font-size: 20px; "
        "background: rgba(30,30,30,150); padding: 8px 12px; border-radius: 6px; }");
	    vortexMetricsLabel->hide();

	    connect(ui->action_compress, &QAction::triggered, this, [&](bool checked) {
	        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return false;
	        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
	        // 支持两种情况：
	        // 1) 单块：当前对象本身是可压缩的 PointSet
	        // 2) 多块：根对象为容器（HasSubDataObject()==true），由 MeshCodecDialog 自动切换到 IGCM + IGC
	        if (!DynamicCast<PointSet>(obj) && !obj->HasSubDataObject()) return false;

	        igQtMeshCodecDialog* d = new igQtMeshCodecDialog(this, obj);
	        d->exec();

	        return true;
	    });

    connect(ui->action_LoadFile, &QAction::triggered, fileLoader, &igQtFileLoader::LoadFile);
    // connect(ui->action_CS, &QAction::triggered, fileLoader, &igQtFileLoader::LoadOnlineS);
    // connect(ui->action_C, &QAction::triggered, fileLoader, &igQtFileLoader::LoadOnlineC);
    connect(ui->action_SaveMeshAs, &QAction::triggered, fileLoader, &igQtFileLoader::SaveFileAs);
    connect(ui->action_SaveMesh, &QAction::triggered, fileLoader, &igQtFileLoader::SaveFileAs);

    //// 添加按钮：将当前标量场移到第一个位置并另存为
    //QAction* action_MoveScalarToFirstAndSave = new QAction("将标量场移到首位并另存为", this);
    //action_MoveScalarToFirstAndSave->setShortcut(QKeySequence()); // 可以设置快捷键
    //ui->menu_help->addAction(action_MoveScalarToFirstAndSave);
    //connect(action_MoveScalarToFirstAndSave, &QAction::triggered, this, [&]() {
    //    // 获取当前场景的当前模型
    //    auto scene = rendererWidget->GetScene();
    //    if (!scene) {
    //        QMessageBox::warning(this, "警告", "当前没有活动场景");
    //        return;
    //    }
    //    auto model = scene->GetCurrentModel();
    //    if (!model) {
    //        QMessageBox::warning(this, "警告", "当前没有活动模型");
    //        return;
    //    }
    //    auto dataObject = model->GetDataObject();
    //    if (!dataObject) {
    //        QMessageBox::warning(this, "警告", "无法获取数据对象");
    //        return;
    //    }
    //
    //    // 获取当前选择的标量场索引
    //    int currentAttributeIndex = dataObject->GetAttributeIndex();
    //    if (currentAttributeIndex < 0) {
    //        QMessageBox::warning(this, "警告", "当前未选择任何标量场");
    //        return;
    //    }
    //
    //    // 获取属性集
    //    auto attributeSet = dataObject->GetAttributeSet();
    //    if (!attributeSet) {
    //        QMessageBox::warning(this, "警告", "无法获取属性集");
    //        return;
    //    }
    //
    //    // 获取所有属性
    //    auto allAttributes = attributeSet->GetAllAttributes();
    //    if (!allAttributes || allAttributes->GetNumberOfElements() == 0) {
    //        QMessageBox::warning(this, "警告", "属性集为空");
    //        return;
    //    }
    //
    //    // 检查索引是否有效
    //    if (currentAttributeIndex >= allAttributes->GetNumberOfElements()) {
    //        QMessageBox::warning(this, "警告", "当前属性索引无效");
    //        return;
    //    }
    //
    //    // 如果已经在第一个位置，直接另存为
    //    if (currentAttributeIndex == 0) {
    //        fileLoader->SaveFileAs();
    //        return;
    //    }
    //
    //    // 创建新的属性数组，将当前属性移到第一个位置
    //    auto newAttributes = ElementArray<AttributeSet::Attribute>::New();
    //    newAttributes->Reserve(allAttributes->GetNumberOfElements());
    //
    //    // 首先添加当前选择的属性
    //    newAttributes->AddElement(allAttributes->GetElement(currentAttributeIndex));
    //
    //    // 然后添加其他属性（跳过当前属性）
    //    for (int i = 0; i < allAttributes->GetNumberOfElements(); i++) {
    //        if (i != currentAttributeIndex) {
    //            newAttributes->AddElement(allAttributes->GetElement(i));
    //        }
    //    }
    //
    //    // 保存当前属性维度
    //    int currentDimension = dataObject->GetAttributeDimension();
    //
    //    // 设置新的属性数组
    //    attributeSet->SetAllAttributes(newAttributes);
    //
    //    // 标记数据对象已修改
    //    dataObject->Modified();
    //
    //    // 如果是 DrawObject，使用 ViewCloudPicture 方法设置新的属性索引为0
    //    auto drawObject = DynamicCast<DrawObject>(dataObject);
    //    if (drawObject) {
    //        drawObject->ViewCloudPicture(scene, 0, currentDimension);
    //    }
    //
    //    // 更新模型树和渲染
    //    modelTreeWidget->updateAllAttriubute(dataObject);
    //    rendererWidget->update();
    //
    //    // 自动触发另存为
    //    fileLoader->SaveFileAs();
    //});
    connect(ui->action_UseOrthographic, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_UseOrthographic->isChecked()) {
            SceneManager::Instance()->GetCurrentScene()->ChangeCameraType(Camera::Type::ORTHOGRAPHIC);
        } else {
            SceneManager::Instance()->GetCurrentScene()->ChangeCameraType(Camera::Type::PERSPECTIVE);
        }
        rendererWidget->update();
    });
    connect(ui->action_ResetCameraView, &QAction::triggered, this, [&]() {
        SceneManager::Instance()->GetCurrentScene()->ResetCameraView();
        rendererWidget->update();
    });

    connect(ui->action_setViewToPositiveX, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToPositiveX();
        rendererWidget->update();
    });
    connect(ui->action_setViewToNegativeX, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToNegativeX();
        rendererWidget->update();
    });
    connect(ui->action_setViewToPositiveY, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToPositiveY();
        rendererWidget->update();
    });
    connect(ui->action_setViewToNegativeY, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToNegativeY();
        rendererWidget->update();
    });
    connect(ui->action_setViewToPositiveZ, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToPositiveZ();
        rendererWidget->update();
    });
    connect(ui->action_setViewToNegativeZ, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToNegativeZ();
        rendererWidget->update();
    });
    connect(ui->action_setViewToIsometric, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToIsometric();
        rendererWidget->update();
    });
    connect(ui->action_ResetViewByBoundingBox, &QAction::triggered, this, [&](bool checked) {
        auto scene = rendererWidget->GetScene();
        if (scene == nullptr) return;
        auto interactor = scene->GetInteractor();
        if (interactor == nullptr) return;
        auto basicStyle = interactor->GetSpecialInteractor("SelectBox");
        if (basicStyle == nullptr) return;
        auto boxStyle = DynamicCast<iGame::BoxStyle>(basicStyle);
        auto box = boxStyle->GetBox();
        auto minMaxP = box->GetExtremePoint();
        auto boundingBox = BoundingBox(minMaxP.first, minMaxP.second);
        scene->ResetCameraView(boundingBox);
        rendererWidget->update();
    });
    connect(ui->action_rotateNinetyClockwise, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->RotateNinetyClockwise();
        rendererWidget->update();
    });
    connect(ui->action_rotateNinetyCounterClockwise, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->RotateNinetyCounterClockwise();
        rendererWidget->update();
    });


    connect(ui->action_ShowCenter, &QAction::toggled, this, [&](bool checked) {
        /*qDebug() << "Toggle state:" << checked;*/

        rendererWidget->GetScene()->ToggleCenterAxes();
        ui->action_ShowCenter->setChecked(checked);

        rendererWidget->update();
    });

    connect(ui->action_PickCenter, &QAction::toggled, this, [&](bool checked) {
        //拖拽
        if (checked) {
            // 显示坐标轴并进入拖拽模式
            rendererWidget->GetScene()->GetCenterAxesModel()->SetVisibility(true);
            rendererWidget->ChangeInteractorStyle(Interactor::DragCenterStyle);
            //rendererWidget->setCursor(Qt::CrossCursor);
        } else {
            // 退出选择模式
            rendererWidget->setCursor(Qt::ArrowCursor);
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
        ui->action_PickCenter->setChecked(checked);
        rendererWidget->update();
    });


    connect(ui->action_SaveScreenShot, &QAction::triggered, this, [&]() {
        QString path =
                QFileDialog::getSaveFileName(nullptr, "Save Screen shot", "", "PNG Images(*.png);;BMP Images(*.bmp)");
        igQtScreenShotOptionDialog dialog(this);
        dialog.setDialogTitle(QStringLiteral("Save Screenshot option"));
        int oldwidth = rendererWidget->width(), oldheight = rendererWidget->height();
        int ratio_pixel = rendererWidget->devicePixelRatio();
        int width = 1920, height = 1080;
        if (dialog.exec() == QDialog::Accepted) {
            auto input = dialog.getInput();
            width = input.first, height = input.second;
        }

        width /= ratio_pixel, height /= ratio_pixel;
        rendererWidget->resize(width, height);
        QImage saved_image = rendererWidget->grabFramebuffer();
        rendererWidget->resize(oldwidth, oldheight);
        const bool savedOk = saved_image.save(path, "BMP");
        showDarkFramelessMessage(QStringLiteral("截图结果"),
                                 savedOk ? QStringLiteral("保存成功") : QStringLiteral("保存失败"), savedOk);
    });

    connect(ui->action_SaveAnimation, &QAction::triggered, this, [&]() { ui->widget_Animation->saveAnimation(); });

    connect(ui->action_SetThreadNum, &QAction::triggered, this, [&](bool checked) {
        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("设置并行线程数"));
        // 获取当前线程池的默认线程数
        int currentThreadCount = iGame::ThreadPool::GetDefaultThreadCount();
        int maxThreads = std::thread::hardware_concurrency();
        QString recommendedThreads = QString::number(maxThreads / 2);
        dialog->setFilterDescription(QString("当前并行线程数: %1<br>"
                                             "硬件支持的最大线程数: %2<br>"
                                             "推荐线程数: %3<br>"
                                             "注意: 设置并行线程数会影响程序的性能。<br>"
                                             "建议根据硬件配置合理设置线程数。")
                                             .arg(currentThreadCount)
                                             .arg(maxThreads)
                                             .arg(recommendedThreads));

        // 添加参数：线程数输入框
        int id1 = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "并行线程数",
                                       QString::number(currentThreadCount));
        // 显示对话框
        dialog->show();
        // 设置应用按钮的回调函数
        dialog->setApplyFunctor([=, this]() {
            bool ok;
            // 获取用户输入的线程数
            int newThreadCount = dialog->getInt(id1, ok);
            // 检查输入是否有效
            if (ok && newThreadCount > 0) {
                // 检查线程数是否超过硬件支持的最大值
                /*if (newThreadCount > maxThreads) {
					QMessageBox::warning(this, "错误", QString("线程数不能超过硬件支持的最大值: %1").arg(maxThreads));
					return;
				}*/
                // 设置新的线程数
                iGame::ThreadPool::SetDefaultThreadCount(newThreadCount);
                showDarkFramelessMessage(QStringLiteral("成功"),
                                         QStringLiteral("并行线程数已设置为: %1").arg(newThreadCount), true);
                dialog->close();
            } else {
                showDarkFramelessMessage(QStringLiteral("错误"), QStringLiteral("请输入有效的线程数（大于0的整数）。"));
            }
        });
    });

    // AI聊天助手
    connect(ui->action_AiChat, &QAction::triggered, this, [&](bool checked) {
        if (aiChatDockWidget->isVisible()) {
            aiChatDockWidget->hide();
        } else {
            aiChatDockWidget->show();
        }
    });

    connect(ui->action_StrucDeformation, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Deformation); });
    connect(ui->action_StreamLine, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Flow); });


    initAllDockWidgetConnectWithAction();
    initAllMySignalConnections();
}

void igQtMainWindow::updateVortexMetricsLabelPos()
{
    if (!vortexMetricsLabel || !vortexMetricsLabel->isVisible()) return;

    vortexMetricsLabel->adjustSize();

    const int margin = 20;
    int x = rendererWidget->width()  - vortexMetricsLabel->width()  - margin;
    int y = rendererWidget->height() - vortexMetricsLabel->height() - margin;

    vortexMetricsLabel->move(x, y);
    vortexMetricsLabel->raise();
}

void igQtMainWindow::showDarkFramelessMessage(const QString& title, const QString& text, bool useInformationIcon) {
    igQtShowDarkFramelessMessage(this, title, text, useInformationIcon);
}

void igQtMainWindow::initAllFilters() {
    /* Data Processing 前两档：加宽以容纳较长参数标签，并关闭参数区滚动条（内容较少无需滚动） */
    auto tuneMeshSimplifyFilterDialog = [](igQtFilterDialogDockWidget* d) {
        constexpr int kDialogWidth = 360;
        d->setFixedWidth(kDialogWidth);
        if (auto* sa = d->findChild<QScrollArea*>(QStringLiteral("scrollArea"))) {
            sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    };

    QMenu* mesh_processing = ui->menu_filters->addMenu(QStringLiteral("数据处理 (Data Processing)"));
    connect(mesh_processing->addAction(QStringLiteral("表面网格简化 (Surface Simplification)")), &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("表面网格简化"));
        int reductionId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("简化比例 (0..1)"), "0.5");
        int preserveId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("保留网格边界"), "true");
        int scalarId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("检查网格全部标量"),
                                            "true");
        int checkId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("几何相似性度量"),
                                           "false");
        tuneMeshSimplifyFilterDialog(dialog);
        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok;
            QString result = "";

            MeshTriangulationFilter::Pointer triangulation = MeshTriangulationFilter::New();
            auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
            triangulation->SetInput(obj);
            ok = triangulation->Execute();

            if (!ok) {
                result = QString("网格简化算法只支持表面网格");
                showDarkFramelessMessage(QStringLiteral("非表面网格"), result);
                dialog->close();
                return;
            }

            obj = triangulation->GetOutput();

            MeshSimplificationFilter::Pointer filter = MeshSimplificationFilter::New();
            filter->SetTargetReduction(1 - dialog->getDouble(reductionId, ok));
            filter->SetPreserveBoundary(dialog->getChecked(preserveId, ok));
            filter->SetAllScalarCheck(dialog->getChecked(scalarId, ok));
            filter->SetInput(obj);

            ok = filter->Execute();

            if (!ok) {
                result = QStringLiteral("执行出错");
                showDarkFramelessMessage(QStringLiteral("执行出错"), result);
                dialog->close();
                return;
            }

            auto oldMesh = DynamicCast<SurfaceMesh>(obj);
            auto outObj = filter->GetOutput();
            auto newMesh = DynamicCast<SurfaceMesh>(outObj);
            auto oldPoints = oldMesh->GetPoints();
            auto newPoints = newMesh->GetPoints();


            if (dialog->getChecked(checkId, ok)) {
                PointFinder::Pointer newPicker = PointFinder::New();
                newPicker->SetPoints(newPoints);
                newPicker->Initialize();

                double w1 = 0.0, w2 = 0.0;
                // 计算原始网格的表面积
                for (int i = 0; i < oldMesh->GetNumberOfFaces(); i++) {
                    igIndex f[3]{};
                    oldMesh->GetFacePointIds(i, f);
                    Point v0 = oldMesh->GetPoint(f[0]);
                    Point v1 = oldMesh->GetPoint(f[1]);
                    Point v2 = oldMesh->GetPoint(f[2]);

                    Vector3f d10 = v1 - v0;
                    Vector3f d20 = v2 - v0;

                    w1 += CrossProduct(d10, d20).norm() / 2.0;
                }

                double d1 = 0.0, d2 = 0.0;
                double d3 = 0.0, d4 = 0.0;

                iGame::ProgressObserver* ProgressBar = iGame::ProgressObserver::Instance();
                ProgressBar->UpdateProgress(0);
                int blockNum = oldPoints->GetNumberOfPoints() / 100, progress = 0;
                // 计算平均平方距离
                for (int i = 0; i < oldPoints->GetNumberOfPoints(); i++) {
                    if (i > progress * blockNum) {
                        ProgressBar->UpdateProgress(progress * 0.01);
                        progress++;
                    }
                    auto p = oldPoints->GetPoint(i);

                    igIndex id = newPicker->FindClosestPoint(p);
                    if (id != -1) {
                        Point cp = newPoints->GetPoint(id);
                        d1 += (p - cp).squaredNorm();
                        d3 += (p - cp).norm();
                    }
                }

                double d = 1.0 / w1 * d1 /*+ 1.0 / w2 * d2*/;
                double dd = 1.0 / oldPoints->GetNumberOfPoints() * d3 /*+ 1.0 / newPoints->GetNumberOfPoints() * d4*/;

                result += "\n几何相似性度量";
                result += "\n Squared Mean Distance: " + QString::number(d);
                result += "\n Mean Distance: " + QString::number(dd);
                result += "\nSquared Mean Distance: " + QString::number(d * 100) + "%";
                result += "\nMean Distance: " + QString::number(dd / oldMesh->GetBoundingBox().diag() * 100) + "%";
                result += "\n\n累计几何误差: " + QString::number(filter->GetError());
            } else {
                result += "\n累计几何误差: " + QString::number(filter->GetError());
            }

            modelTreeWidget->addDataObjectToModelTree(outObj, Algorithm);
            rendererWidget->update();

            // QMessageBox::information(this, "简化成功", result);
            dialog->close();
        });
    });

    connect(mesh_processing->addAction(QStringLiteral("快速表面简化 (Fast Surface Simplification)")), &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene() == nullptr
            || rendererWidget->GetScene()->GetCurrentModel() == nullptr) {
            return;
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("快速表面简化"));
        int reductionId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("目标简化比例 (0..1)"), "0.5");
        int faceCountId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("目标面数"), "0");

        int preserveId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("保留网格边界"), "true");
        //int scalarId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("检查网格全部标量"),
        //                                    "true");

        tuneMeshSimplifyFilterDialog(dialog);
        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok;
            QString result = "";

            if (rendererWidget->GetScene() == nullptr
                || rendererWidget->GetScene()->GetCurrentModel() == nullptr) {
                showDarkFramelessMessage(QStringLiteral("无可用模型"), QStringLiteral("请先加载并选择模型。"));
                dialog->close();
                return;
            }
            auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
            if (obj == nullptr) {
                showDarkFramelessMessage(QStringLiteral("无可用模型"), QStringLiteral("当前模型没有可用数据。"));
                dialog->close();
                return;
            }

            MeshSimplificationFilterPro::Pointer filter = MeshSimplificationFilterPro::New();
            filter->SetInput(obj);
            filter->SetTargetReduction(dialog->getDouble(reductionId, ok));
            filter->SetTargetFaceCount(dialog->getInt(faceCountId, ok));
            filter->SetPreserveBoundary(dialog->getChecked(preserveId, ok));
            filter->SetFreeze(true);
            filter->SetTransformToCellData(true);
            ok = filter->Execute();

            if (!ok) {
                result = QStringLiteral("算法执行错误");
                showDarkFramelessMessage(QStringLiteral("执行出错"), result);
                dialog->close();
                return;
            }

            auto new_mesh = filter->GetOutput(0);
            if (new_mesh == nullptr) {
                showDarkFramelessMessage(QStringLiteral("执行出错"), QStringLiteral("算法未产生有效结果。"));
                dialog->close();
                return;
            }
            modelTreeWidget->addDataObjectToModelTree(new_mesh, Algorithm);
            rendererWidget->update();
            // QMessageBox::information(this, "执行成功", result);
            dialog->close();
        });
    });

    connect(mesh_processing->addAction(QStringLiteral("表面简化 (Surface Simplification)")),
        &QAction::triggered, this, [&](bool checked) {
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

        SurfaceMesh::Pointer mesh;
        if (obj->GetDataObjectType() == IG_SURFACE_MESH) {
            mesh = DynamicCast<SurfaceMesh>(obj);
        } else if (obj->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
            mesh = DynamicCast<UnstructuredMesh>(obj)->TransferToSurfaceMesh();
        }

        MeshTriangulationFilter::Pointer triangulation = MeshTriangulationFilter::New();
        triangulation->SetInput(mesh);
        if (!triangulation->Execute()) return false;
        mesh = DynamicCast<SurfaceMesh>(triangulation->GetOutput());

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
        int reductionId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Reduction (0..1)", "0.05");
        int faceCountId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Target Face Count", "0");

        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok;
            QString result = "";

            std::vector<FVector> V;
            std::vector<int> F;
            std::vector<std::vector<float>> A;
            std::vector<float> AW;

            float minv[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
            float maxv[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

            for (size_t i = 0; i < mesh->GetNumberOfPoints(); ++i) {
                auto& v = mesh->GetPoint(i);

                V.push_back({v[0], v[1], v[2]});

                for (int j = 0; j < 3; ++j) {
                    float vj = v[j];

                    minv[j] = minv[j] > vj ? vj : minv[j];
                    maxv[j] = maxv[j] < vj ? vj : maxv[j];
                }
            }

            float extent = 0.f;

            extent = (maxv[0] - minv[0]) < extent ? extent : (maxv[0] - minv[0]);
            extent = (maxv[1] - minv[1]) < extent ? extent : (maxv[1] - minv[1]);
            extent = (maxv[2] - minv[2]) < extent ? extent : (maxv[2] - minv[2]);

            float scale = extent == 0 ? 0.f : 1.f / extent;

            for (size_t i = 0; i < mesh->GetNumberOfPoints(); ++i) {
                V[i].x = (V[i].x - minv[0]) * scale;
                V[i].y = (V[i].y - minv[1]) * scale;
                V[i].z = (V[i].z - minv[2]) * scale;
            }

            igIndex ids[3];
            for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
                mesh->GetFacePointIds(i, ids);
                F.push_back(ids[0]);
                F.push_back(ids[1]);
                F.push_back(ids[2]);
            }

            for (int i = 0; i < mesh->GetAttributeSet()->GetNumberOfAttributes(); i++) {
                auto& attr = mesh->GetAttributeSet()->GetAttribute(i);
                int dim = attr.pointer->GetDimension();
                for (int d = 0; d < dim; d++) {
                    double val_max = -FLT_MAX;
                    double val_min = FLT_MAX;
                    for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
                        double val = attr.pointer->GetValue(j * dim + d);
                        val_max = val_max < val ? val : val_max;
                        val_min = val_min > val ? val : val_min;
                    }
                    if (val_min == val_max) {
                        std::vector<float> data(mesh->GetNumberOfPoints(), 0.f);
                        A.push_back(std::move(data));
                        AW.push_back(0.f);
                    } else {
                        std::vector<float> data;
                        for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
                            data.push_back(attr.pointer->GetValue(j * dim + d));
                        }
                        //for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
                        //    data.push_back((attr.pointer->GetValue(j * dim + d)));
                        //}
                        A.push_back(std::move(data));
                        AW.push_back(1.0f / (val_max - val_min));
                    }
                }
            }

            clock_t start = clock();
            MeshSaliencyCalculator saliencyCalculator(V, F);
            saliencyCalculator.Execute();

            MeshSimplifierWithAttributes simplifier(V, F, A, AW, saliencyCalculator.NormalCurvature,
                                                    dialog->getDouble(reductionId, ok));

            simplifier.SetUseVertexImportance(false);
            simplifier.SetUseDynamicAttributePenalty(true);

            simplifier.IsOptimizedPosition = true;
            simplifier.Execute();

            clock_t end = clock();
            std::cout << "Time taken: " << double(end - start) / CLOCKS_PER_SEC << " seconds." << std::endl;


            auto newMesh = SurfaceMesh::New();
            for (const auto& v: V) {
                newMesh->AddPoint(Point(v.x * extent + minv[0], v.y * extent + minv[1], v.z * extent + minv[2]));
            }
            //for (const auto& v: V) {
            //    newMesh->AddPoint(Point(v.x, v.y, v.z));
            //}
            auto CellArray = CellArray::New();
            for (int i = 0; i < F.size() / 3; i++) { CellArray->AddCellId3(F[i * 3], F[i * 3 + 1], F[i * 3 + 2]); }
            newMesh->SetFaces(CellArray);

            int count = 0;
            auto Attributes = AttributeSet::New();
            for (int i = 0; i < mesh->GetAttributeSet()->GetNumberOfAttributes(); i++) {
                auto& attr = mesh->GetAttributeSet()->GetAttribute(i);
                int dim = attr.pointer->GetDimension();
                auto arr = FloatArray::New();
                arr->SetDimension(dim);
                arr->Resize(mesh->GetNumberOfPoints());
                arr->SetName(attr.pointer->GetName());

                for (int d = 0; d < dim; d++) {
                    for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
                        arr->SetValue(j * dim + d, A[count + d][j]);
                    }
                }
                count += dim;

                Attributes->AddAttribute(attr.type, attr.attachmentType, arr);
            }
            newMesh->SetAttributeSet(Attributes);

            modelTreeWidget->addDataObjectToModelTree(newMesh, Algorithm);
            rendererWidget->update();
            dialog->close();
        });
    });

    connect(mesh_processing->addAction(QStringLiteral("表面三角化 (Surface Triangulation)")), &QAction::triggered, this, [&](bool checked) {
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

        MeshTriangulationFilter::Pointer triangulation = MeshTriangulationFilter::New();
        triangulation->SetInput(obj);
        if (triangulation->Execute()) {
            auto mesh = DynamicCast<SurfaceMesh>(triangulation->GetOutput());

            modelTreeWidget->addDataObjectToModelTree(mesh, Algorithm);
            rendererWidget->update();
        }
    });

    connect(mesh_processing->addAction(QStringLiteral("表面提取 (Surface Extraction)")), &QAction::triggered, this, [&](bool checked) {
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

        if (VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(obj)) {
            auto new_mesh = mesh->GetRenderableObject();
            new_mesh->SetName(mesh->GetName() + "_surface");
            modelTreeWidget->addDataObjectToModelTree(new_mesh, Algorithm);
            rendererWidget->update();

        } else if (UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(obj)) {
            auto new_mesh = mesh->GetRenderableObject();
            new_mesh->SetName(mesh->GetName() + "_surface");
            modelTreeWidget->addDataObjectToModelTree(new_mesh, Algorithm);
            rendererWidget->update();

        } else if (DrawObject::Pointer mesh = DynamicCast<DrawObject>(obj)) {
            auto new_mesh = mesh->GetRenderableObject();
            new_mesh->SetName(mesh->GetName() + "_surface");
            modelTreeWidget->addDataObjectToModelTree(new_mesh, Algorithm);
            rendererWidget->update();
        }
    });

    //connect(mesh_processing->addAction("Test"), &QAction::triggered, this, [&](bool checked) {
    //    auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

    //    auto m_StreamBase = iGame::StreamBase::New();
    //    auto streamtracer = m_StreamBase->streamFilter;
    //    streamtracer->initStreamTracer(obj);
    //    //auto seeds=streamtracer->getModelSelect();//当实际已经选中了重点区域时直接调用该函数
    //    Vector3f boundMax = streamtracer->GetMesh()->GetBoundingBox().max; //包围盒区域
    //    Vector3f boundMin = streamtracer->GetMesh()->GetBoundingBox().min;
    //    Vector3f centerMax = (boundMax - boundMin) / 5 + boundMin; //模拟被选中重点区域
    //    auto seeds = streamtracer->getAllSubBlockCenters(boundMax, boundMin, centerMax, boundMin, 2,
    //                                                     4); //4，6为划分子块的数量
    //    float lengthOfStreamLine = 5;
    //    float lengthOfStep = 0.3;
    //    float maxSteps = 1000;
    //    float terminalSpeed = 0.005;
    //    streamtracer->SetInput(seeds, "V", lengthOfStreamLine, lengthOfStep, terminalSpeed, maxSteps);
    //    streamtracer->Execute();
    //    std::cout << seeds.size() << std::endl;
    //    auto output = streamtracer->GetOutput();

    //    modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
    //    rendererWidget->update();
    //});

    //connect(mesh_processing->addAction("Test2"), &QAction::triggered, this, [&](bool checked) { 
    //    auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

    //    auto filter = iGame::VolumeMeshMetricsFilter::New();
    //    filter->SetVolumeMetric(VolumeMeshMetricsFilter::HEX_VOLUME);
    //    filter->SetInput(obj);
    //    filter->Execute();

    //    modelTreeWidget->addDataObjectToModelTree(filter->GetOutput(), Algorithm);
    //    rendererWidget->update();
    //    });
    //connect(mesh_processing->addAction("Test3"), &QAction::triggered, this, [&](bool checked) 
    //    { 
    //        CellArray::Pointer cellArray = CellArray::New();
    //        clock_t start = clock();
    //        igIndex cell[3]{};
    //        cellArray->AddCellIds(cell, 2);
    //        for (int i = 0; i < 10000000; i++) { 
    //            cellArray->AddCellIds(cell, 3);
    //        }
    //        clock_t end = clock();
    //        std::cout << end - start << std::endl;

    //    });
    QMenu* convert = ui->menu_filters->addMenu(QStringLiteral("数据转换 (Convert)"));
    connect(convert->addAction(QStringLiteral("转换为点数据 (Convert To PointData)")), &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        ConvertToPointDataFilter::Pointer filter = ConvertToPointDataFilter::New();
        filter->SetInput(obj);
        if (filter->Execute()) {
            modelTreeWidget->addDataObjectToModelTree(filter->GetOutput(), Algorithm);
            rendererWidget->update();
        }
    });
    connect(convert->addAction(QStringLiteral("转换为单元数据 (Convert To CellData)")), &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        ConvertToCellDataFilter::Pointer filter = ConvertToCellDataFilter::New();
        filter->SetInput(obj);
        if (filter->Execute()) {
            modelTreeWidget->addDataObjectToModelTree(filter->GetOutput(), Algorithm);
            rendererWidget->update();
        }
    });


    QMenu* view = ui->menu_filters->addMenu("特征提取");

    QAction* gradient = view->addAction(QStringLiteral("计算梯度 (ComputeGradient)"));
    connect(gradient, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        GradientFilter::Pointer filter = GradientFilter::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        filter->SetAttributeByIndex(data->GetAttributeIndex());
        int index = data->GetAttributeIndex();
        if (filter->Execute()) {
            modelTreeWidget->updateAllAttriubute(data);
            auto drawObject = DynamicCast<DrawObject>(data);
            if (drawObject) {
                auto item = modelTreeWidget->getItemFromObject(data);
                if (item && item->childCount() > 0) {
                    item->setExpanded(true);
                    auto child = item->child(index);
                    if (child) {
                        item->setCurrentChild(child);
                        item->setSelected(false);
                        item->viewAttribute(index, -1);
                        child->setSelected(true);
                        modelTreeWidget->setCurrentItem(child);
                    }
                }
            }
        }
        else {
            std::string message = filter->GetMessage();
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));

        }
    });

    QAction* laplacian = view->addAction(QStringLiteral("计算拉普拉斯 (ComputeLaplacian)"));
    connect(laplacian, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        LaplacianFilter::Pointer filter = LaplacianFilter::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        filter->SetAttributeByIndex(data->GetAttributeIndex());
        int index = data->GetAttributeIndex();
        if (filter->Execute()) {
            modelTreeWidget->updateAllAttriubute(data);
            auto drawObject = DynamicCast<DrawObject>(data);
            if (drawObject) {
                auto item = modelTreeWidget->getItemFromObject(data);
                if (item && item->childCount() > 0) {
                    item->setExpanded(true);
                    auto child = item->child(index);
                    if (child) {
                        item->setCurrentChild(child);
                        item->setSelected(false);
                        item->viewAttribute(index, -1);
                        child->setSelected(true);
                        modelTreeWidget->setCurrentItem(child);
                    }
                }
            }
        }
        else {
            std::string message = filter->GetMessage();
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
        }
    });

    QAction* curvature = view->addAction(QStringLiteral("计算曲率 (ComputeCurvature)"));
    connect(curvature, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        CurvatureFilter::Pointer filter = CurvatureFilter::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        filter->SetAttributeByIndex(data->GetAttributeIndex());
        int index = data->GetAttributeIndex();
        if (filter->Execute()) {
            modelTreeWidget->updateAllAttriubute(data);
            auto drawObject = DynamicCast<DrawObject>(data);
            if (drawObject) {
                auto item = modelTreeWidget->getItemFromObject(data);
                if (item && item->childCount() > 0) {
                    item->setExpanded(true);
                    auto child = item->child(index);
                    if (child) {
                        item->setCurrentChild(child);
                        item->setSelected(false);
                        item->viewAttribute(index, -1);
                        child->setSelected(true);
                        modelTreeWidget->setCurrentItem(child);
                    }
                }
            }
        }
        else {
            std::string message = filter->GetMessage();
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
        }
    });

    QAction* vortex = view->addAction(QStringLiteral("计算涡量 (ComputeVorticity)"));
    connect(vortex, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        int index = data->GetAttributeIndex();

        // 结果刷新到模型树 + 云图，单帧 / 时序共用
        auto refreshTree = [this, data, index]() {
            modelTreeWidget->updateAllAttriubute(data);
            auto drawObject = DynamicCast<DrawObject>(data);
            if (!drawObject) return;
            drawObject->ConvertToDrawableData();
            auto item = modelTreeWidget->getItemFromObject(data);
            if (item && item->childCount() > 0) {
                item->setExpanded(true);
                auto child = item->child(index);
                if (child) {
                    item->setCurrentChild(child);
                    item->setSelected(false);
                    item->viewAttribute(index, -1);
                    child->setSelected(true);
                    modelTreeWidget->setCurrentItem(child);
                }
            }
        };

        // 时序数据（PVD / 多选 VTU 等 MultiSubFiles）：
        // 这里只算「当前帧」，随后开启播放期按需计算——切帧时若该帧已带 vorticities
        // （缓存命中）就直接复用，否则同步算完再渲染该帧。
        // 这样既不用一次性等全部帧算完，也不会因缓存被清而出现空白帧。
        auto frames = data->PeekTimeFrames();
        const int frameNum = frames ? static_cast<int>(frames->GetTimeNum()) : 0;
        if (frameNum > 1) {
            // 属性按名字下传：各帧 AttributeSet 独立，索引不保证一致
            std::string attrName;
            if (auto attrSet = data->GetAttributeSet()) {
                if (index >= 0 && index < attrSet->GetNumberOfAttributes()) {
                    if (auto ptr = attrSet->GetAttribute(index).pointer) { attrName = ptr->GetName(); }
                }
            }
            if (attrName.empty()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("请先选择一个矢量属性"));
                return;
            }

            // 尽量把算过的帧留在缓存里，避免播放时反复重算。
            // 在这里声明诉求，可使后续任何 initAnimationComponents
            //（如选中属性触发 CurrendModelChanged）都不会把缓存关掉。
            ui->widget_Animation->setPreferredCacheNum(frameNum);
            if (frames->GetMaxCacheSize() < static_cast<unsigned int>(frameNum)) {
                frames->EnableCache(frameNum);
            }

            // 只算当前帧；父容器属性登记、值域刷新与进度条都在该函数内部完成
            const int vortIndex = ui->widget_Animation->ensureVortexForCurrentFrame(data, attrName, 0);

            if (vortIndex < 0) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("当前帧的涡量计算失败"));
                return;
            }

            modelTreeWidget->updateAllAttriubute(data);
            if (auto drawObj = DynamicCast<DrawObject>(data)) { drawObj->ConvertToDrawableData(); }
            auto item = modelTreeWidget->getItemFromObject(data);
            if (item && item->childCount() > vortIndex) {
                item->setExpanded(true);
                auto child = item->child(vortIndex);
                if (child) {
                    item->setCurrentChild(child);
                    item->setSelected(false);
                    item->viewAttribute(vortIndex, -1);
                    child->setSelected(true);
                    modelTreeWidget->setCurrentItem(child);
                }
            }

            // 必须放在模型树操作之后：setCurrentItem 会触发 CurrendModelChanged →
            // initAnimationComponents，那里会按模型是否变化决定关闭按需计算。
            // 若提前开启，会被这条链路当成「尚未绑定模型」而立即关掉。
            ui->widget_Animation->setVortexAutoCompute(true, attrName);
            return;
        }

        // 非时序：保持原有单次计算行为
        VortexFilter::Pointer filter = VortexFilter::New();
        filter->SetAttributeByIndex(index);
        filter->SetInput(data);
        if (filter->Execute()) {
            refreshTree();
        } else {
            std::string message = filter->GetMessage();
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
        }
    });

    QAction* vortexPrection = view->addAction(QStringLiteral("涡旋预测 (PredictVortex)"));
    connect(vortexPrection, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        VortexDetection::Pointer filter = VortexDetection::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        filter->SetAttributeByIndex(data->GetAttributeIndex());
        int index = data->GetAttributeIndex();
        if (filter->Execute()) {
            modelTreeWidget->updateAllAttriubute(data);
            rendererWidget->update();
            auto drawObject = DynamicCast<DrawObject>(data);
            if (drawObject) {
                auto item = modelTreeWidget->getItemFromObject(data);
                if (item && item->childCount() > 0) {
                    item->setExpanded(true);
                    auto child = item->child(index);
                    if (child) {
                        item->setCurrentChild(child);
                        item->setSelected(false);
                        item->viewAttribute(index, -1);
                        child->setSelected(true);
                        modelTreeWidget->setCurrentItem(child);
                    }

                }
            }
            // 显示指标
            // double acc  = filter->GetAccuracy();
            // double prec = filter->GetPrecision();
            // double rec  = filter->GetRecall();
            //
            // if (acc > 0.0 && prec > 0.0 && rec > 0.0 &&
            //     !std::isnan(acc) && !std::isnan(prec) && !std::isnan(rec)) {
            //     QDialog* dialog = this->property("vortexMetricsDialog").value<QDialog*>();
            //
            //     if (!dialog) {
            //         dialog = new QDialog(this);
            //         dialog->setWindowTitle("Vortex Prediction Metrics");
            //         dialog->setAttribute(Qt::WA_DeleteOnClose);
            //         dialog->setModal(false);
            //
            //         this->setProperty("vortexMetricsDialog", QVariant::fromValue(dialog));
            //
            //         QLabel* label = new QLabel(dialog);
            //         label->setObjectName("vortexMetricsLabel");
            //         label->setTextFormat(Qt::RichText);
            //         label->setAlignment(Qt::AlignCenter);
            //
            //         QVBoxLayout* layout = new QVBoxLayout(dialog);
            //         layout->addWidget(label);
            //         dialog->setLayout(layout);
            //         dialog->resize(270, 100);
            //         connect(dialog, &QDialog::destroyed, this, [this]() {
            //             this->setProperty("vortexMetricsDialog", QVariant());
            //         });
            //     }
            //     QLabel* label = dialog->findChild<QLabel*>("vortexMetricsLabel");
            //     if (label) {
            //         QString msg = QString(
            //             "<table align='center' cellspacing='6'>"
            //             // "<tr><td>Accuracy</td><td>:</td><td>%1</td></tr>"
            //             "<tr><td>Precision</td><td>:</td><td>%1%<</td></tr>"
            //             "<tr><td>Recall</td><td>:</td><td>%2%<</td></tr>"
            //             "</table>"
            //         )
            //         // .arg(acc,  0, 'f', 3)
            //         .arg(prec * 100.0, 0, 'f', 2)
            //         .arg(rec * 100.0,  0, 'f', 2);
            //
            //         label->setText(msg);
            //     }
            //     QPointer<QDialog> safeDialog(dialog);
            //     QTimer::singleShot(48, this, [safeDialog]() {
            //         if (!safeDialog) return;
            //         safeDialog->show();
            //         safeDialog->raise();
            //     });
            // }

            // old version
            // vortexMetricsLabel
            // double acc  = filter->GetAccuracy();
            // double prec = filter->GetPrecision();
            // double rec  = filter->GetRecall();
            // if (acc > 0.0 && prec > 0.0 && rec > 0.0) {
            //     QString txt = QString("Acc: %1  Prec: %2  Rec: %3")
            //                       .arg(acc,  0, 'f', 3)
            //                       .arg(prec, 0, 'f', 3)
            //                       .arg(rec,  0, 'f', 3);
            //     vortexMetricsLabel->setText(txt);
            //     vortexMetricsLabel->show();
            //     updateVortexMetricsLabelPos();
            // } else {
            //     vortexMetricsLabel->clear();
            //     vortexMetricsLabel->hide();
            // }
        }else {
            std::string message = filter->GetMessage();
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
        }
    });

    QAction* lagrangeUnstructedMesh_visualization = ui->menu_filters->addAction(
            QStringLiteral("拉格朗日非结构网格可视化 (LagrangeUnstructedMesh Visualization)"));
    connect(lagrangeUnstructedMesh_visualization, &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        ConvertToLagrangeUnstructuredMeshFilter::Pointer filter = ConvertToLagrangeUnstructuredMeshFilter::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        if (filter->Execute()) {
            DataObject::Pointer res = filter->GetOutput(0);
            res->SetName(data->GetName());
            modelTreeWidget->addDataObjectToModelTree(res, Algorithm);
        }
    });
}

void igQtMainWindow::initAllDockWidgetConnectWithAction() {
    // 显示并切换到对应 DockWidget / Tab
    auto showAndRaiseDock = [&](QDockWidget* dock) {
        if (!dock) return;
        dock->show();
        dock->raise();
        if (dock->widget()) dock->widget()->setFocus(Qt::OtherFocusReason);
    };

    connect(ui->action_IsShowColorBar, &QAction::triggered, this, &igQtMainWindow::updateColorBarShow);
    connect(ui->action_ExportAnimation, &QAction::triggered, this, [&](bool checked) { showAndRaiseDock(ui->dockWidget_Animation); });
    connect(ui->action_SearchInfo, &QAction::triggered, this, [&](bool checked) {
        if (ui->dockWidget_SearchInfo) {
            ui->dockWidget_SearchInfo->show();
            ui->dockWidget_SearchInfo->raise();
        }
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        ui->widget_SearchInfo->setCurrentModel(model);
    });
    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [this]() {
        if (!ui->dockWidget_SearchInfo || !ui->dockWidget_SearchInfo->isVisible()) return;
        QTimer::singleShot(0, this, [this]() {
            ui->widget_SearchInfo->setCurrentModel(rendererWidget->GetScene()->GetCurrentModel());
        });
    });
    connect(ui->action_Scalar, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Scalar); });
    connect(ui->action_Vector, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Vector); });
    connect(ui->action_Glyph, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Vector); });
    connect(ui->action_Tensor, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Tensor); });
    connect(ui->action_ParallelCoordinates, &QAction::triggered, this, [&](bool checked) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        showAndRaiseDock(ui->dockWidget_ParallelCoordinatesField);
        ui->widget_ParallelCoordinatesField->SetParallelCoordinates(model);
    });

    //############# HIDE SOMETHING ST #############
    ui->action_ParallelCoordinates->setVisible(false);
    ui->action_SearchInfo->setVisible(true);
    //############# HIDE SOMETHING ED #############

    //############# TESTS ST #############
    {
        QAction* testAction{};
        testAction = new QAction(this);
        testAction->setObjectName(QString::fromUtf8("MeshSplit"));
        testAction->setText(QString::fromUtf8("MeshSplit"));
        ui->menu_filters->addAction(testAction);
        testAction->setVisible(true);
        connect(testAction, &QAction::triggered, this, [&](bool checked) {
#define TEST_MAP_BACK
#ifdef TEST_MAP_BACK
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (!model || !model->GetDataObject()) {
                std::cout << "[Block Mapping Test] No model selected." << std::endl;
                return;
            }

            auto obj = iGame::FileIO::ReadFile("D:/RealStudy/editOpeniGame/Examples/Models/segment_result.vtk");
            if (!obj) {
                std::cout << "[Block Mapping Test] Failed to read segment_result.vtk." << std::endl;
                return;
            }

            auto drawObj = DynamicCast<DrawObject>(model->GetDataObject());
            if (!drawObj) {
                std::cout << "[Block Mapping Test] Current model is not drawable." << std::endl;
                return;
            }
            drawObj->ConvertToDrawableData();
            auto surfaceMesh = DynamicCast<SurfaceMesh>(drawObj->GetRenderableObject(false));
            auto segmentedMesh = DynamicCast<UnstructuredMesh>(obj);
            if (!surfaceMesh || !segmentedMesh) {
                std::cout << "[Block Mapping Test] Unsupported original or segmented mesh type." << std::endl;
                return;
            }
            auto resultArray = BlockMapping::GetMappingBlockCellsArray(surfaceMesh, segmentedMesh);
            if (!resultArray) {
                std::cout << "[Block Mapping Test] Failed to map block IDs." << std::endl;
                return;
            }
            resultArray->SetName("block_id");
            auto dataObj = model->GetDataObject();
            dataObj->SetBlockMapping(resultArray);
            modelTreeWidget->updateAllAttriubute(dataObj);
            ui->widget_SearchInfo->setCurrentModel(model);
            std::cout << "[Block Mapping Test] Mapping complete. Cells: " << resultArray->GetNumberOfValues()
                      << std::endl;
#else
            // 测试P3SAM分割器
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (!model) {
                std::cout << "[P3SAM Test] No model selected." << std::endl;
                return;
            }

            auto dataObj = model->GetDataObject();

            std::cout << "[P3SAM Test] Starting P3SAM segmentation..." << std::endl;
            P3SAMSegmenter::Pointer segmenter = P3SAMSegmenter::New();
            segmenter->SetInput(dataObj);
            segmenter->SetSimplificationRatio(0.1f);  // 简化到10%
            segmenter->SetPointNum(20000);
            segmenter->SetPromptNum(500);
            segmenter->SetSeed(42);
            segmenter->SetPostProcess(false);
            segmenter->SetTimeout(300000);  // 5分钟超时

            if (!segmenter->Execute()) {
                std::cout << "[P3SAM Test] Segmentation failed: "
                          << segmenter->GetErrorMessage() << std::endl;
                return;
            }

            modelTreeWidget->updateAllAttriubute(dataObj);
            std::cout << "[P3SAM Test] Segmentation complete! Parts: "
                      << segmenter->GetPartCount() << std::endl;
#endif // TEST_MAP_BACK
        });
    }
    //############# TESTS ED #############

    connect(ui->widget_ParallelCoordinatesField, &igQtParallelCoordinatesWidget::SIGNAL_RefreshDataClicked, this,
            [&]() {
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) return;
                ui->widget_ParallelCoordinatesField->SetParallelCoordinates(model);
            });
    connect(ui->action_VariableCorrelation, &QAction::triggered, this, [&](bool checked) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;

        // 使用动态属性存储对话框指针    // 匿名命名空间，只在当前cpp文件可见
        static igQtChromeFramelessDialog* dialog = nullptr;
        static igQtVariableCorrelationWidget* widget = nullptr;

        if (!dialog) {
            dialog = new igQtChromeFramelessDialog(this);
            dialog->setDialogTitle(QStringLiteral("变量相关性分析"));

            widget = new igQtVariableCorrelationWidget(dialog->contentHost());
            widget->GetUi()->splitter->setSizes({200, 300, 400});
            dialog->setContentWidget(widget);
            dialog->resize(900, 500);
            connect(widget, &igQtVariableCorrelationWidget::SIGNAL_RefreshDataClicked, this, [&]() {
                // 使用sender()获取信号发送者
                auto* senderWidget = qobject_cast<igQtVariableCorrelationWidget*>(sender());
                if (!senderWidget) return;
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) return;
                senderWidget->SetModel(model);
            });
        }

        widget->SetModel(model);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();


        //ui->dockWidget_VariableCorrelationField->show();
        //ui->widget_VariableCorrelationField->SetModel(model);
    });

    connect(ui->widget_VariableCorrelationField, &igQtVariableCorrelationWidget::SIGNAL_RefreshDataClicked, this,
            [&]() {
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) return;
                ui->widget_VariableCorrelationField->SetModel(model);
            });
    connect(ui->action_VariableDensity, &QAction::triggered, this, [&](bool checked) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        openLeftToolPanel(LeftToolPanelId::VariableDensity);
        ui->widget_VariableDensityField->SetModel(model);
    });

    connect(ui->widget_VariableDensityField, &igQtVariableDensityWidget::SIGNAL_RefreshDataClicked, this, [&]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        ui->widget_VariableDensityField->SetModel(model);
    });
    auto DataChangeFunc = [&](igQtMainWindow* mainWindow) {
        auto model = mainWindow->rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        mainWindow->openLeftToolPanel(LeftToolPanelId::DataChange);
        mainWindow->ui->widget_DataChangeField->InitRadialStyle(
                mainWindow->rendererWidget->GetScene()->GetInteractor());
        auto name = mainWindow->rendererWidget->GetScene()->GetInteractor()->SetSpecialInteractor(
                mainWindow->ui->widget_DataChangeField->GetRadialStyle());
        mainWindow->ui->widget_DataChangeField->SetInteractorName(name);
        mainWindow->ui->widget_DataChangeField->SetModel(model);
        mainWindow->ui->widget_DataChangeField->SetScene(mainWindow->rendererWidget->GetScene());
    };
    connect(ui->action_DataChange, &QAction::triggered, this, [&](bool checked) { DataChangeFunc(this); });
    connect(ui->widget_DataChangeField, &igQtDataChangeWidget::SIGNAL_RefreshDataClicked, this,
            [&]() { DataChangeFunc(this); });

    ui->action_ContextPreserving->setVisible(false);
    connect(ui->action_ContextPreserving, &QAction::triggered, this, [&](bool checked) {
        if (checked && !ui->dockWidget_ContextPreservingShowField->isVisible()) {
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (model == nullptr) return;
            showAndRaiseDock(ui->dockWidget_ContextPreservingShowField);
            ui->widget_ContextPreservingShowField->SetContextPreserving(model);
        } else if (!checked && ui->dockWidget_ContextPreservingShowField->isVisible())
            ui->dockWidget_ContextPreservingShowField->hide();
    });
    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [&]() {
        if (ui->dockWidget_ContextPreservingShowField->isHidden()) return;
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) {
            ui->dockWidget_ContextPreservingShowField->hide();
            return;
        }
        ui->widget_ContextPreservingShowField->SetContextPreserving(model);
    });
    connect(ui->widget_ContextPreservingShowField, &igQtContextPreservingShowWidget::DrawUpdated, this,
            [&]() { rendererWidget->update(); });
    connect(ui->action_FlowField, &QAction::triggered, this,
            [this](bool) { openLeftToolPanel(LeftToolPanelId::Flow); });

//    connect(ui->action_FlowField_2, &QAction::triggered, this, [&](bool checked) {
//        ui->dockWidget_FlowField->show();
//        ui->widget_FlowField->updateVectorNameList();
//    });

    //  connect(ui->action_EditMode, &QAction::triggered, this, [&](bool checked)
    //  {
    //	ui->dockWidget_EditMode->show();
    //	});
    //  connect(ui->action_QualityDetection, &QAction::triggered, this, [&](bool
    //  checked) { 	ui->dockWidget_QualityDetection->show();
    //	});
    connect(ui->action_ContourExtract, &QAction::triggered, this, [this](bool) {
        openLeftToolPanel(LeftToolPanelId::ContourExtract);
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        if (!scene) return;
        auto CurrentModel = scene->GetCurrentModel();
        if (!CurrentModel) return;
        auto dataObject = CurrentModel->GetDataObject();
        if (!dataObject) return;
        ui->widget_ContourExtract->SetOriginDataObject(dataObject);
    });
    connect(ui->action_GenerateChart, &QAction::triggered, this, [&](bool checked) {
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        if (!scene) return;
        auto CurrentModel = scene->GetCurrentModel();
        if (!CurrentModel) return;
        auto dataObject = CurrentModel->GetDataObject();
        if (!dataObject) return;
        auto attributeSet = dataObject->GetAttributeSet();
        auto dataIndex = dataObject->GetAttributeIndex();
        auto attrDimension = dataObject->GetAttributeDimension();
        if (dataIndex < 0) { return; }
        auto array = attributeSet->GetAttribute(dataIndex).pointer;
        if (array == nullptr) return;
        ArrayObject::Pointer drawArray = nullptr;
        if (array->GetDimension() <= 1) {
            drawArray = array;
        } else {
            auto tmpArray = FloatArray::New();
            int size = array->GetNumberOfElements();
            tmpArray->Reserve(size);
            tmpArray->SetName(array->GetName());
            for (int i = 0; i < size; i++) { tmpArray->AddValue(array->GetElementValue(i, attrDimension)); }
            drawArray = tmpArray;
        }
        auto chart = new igQtCharts;
        chart->drawBarChart(drawArray);
        chart->exec();
    });
    auto DrawSurfaceMeshByPointer = [](SurfaceMesh::Pointer m, Painter3D* painter, const float color[3]) -> void {
        // 1. draw faces
        painter->SetPen(Pen::Style::NoPen);
        painter->SetBrush(color[0], color[1], color[2]);
        igIndex cell[32]{};
        for (int i = 0; i < m->GetNumberOfFaces(); i++) {
            int ncell = m->GetFacePointIds(i, cell);
            for (int j = 2; j < ncell; j++) {
                painter->DrawTriangle(m->GetPoint(cell[0]), m->GetPoint(cell[j - 1]), m->GetPoint(cell[j]));
            }
        }
        // 2. draw lines
        painter->SetPen(Color::Black);
        painter->SetBrush(Brush::Style::NoBrush);
        if (m->GetEdges() == nullptr) { m->BuildEdges(); }
        for (int i = 0; i < m->GetNumberOfEdges(); i++) {
            int ncell = m->GetEdgePointIds(i, cell);
            if (cell[0] < 0 || cell[1] < 0) {
                throw std::runtime_error("The index of the edge is negative.");
            } else {
                painter->DrawLine(m->GetPoint(cell[0]), m->GetPoint(cell[1]));
            }
        }
        painter->Modified();
    };

    auto AddClippingMeshToScene = [DrawSurfaceMeshByPointer](const std::string& mainName, SurfaceMesh::Pointer OV,
                                                             SurfaceMesh::Pointer t_IV, SurfaceMesh::Pointer OIV,
                                                             igQtModelDialogWidget* modelTreeWidget) {
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

        const std::string OVName = "__" + mainName + "_OV";   // 临时模型
        const std::string IVName = "__" + mainName + "_IV";   // 临时模型
        const std::string OIVName = "__" + mainName + "_OIV"; // 临时模型
        const float OVColor[3]{1.f, 1.f, 1.f};
        const float IVColor[3]{1.f, 1.f, 0.f};
        const float OIVColor[3]{1.f, 1.f, 1.f};
        const float OIVAlpha = 0.2f;

        SurfaceMesh::Pointer IV = SurfaceMesh::New();
        OV->SetName(OVName);
        IV->SetName(IVName);
        OIV->SetName(OIVName);

        Model* IVModel{nullptr};
        bool exist[3]{false, false, false};

        auto modelList = scene->GetModelList();
        for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
            auto id = it->first;
            auto model = it->second;

            if (model->GetDataObject()->GetName() == OVName) {
                auto model = scene->GetModelById(id);
                model->SetDataObject(OV);
                exist[0] = true;
            } else if (model->GetDataObject()->GetName() == IVName) {
                auto model = scene->GetModelById(id);
                model->SetDataObject(IV);
                exist[1] = true;
                IVModel = model;
            } else if (model->GetDataObject()->GetName() == OIVName) {
                auto model = scene->GetModelById(id);
                model->SetDataObject(OIV);
                exist[2] = true;
            }
        }
        if (!exist[0]) modelTreeWidget->addDataObjectToModelTree(OV, ItemSource::Algorithm);
        if (!exist[1]) {
            int id = modelTreeWidget->addDataObjectToModelTree(IV, ItemSource::Algorithm);
            IVModel = scene->GetModelById(id);
        }
        if (!exist[2]) modelTreeWidget->addDataObjectToModelTree(OIV, ItemSource::Algorithm);

        DrawSurfaceMeshByPointer(t_IV, IVModel->GetPainter3D(), IVColor);

        //OV->SetFaceColor(OVColor);
        OV->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
        //IV->SetFaceColor(IVColor);
        //IV->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
        //OIV->SetFaceColor(OIVColor);
        OIV->SetTransparency(OIVAlpha);
        OIV->SetViewStyle(IG_SURFACE);
    };

    connect(ui->action_BoxClipping_Better, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        bool ok;
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto oldCurrentModel = scene->GetCurrentModel();
        auto dataObject = scene->GetCurrentModel()->GetDataObject();
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        static std::vector<int> supportTypes = {IG_VOLUME_MESH, IG_UNSTRUCTURED_MESH, IG_STRUCTURED_MESH};
        if (std::find(supportTypes.begin(), supportTypes.end(), dataObject->GetDataObjectType()) ==
            supportTypes.end()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        auto inputMesh = DynamicCast<iGame::DataObject>(dataObject);
        auto box = inputMesh->GetBoundingBox();
        auto center = (box.min + box.max) * 0.5;
        auto size = box.max - box.min;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("盒子切割（美观）"));
        int x_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_min(0..1)", "0.0");
        int y_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_min(0..1)", "0.0");
        int z_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_min(0..1)", "0.0");
        int x_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_max(0..1)", "0.5");
        int y_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_max(0..1)", "1.0");
        int z_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_max(0..1)", "1.0");
        int flip_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip", "false");
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok;
            auto Clamp = [](double x, double l, double r) -> double {
                if (x < l) return l;
                if (x > r) return r;
                return x;
            };

            double x_min = box.min[0] + size[0] * Clamp(dialog->getDouble(x_min_id, ok), 0., 1.);
            double y_min = box.min[1] + size[1] * Clamp(dialog->getDouble(y_min_id, ok), 0., 1.);
            double z_min = box.min[2] + size[2] * Clamp(dialog->getDouble(z_min_id, ok), 0., 1.);
            double x_max = box.min[0] + size[0] * Clamp(dialog->getDouble(x_max_id, ok), 0., 1.);
            double y_max = box.min[1] + size[1] * Clamp(dialog->getDouble(y_max_id, ok), 0., 1.);
            double z_max = box.min[2] + size[2] * Clamp(dialog->getDouble(z_max_id, ok), 0., 1.);
            bool flip = dialog->getChecked(flip_id, ok);

            auto clipper = iGameVolumeMeshClipper::New();
            clipper->SetInput(0, dataObject);
            clipper->SetExtent(x_min, x_max, y_min, y_max, z_min, z_max, flip);
            clipper->Execute();
            auto OV = DynamicCast<SurfaceMesh>(clipper->GetOutput(0));
            auto IV = DynamicCast<SurfaceMesh>(clipper->GetOutput(1));
            auto OIV = DynamicCast<SurfaceMesh>(clipper->GetOutput(2));
            AddClippingMeshToScene(dataObject->GetName(), OV, IV, OIV, modelTreeWidget);
            drawObject->SetVisibility(false);
            scene->SetCurrentModel(oldCurrentModel);
            rendererWidget->update();
        });
    });

    connect(ui->action_PlaneClipping_Better, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        bool ok;
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto oldCurrentModel = scene->GetCurrentModel();
        auto dataObject = scene->GetCurrentModel()->GetDataObject();
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        static std::vector<int> supportTypes = {IG_VOLUME_MESH, IG_UNSTRUCTURED_MESH, IG_STRUCTURED_MESH};
        if (std::find(supportTypes.begin(), supportTypes.end(), dataObject->GetDataObjectType()) ==
            supportTypes.end()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        auto inputMesh = DynamicCast<iGame::DataObject>(dataObject);
        auto box = inputMesh->GetBoundingBox();
        auto center = (box.min + box.max) * 0.5;
        auto size = box.max - box.min;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("平面切割（美观）"));
        int origin_x_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_x(0..1)", "0.5");
        int origin_y_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_y(0..1)", "0.5");
        int origin_z_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_z(0..1)", "0.5");
        int normal_x_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_x(-1..1)", "1.0");
        int normal_y_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_y(-1..1)", "0.0");
        int normal_z_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_z(-1..1)", "0.0");
        int flip_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip", "false");
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok;
            auto Clamp = [](double x, double l, double r) -> double {
                if (x < l) return l;
                if (x > r) return r;
                return x;
            };

            double origin_x = box.min[0] + size[0] * Clamp(dialog->getDouble(origin_x_id, ok), 0., 1.);
            double origin_y = box.min[1] + size[1] * Clamp(dialog->getDouble(origin_y_id, ok), 0., 1.);
            double origin_z = box.min[2] + size[2] * Clamp(dialog->getDouble(origin_z_id, ok), 0., 1.);
            double normal_x = Clamp(dialog->getDouble(normal_x_id, ok), -1., 1.);
            double normal_y = Clamp(dialog->getDouble(normal_y_id, ok), -1., 1.);
            double normal_z = Clamp(dialog->getDouble(normal_z_id, ok), -1., 1.);
            bool flip = dialog->getChecked(flip_id, ok);
            if (normal_x == 0. && normal_y == 0. && normal_z == 0.) {
                std::cout << "Normal is a vector of zero" << std::endl;
                return;
            }

            auto clipper = iGameVolumeMeshClipper::New();
            clipper->SetInput(0, dataObject);
            clipper->SetPlane(origin_x, origin_y, origin_z, normal_x, normal_y, normal_z, flip);
            clipper->Execute();
            auto OV = DynamicCast<SurfaceMesh>(clipper->GetOutput(0));
            auto IV = DynamicCast<SurfaceMesh>(clipper->GetOutput(1));
            auto OIV = DynamicCast<SurfaceMesh>(clipper->GetOutput(2));
            AddClippingMeshToScene(dataObject->GetName(), OV, IV, OIV, modelTreeWidget);
            drawObject->SetVisibility(false);
            scene->SetCurrentModel(oldCurrentModel);
            rendererWidget->update();
        });
    });

    connect(ui->action_slice, &QAction::triggered, this, [this](bool) {
        const int sid = static_cast<int>(LeftToolPanelId::Slice);
        const int existing = m_leftToolTabByPanel[static_cast<size_t>(sid)];
        if (existing >= 0 && m_leftFieldDock && m_leftFieldDock->isVisible() && m_leftFieldTabs &&
            m_leftFieldTabs->currentIndex() == existing) {
            return;
        }
        openLeftToolPanel(LeftToolPanelId::Slice);
        if (!rendererWidget->GetScene() || !rendererWidget->GetScene()->GetCurrentModel()) return;
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!obj) return;
        SliceWidget->SetOriginDataObject(obj);
        rendererWidget->getInteractor()->SetDataObject(obj);
        rendererWidget->getInteractor()->SetPainter3D(rendererWidget->GetScene()->GetCurrentModel()->GetPainter3D());
        rendererWidget->getInteractor()->RequestSlicingStyle(SliceWidget->GetSelection());
    });
    connect(SliceWidget, &igQtModelClipWidget::DrawClipModel, this,
            [&](DrawObject::Pointer mesh) { modelTreeWidget->addDataObjectToModelTree(mesh, ItemSource::Algorithm); });
    connect(SliceWidget, &igQtModelClipWidget::UpdateClipModel, this, [&]() {
        modelTreeWidget->updateCurrentModelInfo();
        rendererWidget->update();
    });
    connect(SliceWidget, &igQtModelClipWidget::ResetInteractor, this, [&]() {
        if (!rendererWidget->getInteractor()->IsBasicStyle()) {
            rendererWidget->getInteractor()->RequestBasicStyle();
            return;
        }
    });
    connect(ui->action_deformation, &QAction::triggered, this, [this](bool checked) {
        if (checked)
            openLeftToolPanel(LeftToolPanelId::Deformation);
        else
            closeLeftToolPanel(LeftToolPanelId::Deformation);
    });
}

QDockWidget* igQtMainWindow::shellDockForLeftPanel(LeftToolPanelId id) const {
    switch (id) {
    case LeftToolPanelId::Scalar: return ui->dockWidget_ScalarField;
    case LeftToolPanelId::Vector: return ui->dockWidget_VectorField;
    case LeftToolPanelId::Tensor: return ui->dockWidget_TensorField;
    case LeftToolPanelId::Flow: return ui->dockWidget_FlowField;
    case LeftToolPanelId::ContourExtract: return ui->dockWidget_ContourExtract;
    case LeftToolPanelId::Slice: return SliceDockWidget;
    case LeftToolPanelId::Deformation: return DeformationDockWidget;
    case LeftToolPanelId::Selection: return ui->dockWidget_SelectionField;
    case LeftToolPanelId::VariableDensity: return ui->dockWidget_VariableDensityField;
    case LeftToolPanelId::DataChange: return ui->dockWidget_DataChangeField;
    case LeftToolPanelId::Count: return nullptr;
    }
    return nullptr;
}

QWidget* igQtMainWindow::wrapContentInScrollArea(QWidget* content, QWidget* parent, bool centerFlowField) {
    if (!content) return nullptr;
    if (qobject_cast<QScrollArea*>(content)) return content;
    content->setMinimumHeight(0);
    content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto* scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(content);
    if (centerFlowField) scroll->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    return scroll;
}

void igQtMainWindow::applyLeftToolStackVerticalSplit() {
    if (!m_leftFieldDock || !modelTreeWidget) return;
    QDockWidget* props = modelTreeWidget->getPropertiesDock();
    if (!props || !m_leftFieldDock->isVisible()) return;
    // 工具面板 : Properties = 1:1（事件循环跑完后再调，否则常不生效）
    resizeDocks({m_leftFieldDock, props}, {1, 1}, Qt::Vertical);
}

void igQtMainWindow::relocateContentToLeftTab(QDockWidget* shell, QWidget* inner, const QString& title, LeftToolPanelId id,
                                              bool centerFlowField) {
    const int pid = static_cast<int>(id);
    if (!m_leftFieldTabs || !m_leftFieldDock || !inner) return;
    if (m_leftToolTabByPanel[static_cast<size_t>(pid)] >= 0) {
        const int t = m_leftToolTabByPanel[static_cast<size_t>(pid)];
        if (t < m_leftFieldTabs->count()) m_leftFieldTabs->setCurrentIndex(t);
        m_leftFieldDock->show();
        m_leftFieldDock->raise();
        return;
    }
    if (shell) {
        QWidget* top = shell->widget();
        if (top == inner) {
            shell->setWidget(nullptr);
            removeDockWidget(shell);
            shell->hide();
        } else if (auto* sa = qobject_cast<QScrollArea*>(top)) {
            if (sa->widget() == inner) {
                sa->takeWidget();
                shell->setWidget(nullptr);
                delete sa;
                removeDockWidget(shell);
                shell->hide();
            }
        }
    }
    const bool firstTabInStack = (m_leftFieldTabs->count() == 0);
    QWidget* page = wrapContentInScrollArea(inner, m_leftFieldTabs, centerFlowField);
    const int idx = m_leftFieldTabs->addTab(page, title);
    m_leftToolTabByPanel[static_cast<size_t>(pid)] = idx;
    m_leftFieldDock->show();
    m_leftFieldDock->raise();
    m_leftFieldTabs->setCurrentIndex(idx);
    if (firstTabInStack) {
        QTimer::singleShot(0, this, [this]() { applyLeftToolStackVerticalSplit(); });
    }
}

void igQtMainWindow::openLeftToolPanel(LeftToolPanelId id) {
    switch (id) {
    case LeftToolPanelId::Scalar:
        relocateContentToLeftTab(ui->dockWidget_ScalarField, ui->widget_ScalarField, QStringLiteral("标量场"), id, false);
        break;
    case LeftToolPanelId::Vector:
        relocateContentToLeftTab(ui->dockWidget_VectorField, ui->widget_VectorField, QStringLiteral("矢量场"), id, false);
        ui->widget_VectorField->updateVectorNameList();
        break;
    case LeftToolPanelId::Tensor:
        relocateContentToLeftTab(ui->dockWidget_TensorField, ui->widget_TensorField, QStringLiteral("张量场"), id, false);
        ui->widget_TensorField->InitTensorWidget();
        break;
    case LeftToolPanelId::Flow:
        relocateContentToLeftTab(ui->dockWidget_FlowField, ui->widget_FlowField, QStringLiteral("流场"), id, true);
        ui->widget_FlowField->updateVectorNameList();
        break;
    case LeftToolPanelId::ContourExtract:
        relocateContentToLeftTab(ui->dockWidget_ContourExtract, ui->widget_ContourExtract, QStringLiteral("轮廓提取"), id, false);
        break;
    case LeftToolPanelId::Slice:
        relocateContentToLeftTab(SliceDockWidget, SliceWidget, QStringLiteral("网格切面"), id, false);
        break;
    case LeftToolPanelId::Deformation:
        relocateContentToLeftTab(DeformationDockWidget, DeformationWidget, QStringLiteral("结构形变"), id, false);
        break;
    case LeftToolPanelId::Selection:
        relocateContentToLeftTab(ui->dockWidget_SelectionField, ui->widget_SelectionField, QStringLiteral("选择"), id, false);
        break;
    case LeftToolPanelId::VariableDensity:
        relocateContentToLeftTab(ui->dockWidget_VariableDensityField, ui->widget_VariableDensityField,
                                 QStringLiteral("变量数据密度"), id, false);
        break;
    case LeftToolPanelId::DataChange:
        relocateContentToLeftTab(ui->dockWidget_DataChangeField, ui->widget_DataChangeField, QStringLiteral("路径图"), id,
                                 false);
        break;
    case LeftToolPanelId::Count:
        break;
    }
}

void igQtMainWindow::onLeftToolTabCloseRequested(int index) {
    if (index < 0) return;
    for (size_t i = 0; i < m_leftToolTabByPanel.size(); ++i) {
        if (m_leftToolTabByPanel[i] == index) {
            closeLeftToolPanel(static_cast<LeftToolPanelId>(i));
            return;
        }
    }
}

void igQtMainWindow::closeLeftToolPanel(LeftToolPanelId id) {
    const int pid = static_cast<int>(id);
    if (!m_leftFieldTabs) return;
    int idx = m_leftToolTabByPanel[static_cast<size_t>(pid)];
    if (idx < 0 || idx >= m_leftFieldTabs->count()) return;

    QWidget* page = m_leftFieldTabs->widget(idx);
    auto* scroll = qobject_cast<QScrollArea*>(page);
    QWidget* inner = scroll ? scroll->takeWidget() : nullptr;
    if (scroll) scroll->deleteLater();

    QDockWidget* shell = shellDockForLeftPanel(id);
    if (shell && inner) {
        shell->setWidget(inner);
        if (id == LeftToolPanelId::Deformation)
            addDockWidget(Qt::RightDockWidgetArea, shell);
        else
            addDockWidget(Qt::LeftDockWidgetArea, shell);
        shell->hide();
    }

    m_leftFieldTabs->removeTab(idx);
    m_leftToolTabByPanel[static_cast<size_t>(pid)] = -1;
    for (size_t i = 0; i < m_leftToolTabByPanel.size(); ++i) {
        int& t = m_leftToolTabByPanel[i];
        if (t == idx) t = -1;
        else if (t > idx) --t;
    }
    if (id == LeftToolPanelId::Slice && rendererWidget && rendererWidget->getInteractor() &&
        !rendererWidget->getInteractor()->IsBasicStyle()) {
        rendererWidget->getInteractor()->RequestBasicStyle();
    }
    if (id == LeftToolPanelId::Deformation && ui->action_deformation) ui->action_deformation->setChecked(false);
    if (id == LeftToolPanelId::Selection && ui->action_SelectView) ui->action_SelectView->setChecked(false);
    if (m_leftFieldTabs->count() == 0 && m_leftFieldDock) m_leftFieldDock->hide();
}

void igQtMainWindow::initAllMySignalConnections() {
    // connect(rendererWidget, &igQtModelDrawWidget::insertToModelListView,
    // ui->modelTreeView, &igQtModelListView::InsertModel);

    connect(fileLoader, &igQtFileLoader::NewModel, modelTreeWidget, &igQtModelDialogWidget::addDataObjectToModelTree);
    connect(fileLoader, &igQtFileLoader::FinishReading, this, &igQtMainWindow::updateRecentFilePaths);
    connect(ui->action_DeleteMesh, &QAction::triggered, modelTreeWidget, &igQtModelDialogWidget::deleteCurrentModel);

    connect(ui->action_DeleteMesh, &QAction::triggered, this, [&](bool){
        if (vortexMetricsLabel) {
            vortexMetricsLabel->clear();
            vortexMetricsLabel->hide();
        }
    });

    // connect(fileLoader, &igQtFileLoader::FinishReading, this,
    // &igQtMainWindow::updateViewStyleAndCloudPicture); connect(fileLoader,
    // &igQtFileLoader::FinishReading, this,
    // &igQtMainWindow::updateCurrentSceneWidget);

    connect(fileLoader, &igQtFileLoader::FinishReading, ui->widget_Animation, [&](){
        ui->widget_Animation->initAnimationComponents();
    });
    connect(fileLoader, &igQtFileLoader::FinishReading, DeformationWidget, &igQtDeformationWidget::updateInfo);

    connect(fileLoader, &igQtFileLoader::FinishReading, this, [&]() {
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        if (!scene) return;

        auto model = scene->GetCurrentModel();
        if (!model) return;

        auto dataObject = model->GetDataObject();
        if (!dataObject) return;

        auto attributeSet = dataObject->GetAttributeSet();
        if (!attributeSet) return;

        auto allAttributes = attributeSet->GetAllAttributes();
        if (!allAttributes || allAttributes->GetNumberOfElements() == 0) return;

        auto drawObject = DynamicCast<DrawObject>(dataObject);
        if (drawObject) {
            auto item = modelTreeWidget->getItemFromObject(dataObject);
            if (item && item->childCount() > 0) {
                item->setExpanded(true);
                auto child = item->child(0);
                item->setCurrentChild(child);
                item->setSelected(false);
                item->viewAttribute(0, -1);
                child->setSelected(true);
                modelTreeWidget->setCurrentItem(child);
            }
        }
    });

    connect(ui->widget_FlowField, &igQtStreamTracerWidget::AddStreamObject, this, [&](iGame::DataObject::Pointer res) {
        streamTreeIndex=modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        scene->GetCurrentModel()->SetViewWireframeSwitch(true);
    });
    connect(ui->widget_FlowField, &igQtStreamTracerWidget::UpdateStreamObject, this,
            [&](iGame::DataObject::Pointer res) {
                auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                if (scene->GetCurrentModelID() == streamTreeIndex) {
                    modelTreeWidget->updateCurrentModelProperty();
                }
                modelTreeWidget->updateAllAttriubute(res);
                rendererWidget->update();
                auto drawObject = DynamicCast<DrawObject>(res);
                if (drawObject) {
                    auto item = modelTreeWidget->getItemFromObject(res);
                    if (item && item->childCount() > 0) {
                        item->setExpanded(true);
                        auto child = item->child(0);
                        item->setCurrentChild(child);
                        item->setSelected(false);
                        item->viewAttribute(0, -1);
                        child->setSelected(true);
                        modelTreeWidget->setCurrentItem(child);
                    }
                }
            });


    connect(fileLoader, &igQtFileLoader::AddFileToModelList, ui->modelTreeView, &igQtModelListView::AddModel);


    /* Animation signal connect BEGIN.*/
    connect(ui->widget_Animation, &igQtAnimationWidget::UpdateScene,
            this, &igQtMainWindow::UpdateRenderingWidget);
    // Update scalar view UI when animation frame changes (updates DataRange slider and info label)
    connect(ui->widget_Animation, &igQtAnimationWidget::AnimationFrameChanged,
            ui->widget_ScalarField, &igQtScalarViewWidget::showScalarView);

    connect(ui->widget_Animation, &igQtAnimationWidget::AnimationFrameChanged,
            this, [&](){
                ui->widget_VectorField->drawV();
            });

    connect(ui->widget_Animation, &igQtAnimationWidget::AnimationFrameChanged, this, [&](){
        SliceWidget->ClipModel();
//        SliceWidget->UpdateOriginDataObject()
    });
//    connect(ui->widget_Animation, &igQtAnimationWidget::AnimationFrameChanged,
//            DeformationWidget, &igQtDeformationWidget::updateInfo);

    //connect(ui->widget_QualityDetection,
    //&igQtQualityDetectionWidget::updateCurrentModelColor, rendererWidget,
    //&igQtModelDrawWidget::UpdateCurrentModel);
    connect(ui->widget_ScalarField, &igQtScalarViewWidget::changeColorBarShow, this,
            &igQtMainWindow::updateColorBarShow);
    /* Animation signal connect END.*/


    /* Model Tree signal connect BEGIN.*/
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CloudPictureChanged, ui->widget_ScalarField,
            &igQtScalarViewWidget::showScalarView);
    // Update Deformation Info when model is deleted
    connect(this->modelTreeWidget, &igQtModelDialogWidget::ModelDeleted,
            DeformationWidget, &igQtDeformationWidget::updateInfo);
    connect(this->modelTreeWidget, &igQtModelDialogWidget::ModelDeleted,
            ui->widget_Animation, &igQtAnimationWidget::initAnimationComponents);

    // Update animation controls when model changes
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged,
            ui->widget_Animation, &igQtAnimationWidget::initAnimationComponents);
    // Update Deformation Info when model changes
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged,
            DeformationWidget, &igQtDeformationWidget::updateInfo);

    /* Model Tree signal connect END.*/

    connect(ui->widget_ScalarField, &igQtScalarViewWidget::ChangeShowColorManager, this, [&]() {
        if (this->ColorManagerWidget->isHidden()) {
            this->ColorManagerWidget->resetColorRange();
            this->ColorManagerWidget->show();
        } else {
            this->ColorManagerWidget->hide();
        }
    });

    connect(this->ColorManagerWidget, &igQtColorManagerWidget::UpdateColorBarFinished, this, [&]() {
        ui->widget_ScalarField->updateDrawStyle();
        this->rendererWidget->getColorBarWidget()->update();
    });

    connect(ui->widget_VectorField, &igQtVectorWidget::DrawDireVector, this, [&](iGame::DataObject::Pointer res) {
        modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
    });
    connect(ui->widget_VectorField, &igQtVectorWidget::UpdateDireVector, this, [&](iGame::DataObject::Pointer res) {
        res->Modified();
        modelTreeWidget->updateItemName(res);
        rendererWidget->update();
    });
    connect(ui->widget_TensorField, &igQtTensorWidget::DrawTensorGlyphs, this, [&](iGame::DataObject::Pointer res) {
        modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
    });
    connect(ui->widget_TensorField, &igQtTensorWidget::UpdateTensorGlyphs, this,
            [&](iGame::DataObject::Pointer res) { rendererWidget->update(); });
    connect(ui->widget_TensorField, &igQtTensorWidget::UpdateAttributes, this,
            [&](iGame::DataObject::Pointer res) { modelTreeWidget->updateAllAttriubute(res); });

    connect(ui->widget_ContourExtract, &igQtContourExtractWidget::DrawContourModel, this,
            [&](iGame::DataObject::Pointer res) {
                modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
            });
    connect(ui->widget_ContourExtract, &igQtContourExtractWidget::UpdateContourModel, this,
            [&](DataObject::Pointer mesh) {
                modelTreeWidget->updateCurrentModelInfo();
                rendererWidget->update();
            });
    // reset clipping
    connect(ui->action_ResetClipping, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto inputMesh = DynamicCast<iGame::DrawObject>(scene->GetCurrentModel()->GetDataObject());
        if (!inputMesh->GetClipped()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        inputMesh->GetClipper()->DisableAll();
        inputMesh->SetVisibility(true);

        const std::string OVName = "__" + inputMesh->GetName() + "_OV";   // 临时模型
        const std::string IVName = "__" + inputMesh->GetName() + "_IV";   // 临时模型
        const std::string OIVName = "__" + inputMesh->GetName() + "_OIV"; // 临时模型
        bool exist[3]{false, false, false};
        //for (auto& [id, model]: scene->GetModelList()) {
        //    if (model->GetDataObject()->GetName() == OVName) {
        //        auto model = scene->GetModelById(id);
        //        model->GetDataObject()->SetVisibility(false);
        //    } else if (model->GetDataObject()->GetName() == IVName) {
        //        auto model = scene->GetModelById(id);
        //        model->GetDataObject()->SetVisibility(false);
        //    } else if (model->GetDataObject()->GetName() == OIVName) {
        //        auto model = scene->GetModelById(id);
        //        model->GetDataObject()->SetVisibility(false);
        //    }
        //}
        auto modelList = scene->GetModelList();
        for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
            auto id = it->first;
            auto model = it->second;

            auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());

            if (drawObject->GetName() == OVName) {
                drawObject->SetVisibility(false);
            } else if (drawObject->GetName() == IVName) {
                drawObject->SetVisibility(false);
            } else if (drawObject->GetName() == OIVName) {
                drawObject->SetVisibility(false);
            }
        }

        rendererWidget->update();
    });


    // box clipping
    connect(ui->action_BoxClipping, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        bool ok;

        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto inputMesh = DynamicCast<iGame::DrawObject>(scene->GetCurrentModel()->GetDataObject());
        if (!inputMesh->GetClipped()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        auto box = inputMesh->GetBoundingBox();
        auto center = (box.min + box.max) * 0.5;
        auto size = box.max - box.min;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("盒子切割"));
        int x_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_min(0..1)", "0.0");
        int y_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_min(0..1)", "0.0");
        int z_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_min(0..1)", "0.0");
        int x_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_max(0..1)", "0.5");
        int y_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_max(0..1)", "1.0");
        int z_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_max(0..1)", "1.0");
        int flip_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip", "false");
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok;
            auto Clamp = [](double x, double l, double r) -> double {
                if (x < l) return l;
                if (x > r) return r;
                return x;
            };

            auto clipper = inputMesh->GetClipper();

            auto& cbox = clipper->m_Box;
            cbox.m_Use = true;

            cbox.m_Bmin[0] = box.min[0] + size[0] * Clamp(dialog->getDouble(x_min_id, ok), 0., 1.);
            cbox.m_Bmin[1] = box.min[1] + size[1] * Clamp(dialog->getDouble(y_min_id, ok), 0., 1.);
            cbox.m_Bmin[2] = box.min[2] + size[2] * Clamp(dialog->getDouble(z_min_id, ok), 0., 1.);
            cbox.m_Bmax[0] = box.min[0] + size[0] * Clamp(dialog->getDouble(x_max_id, ok), 0., 1.);
            cbox.m_Bmax[1] = box.min[1] + size[1] * Clamp(dialog->getDouble(y_max_id, ok), 0., 1.);
            cbox.m_Bmax[2] = box.min[2] + size[2] * Clamp(dialog->getDouble(z_max_id, ok), 0., 1.);
            cbox.m_Flip = dialog->getChecked(flip_id, ok);

            clipper->Modified();

            rendererWidget->update();
        });
    });


    // plane clipping
    connect(ui->action_PlaneClipping, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        bool ok;

        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

        auto inputMesh = DynamicCast<iGame::DrawObject>(scene->GetCurrentModel()->GetDataObject());
        if (!inputMesh->GetClipped()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        auto box = inputMesh->GetBoundingBox();
        auto center = (box.min + box.max) * 0.5;
        auto size = box.max - box.min;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("平面切割"));
        int origin_x_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_x(0..1)", "0.5");
        int origin_y_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_y(0..1)", "0.5");
        int origin_z_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_z(0..1)", "0.5");
        int normal_x_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_x(-1..1)", "1.0");
        int normal_y_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_y(-1..1)", "0.0");
        int normal_z_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_z(-1..1)", "0.0");
        int flip_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip", "false");
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok;
            auto Clamp = [](double x, double l, double r) -> double {
                if (x < l) return l;
                if (x > r) return r;
                return x;
            };

            auto clipper = inputMesh->GetClipper();

            auto& cplane = clipper->m_Plane;
            cplane.m_Use = true;

            cplane.m_Origin[0] = box.min[0] + size[0] * Clamp(dialog->getDouble(origin_x_id, ok), 0., 1.);
            cplane.m_Origin[1] = box.min[1] + size[1] * Clamp(dialog->getDouble(origin_y_id, ok), 0., 1.);
            cplane.m_Origin[2] = box.min[2] + size[2] * Clamp(dialog->getDouble(origin_z_id, ok), 0., 1.);
            cplane.m_Normal[0] = Clamp(dialog->getDouble(normal_x_id, ok), -1., 1.);
            cplane.m_Normal[1] = Clamp(dialog->getDouble(normal_y_id, ok), -1., 1.);
            cplane.m_Normal[2] = Clamp(dialog->getDouble(normal_z_id, ok), -1., 1.);
            cplane.m_Flip = dialog->getChecked(flip_id, ok);
            if (cplane.m_Normal[0] == 0. && cplane.m_Normal[1] == 0. && cplane.m_Normal[2] == 0.) {
                std::cout << "Normal is a vector of zero" << std::endl;
                return;
            }

            clipper->Modified();

            rendererWidget->update();
        });
    });
}
void igQtMainWindow::updateRecentFilePaths() {
    ui->menu_RecentFiles->clear();
    auto recentFileActions = fileLoader->GetRecentActionList();
    for (auto i = recentFileActions.size() - 1; i >= 0; i--) {
        ui->menu_RecentFiles->addAction(recentFileActions.at(i));
    }
}
void igQtMainWindow::updateColorBarShow() {
    auto colorBar = this->rendererWidget->getColorBarWidget();
    if (!colorBar) { return; }
    colorBar->update();
    if (colorBar->isHidden()) {
        colorBar->show();
    } else {
        colorBar->hide();
    }
}

void igQtMainWindow::initAllSources() {
    // connect(ui->action_LineSource, &QAction::triggered, this, [&]() {
    //	UnstructuredMesh::Pointer newLinePointSet = UnstructuredMesh::New();
    //	newLinePointSet->SetViewStyle(IG_POINTS);
    //	newLinePointSet->AddPoint(Point(0.f, 0.f, 0.f));
    //	newLinePointSet->AddPoint(Point(1.f, 1.0f, 1.f));
    //	igIndex cell[1] = { 0 };
    //	newLinePointSet->AddCell(cell, 1, IG_VERTEX);
    //	cell[0] = 1;
    //	newLinePointSet->AddCell(cell, 1, IG_VERTEX);
    //	auto curScene = SceneManager::Instance()->GetCurrentScene();

    //	LineTypePointsSource::Pointer lineSource = LineTypePointsSource::New();

    //	lineSource->SetInput(newLinePointSet);
    //	lineSource->SetResolution(20);
    //	lineSource->GetOutput()->SetName("lineSource");

    //	auto model = curScene->CreateModel(lineSource->GetOutput());
    //	modelTreeWidget->addModelToModelTree(model);
    //	auto interactor = LineSourceInteractor::New();

    //	//        auto interactor = PointDragInteractor::New();
    //	interactor->SetPointSet(DynamicCast<PointSet>(SceneManager::Instance()
    //		->GetCurrentScene()
    //		->GetCurrentModel()
    //		->GetDataObject()));

    //	rendererWidget->ChangeInteractor(interactor);
    //	});
}

void igQtMainWindow::initAllInteractor() {
    connect(ui->action_SelectView, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            openLeftToolPanel(LeftToolPanelId::Selection);
            if (ui->widget_SelectionField) ui->widget_SelectionField->setFocus(Qt::OtherFocusReason);
        } else {
            closeLeftToolPanel(LeftToolPanelId::Selection);
        }
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::Signal_SetSelectionStationChanged, this, [&]() {
        if (!iGame::SelectionParameter::Instance().GetInSelection()) {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
            auto removeBoxFunc = [&]() {
                auto scene = rendererWidget->GetScene();
                SelectionParameter::Instance().SetHaveBox(false);
                scene->GetInteractor()->RemoveSepcialInteractor("SelectBox");
                rendererWidget->update();
            };
            removeBoxFunc();
            return;
        }
        auto selectionStation = iGame::SelectionParameter::Instance().GetSelectionStation();
        switch (selectionStation) {
            case iGame::SelectionParameter::SelectionStation::NONE_SELECTION:
                ui->widget_SelectionField->SetVariableNames({});
                break;
            case iGame::SelectionParameter::SelectionStation::POINT_SELECTION: {
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) {
                    ui->widget_SelectionField->SetVariableNames({});
                } else {
                    auto attrs = model->GetDataObject()->GetAttributeSet()->GetAllAttributes();
                    auto variableNames = CtxPresObjData_Main::GenerateVariableNames(attrs, IG_POINT);
                    ui->widget_SelectionField->SetVariableNames(variableNames);
                }
            } break;
            case iGame::SelectionParameter::SelectionStation::CELL_SELECTION: {
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) {
                    ui->widget_SelectionField->SetVariableNames({});
                } else {
                    auto attrs = model->GetDataObject()->GetAttributeSet()->GetAllAttributes();
                    auto variableNames = CtxPresObjData_Main::GenerateVariableNames(attrs, IG_CELL);
                    ui->widget_SelectionField->SetVariableNames(variableNames);
                }
            } break;
            default:
                break;
        }

        static auto PreVisitFunc = [](iGame::Model::Pointer model) {
            if (model == nullptr) return;
            auto dataObj = model->GetDataObject();
            if (dataObj == nullptr) return;
            auto type = dataObj->GetDataObjectType();
            switch (type) {
                case IG_SURFACE_MESH:
                case IG_STRUCTURED_MESH:
                case IG_VOLUME_MESH: {
                    auto buildAdjacencyRelationFilter = BuildAdjacencyRelationFilter::New();
                    buildAdjacencyRelationFilter->SetInput(dataObj);
                    buildAdjacencyRelationFilter->Execute();
                } break;
                case IG_UNSTRUCTURED_MESH: {
                    auto mesh = DynamicCast<UnstructuredMesh>(dataObj);
                    if (mesh == nullptr) return;
                    auto selection = mesh->GetSelection();
                    if (selection == nullptr) return;
                    auto& cellFaceExtracter = selection->GetCellFaceExtracter();
                    cellFaceExtracter.PreVisit(mesh);
                } break;
                default:
                    return;
            }
        };

        switch (selectionStation) {
            case iGame::SelectionParameter::SelectionStation::NONE_SELECTION:
                rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
                break;
            case iGame::SelectionParameter::SelectionStation::POINT_SELECTION: {
                rendererWidget->ChangeInteractorStyle(Interactor::SinglePointSelectionStyle);
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                PreVisitFunc(model);
            } break;
            case iGame::SelectionParameter::SelectionStation::CELL_SELECTION: {
                rendererWidget->ChangeInteractorStyle(Interactor::SingleFaceSelectionStyle);
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                PreVisitFunc(model);
            } break;
            default:
                break;
        }
    });
    //######### View Cloud Change ST #########
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CloudPictureChanged, this, [&]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        auto dataObj = model->GetDataObject();
        if (dataObj == nullptr) return;
        auto attrSet = dataObj->GetAttributeSet();
        if (attrSet == nullptr) return;
        auto allAttr = attrSet->GetAllAttributes();
        if (allAttr == nullptr) return;
        auto currentAttributeIndex = dataObj->GetCurrentAttributeIndex();
        if (currentAttributeIndex < 0 || allAttr->Size() <= currentAttributeIndex) return;
        auto& currentAttr = allAttr->GetElement(currentAttributeIndex);
        auto dataType = currentAttr.GetAttachmentType();
        auto currentAttributeDim = dataObj->GetCurrentAttributeDimension();
        int variableIndex = 0;
        for (int attrIndex = 0; attrIndex < currentAttributeIndex; attrIndex++) {
        //for (int attrIndex = 0; attrIndex < allAttr->Size(); attrIndex++) {
            auto& attr = allAttr->GetElement(attrIndex);
            if (attr.attachmentType != dataType) continue;
            auto dim = attr.pointer->GetDimension();
            variableIndex += ((dim == 1) ? 1 : dim + 1);
        }
        //if the attribute is not scalar, Extra plus one
        variableIndex += ((currentAttr.pointer->GetDimension() == 1) ? currentAttributeDim : currentAttributeDim + 1);

        auto variableNames = CtxPresObjData_Main::GenerateVariableNames(allAttr, dataType);
        ui->widget_SelectionField->SetVariableNames(variableNames);

        ui->widget_SelectionField->SetCurrentVariable(dataType, variableIndex);
        });
    
    //######### View Cloud Change ED #########
    connect(ui->widget_FlowField, &igQtStreamTracerWidget::SetSelectItemShow, this, [&](bool visiable) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        auto selection = model->GetSelection();
        if (selection == nullptr) return;
        selection->SetSelectItemVisable(visiable);
        rendererWidget->update();
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::SetSelectItemShow, this, [&](bool visiable) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        auto selection = model->GetSelection();
        if (selection == nullptr) return;
        selection->SetSelectItemVisable(visiable);
        rendererWidget->update();
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::SetClearSelection, this, [&]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        auto selection = model->GetSelection();
        if (selection == nullptr) return;
        selection->Reset();
        rendererWidget->update();
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::SetClearBox, this, [&]() {
        auto scene = rendererWidget->GetScene();
        SelectionParameter::Instance().SetHaveBox(false);
        scene->GetInteractor()->RemoveSepcialInteractor("SelectBox");
        rendererWidget->update();
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::SetBoxSettingDialog, this, [&]() {
        auto scene = rendererWidget->GetScene();
        auto interactor = scene->GetInteractor();
        if (!SelectionParameter::Instance().GetHaveBox()) return;
        auto basicStyle = interactor->GetSpecialInteractor("SelectBox");
        if (basicStyle == nullptr) return;
        auto boxStyle = DynamicCast<iGame::BoxStyle>(basicStyle);
        if (boxStyle == nullptr) return;
        auto dynamicBox = boxStyle->GetBox();
        if (dynamicBox == nullptr) return;
        ui->widget_SelectionField->SetInitBoxSettingDialog(rendererWidget);
    });
    connect(ui->widget_FlowField, &igQtStreamTracerWidget::SetUseBox, this, [&](Model::Pointer model) {
        // model = rendererWidget-> GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        auto dataObj = model->GetDataObject();
        if (dataObj == nullptr) return;
        auto selection = model->GetSelection();
        if (selection == nullptr) return;
        auto scene = rendererWidget->GetScene();
        auto interactor = scene->GetInteractor();
        if (!SelectionParameter::Instance().GetHaveBox()) return;
        auto basicStyle = interactor->GetSpecialInteractor("SelectBox");
        if (basicStyle == nullptr) return;
        auto boxStyle = DynamicCast<iGame::BoxStyle>(basicStyle);
        if (boxStyle == nullptr) return;
        auto dynamicBox = boxStyle->GetBox();
        if (dynamicBox == nullptr) return;
        auto faces = dynamicBox->GetAllFaces();
        auto meshType = dataObj->GetDataObjectType();
        switch (meshType) {
            case IG_SURFACE_MESH: {
                auto mesh = DynamicCast<SurfaceMesh>(dataObj);
                mesh->RequestEditStatus();
                auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                selection->SelectionCallBackEvent(IG_POINT, pointIds,  Selection::Operate::Add);
            } break;
            case IG_VOLUME_MESH: {
                auto mesh = DynamicCast<VolumeMesh>(dataObj);
                mesh->RequestEditStatus();
                auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                selection->SelectionCallBackEvent(IG_POINT, pointIds,Selection::Operate::Add);
            } break;
            case IG_UNSTRUCTURED_MESH: {
                auto mesh = DynamicCast<UnstructuredMesh>(dataObj);
                    auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_POINT, pointIds, Selection::Operate::Add );
            } break;
            default:
                return;
        }
        rendererWidget->update();
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::SetUseBox, this, [&]() {
        if (!SelectionParameter::Instance().GetInSelection()) return;
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        auto dataObj = model->GetDataObject();
        if (dataObj == nullptr) return;
        auto selection = model->GetSelection();
        if (selection == nullptr) return;
        auto scene = rendererWidget->GetScene();
        auto interactor = scene->GetInteractor();
        if (!SelectionParameter::Instance().GetHaveBox()) return;
        auto basicStyle = interactor->GetSpecialInteractor("SelectBox");
        if (basicStyle == nullptr) return;
        auto boxStyle = DynamicCast<iGame::BoxStyle>(basicStyle);
        if (boxStyle == nullptr) return;
        auto dynamicBox = boxStyle->GetBox();
        if (dynamicBox == nullptr) return;
        auto faces = dynamicBox->GetAllFaces();
        auto meshType = dataObj->GetDataObjectType();
        switch (meshType) {
            case IG_SURFACE_MESH: {
                auto mesh = DynamicCast<SurfaceMesh>(dataObj);
                mesh->RequestEditStatus();
                if (iGame::SelectionParameter::Instance().GetSelectionStation() ==
                    iGame::SelectionParameter::SelectionStation::CELL_SELECTION) {
                    auto cellIds = iGame::SingleSelectionStyle::GetCellsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_CELL, cellIds,
                                                      SelectionParameter::Instance().GetSelectOrUnSelect()
                                                              ? Selection::Operate::Add
                                                              : Selection::Operate::Remove);
                } else {
                    auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_POINT, pointIds,
                                                      SelectionParameter::Instance().GetSelectOrUnSelect()
                                                              ? Selection::Operate::Add
                                                              : Selection::Operate::Remove);
                }
            } break;
            case IG_STRUCTURED_MESH:
            case IG_VOLUME_MESH: {
                auto mesh = DynamicCast<VolumeMesh>(dataObj);
                mesh->RequestEditStatus();
                if (iGame::SelectionParameter::Instance().GetSelectionStation() ==
                    iGame::SelectionParameter::SelectionStation::CELL_SELECTION) {
                    auto cellIds = iGame::SingleSelectionStyle::GetCellsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_CELL, cellIds,
                                                      SelectionParameter::Instance().GetSelectOrUnSelect()
                                                              ? Selection::Operate::Add
                                                              : Selection::Operate::Remove);
                } else {
                    auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_POINT, pointIds,
                                                      SelectionParameter::Instance().GetSelectOrUnSelect()
                                                              ? Selection::Operate::Add
                                                              : Selection::Operate::Remove);
                }
            } break;
            case IG_UNSTRUCTURED_MESH: {
                auto mesh = DynamicCast<UnstructuredMesh>(dataObj);
                if (iGame::SelectionParameter::Instance().GetSelectionStation() ==
                    iGame::SelectionParameter::SelectionStation::CELL_SELECTION) {
                    auto cellIds = iGame::SingleSelectionStyle::GetCellsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_CELL, cellIds,
                                                      SelectionParameter::Instance().GetSelectOrUnSelect()
                                                              ? Selection::Operate::Add
                                                              : Selection::Operate::Remove);
                } else {
                    auto pointIds = iGame::SingleSelectionStyle::GetPointsInBox(
                            faces, mesh, SelectionParameter::Instance().GetSelectOnlySelectSeeAbleCells());
                    selection->SelectionCallBackEvent(IG_POINT, pointIds,
                                                      SelectionParameter::Instance().GetSelectOrUnSelect()
                                                              ? Selection::Operate::Add
                                                              : Selection::Operate::Remove);
                }
            } break;
            default:
                return;
        }
        rendererWidget->update();
    });

    connect(ui->widget_SelectionField, &igQtSelectionWidget::Hided, this, [this]() {
        if (m_leftToolTabByPanel[static_cast<size_t>(LeftToolPanelId::Selection)] >= 0)
            closeLeftToolPanel(LeftToolPanelId::Selection);
        ui->action_SelectView->setChecked(false);
        auto scene = rendererWidget->GetScene();
        SelectionParameter::Instance().SetHaveBox(false);
        scene->GetInteractor()->RemoveSepcialInteractor("SelectBox");
        ui->widget_SelectionField->PreventSignalSend(true);
        ui->widget_SelectionField->SetDefaultSelectionButton();
        ui->widget_SelectionField->PreventSignalSend(false);
        rendererWidget->update();
    });

    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [&]() {
        ui->widget_SelectionField->PreventSignalSend(true);
        ui->widget_SelectionField->SetDefaultSelectionButton();
        ui->widget_SelectionField->PreventSignalSend(false);
        //####### ATTENTION #######
        auto attenetionFunc = [&]() {
            ui->widget_SelectionField->SetNoAttention();
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (model == nullptr) return;
            auto dataObj = model->GetDataObject();
            if (dataObj == nullptr) return;
            auto attributeSet = dataObj->GetAttributeSet();
            if (attributeSet == nullptr) return;
            bool haveNoPointAttr = (attributeSet->GetAllPointAttributes()->GetNumberOfElements() == 0);
            bool haveNoCellAttr = (attributeSet->GetAllCellAttributes()->GetNumberOfElements() == 0);
            if (haveNoPointAttr && haveNoCellAttr) {
                ui->widget_SelectionField->SetAllAttention();
            } else if (haveNoPointAttr) {
                ui->widget_SelectionField->SetPointAttention();
            } else if (haveNoCellAttr) {
                ui->widget_SelectionField->SetCellAttention();
            }
        };
        attenetionFunc();
        //####### SelectFunc #######
        auto selectFunc = [&]() {
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (model == nullptr) return;
            auto selection = model->GetSelection();
            if (selection == nullptr) return;
            ui->widget_SelectionField->SetBoxInitCallBackFunc(selection);
        };
        selectFunc();
        return;
        //auto radius = ui->widget_SelectionField->GetSelectionRadius();
        //auto selectionStation = ui->widget_SelectionField->GetSelectionStation();
        //auto selectOrUnSelect = ui->widget_SelectionField->GetSelectOrUnSelect();
        //switch (selectionStation) {
        //    case SelectionStation::NONE_SELECTION:
        //        rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        //        break;
        //    case SelectionStation::POINT_SELECTION:
        //        rendererWidget->ChangeInteractorStyle(Interactor::SinglePointSelectionStyle, radius, selectOrUnSelect);
        //        break;
        //    case SelectionStation::CELL_SELECTION:
        //        rendererWidget->ChangeInteractorStyle(Interactor::SingleFaceSelectionStyle, radius, selectOrUnSelect);
        //        break;
        //    default:
        //        break;
        //}
        //auto visiable = ui->widget_SelectionField->GetSelectItemShow();
        //auto model = rendererWidget->GetScene()->GetCurrentModel();
        //if (model == nullptr) return;
        //if (visiable) model->GetPainter3D()->ShowAll();
        //else
        //    model->GetPainter3D()->HideAll();
    });
    connect(ui->widget_ContextPreservingShowField, &igQtContextPreservingShowWidget::Hided, this,
            [&]() { ui->action_ContextPreserving->setChecked(false); });


    ui->action_select_point->setVisible(false);
    ui->action_select_face->setVisible(false);
    connect(ui->action_select_point, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_select_point->isChecked()) {
            if (ui->action_select_face->isChecked()) { ui->action_select_face->setChecked(false); }
            rendererWidget->ChangeInteractorStyle(Interactor::SinglePointSelectionStyle);
        } else {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
    });


    connect(ui->action_select_face, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_select_face->isChecked()) {
            if (ui->action_select_point->isChecked()) { ui->action_select_point->setChecked(false); }
            rendererWidget->ChangeInteractorStyle(Interactor::SingleFaceSelectionStyle);
        } else {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
    });

    connect(ui->action_drag_point, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_drag_point->isChecked()) {
            rendererWidget->ChangeInteractorStyle(Interactor::DragPointStyle);
        } else {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
    });
}

void igQtMainWindow::UpdateRenderingWidget() { rendererWidget->update(); }


QString igQtMainWindow::LoadExternalFonts() {
    int fontId = QFontDatabase::addApplicationFont(":/Styles/Styles/SourceHanSansCN-Normal.otf");
    if (fontId == -1) {
        qWarning() << "Failed to load font from resource :/Styles/SourceHanSansCN-Normal.otf";
        return QString();
    }

    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) {
        qWarning() << "No font families found in loaded font.";
        return QString();
    }

    const QString family = families.first();
    qDebug() << "Loaded font family:" << family;

    QFont     appFont(family);
    appFont.setPointSize(12);

    QApplication::setFont(appFont);

    return family;
}
void igQtMainWindow::rebuildActionsAsTwoRowWidget(QToolBar* toolbar, const QList<QAction*>& targetActions,
                                                  int columns, QAction* insertBefore) {
    if (!toolbar || targetActions.isEmpty())
        return;

    // 1. 先移除目标action（原逻辑保留）
    for (QAction* act : targetActions) {
        if (act)
            toolbar->removeAction(act);
    }

    // 2. 创建容器和布局（原逻辑保留，微调尺寸计算）
    QWidget* container = new QWidget(toolbar);
    QGridLayout* grid = new QGridLayout(container);
    QSize iconSize = toolbar->iconSize();
    const int gridSpacing = qMax(4, iconSize.height() / 6);
    grid->setSpacing(gridSpacing);
    grid->setContentsMargins(0, 0, 0, 0);

    // 两行视图按钮比默认更大，提升可见性和点击性
    const int targetIcon = qMax(20, static_cast<int>(iconSize.height() * 0.65));
    const int rowHeight = targetIcon + 8;
    const int containerHeight = 2 * rowHeight + gridSpacing;
    QSize btnSize(rowHeight, rowHeight);

    // 【修改1】放宽尺寸约束，避免被父布局挤压
    container->setMinimumHeight(containerHeight);
    container->setMinimumWidth(3 * rowHeight + 2 * gridSpacing); // 去掉fixedHeight，改用minimumHeight
    container->setObjectName("twoRowViewGrid");

    // 3. 构建两行按钮（原逻辑保留）
    int row = 0, col = 0;
    for (QAction* act : targetActions) {
        if (!act)
            continue;
        QToolButton* btn = new QToolButton(container);
        btn->setDefaultAction(act);
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setIconSize(QSize(targetIcon, targetIcon));
        btn->setAutoRaise(true);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        btn->setMinimumSize(btnSize);
        btn->setMaximumSize(btnSize);
        btn->setStyleSheet(R"(
            QToolButton { border: none; margin: 0; padding: 0; }
            QToolButton:hover { background-color: #3A3A3A; border-radius: 2px; }
            QToolButton:pressed { background-color: #4A4A4A; }
        )");
        grid->addWidget(btn, row, col, Qt::AlignVCenter | Qt::AlignHCenter);
        if (++col >= columns) {
            col = 0;
            ++row;
        }
    }

    // 4. 添加QWidgetAction到toolbar（原逻辑保留）
    QWidgetAction* widgetAction = new QWidgetAction(toolbar);
    widgetAction->setDefaultWidget(container);
    if (insertBefore && toolbar->actions().contains(insertBefore))
        toolbar->insertAction(insertBefore, widgetAction);
    else
        toolbar->addAction(widgetAction); // 【修改2】改用addAction，避免insert位置异常

    // 调试：确认container已添加（可选，测试后可删除）
    qDebug() << "两行按钮容器已创建：" << container->objectName() << "子控件数：" << container->children().count();
}

void igQtMainWindow::addToolbarTitle(QToolBar* toolbar, const QString& title) {
    if (!toolbar)
        return;

    Qt::ToolBarArea area = this->toolBarArea(toolbar);
    QSize iconSize = toolbar->iconSize();
    if (iconSize.width() <= 0)
        iconSize = QSize(60, 60);
    const ToolbarSpacingMetrics spacing = metricsForIconSize(iconSize.width());
    Qt::ToolButtonStyle btnStyle = toolbar->toolButtonStyle();
    const QList<QAction*> actions = toolbar->actions();

    QFont titleFont(QStringLiteral("PingFang SC"));
    titleFont.setPointSize(titlePointSizeForIcon(iconSize.width()));
    const int titleTextH = QFontMetrics(titleFont).height();
    const int totalH = iconSize.height() + spacing.verticalGap + titleTextH + spacing.bottomMargin;

    QWidget* container = new QWidget(this);
    container->setObjectName("toolbarContainer_" + toolbar->objectName());
    container->setMinimumSize(100, totalH);
    container->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // 【删除这行】container->setStyleSheet("background-color: #222222;");

    QWidget* topRow = new QWidget(container);
    topRow->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout* hLayout = new QHBoxLayout(topRow);
    hLayout->setContentsMargins(spacing.edgeMargin, 0, spacing.edgeMargin, 0);
    hLayout->setSpacing(spacing.btnGap);
    hLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    for (QAction* act : actions) {
        if (!act) continue;

        if (QWidgetAction* wa = qobject_cast<QWidgetAction*>(act)) {
            QWidget* w = wa->defaultWidget();
            if (w) {
                w->setParent(topRow);
                w->setVisible(true);
                w->show();
                w->setMinimumSize(w->minimumSizeHint());
                // 【删除下面这4行】
                // if (w->objectName() == "twoRowViewGrid") {
                //     w->setStyleSheet("background-color: #444444;");
                //     qDebug() << "转移后twoRowViewGrid尺寸：" << w->size() << "最小尺寸：" << w->minimumSize();
                // }
                hLayout->addWidget(w, 0, Qt::AlignLeft | Qt::AlignVCenter);
                continue;
            }
        }

        // 普通按钮逻辑（不变）
        QToolButton* b = new QToolButton(topRow);
        b->setDefaultAction(act);
        b->setIconSize(iconSize);
        b->setToolButtonStyle(btnStyle);
        b->setAutoRaise(true);
        b->setMinimumSize(iconSize.width() + 2 * spacing.buttonPadding,
                          iconSize.height() + 2 * spacing.buttonPadding);
        b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        b->setStyleSheet(
                "QToolButton { border: none; margin: 0; padding: 1px; }"
                "QToolButton:hover { background-color: #3A3A3A; border-radius: 2px; }"
                "QToolButton:pressed { background-color: #4A4A4A; }"
        );
        hLayout->addWidget(b, 0, Qt::AlignLeft | Qt::AlignVCenter);
    }

    this->removeToolBar(toolbar);
    toolbar->hide();

    // 垂直布局逻辑（不变）
    QVBoxLayout* vLayout = new QVBoxLayout(container);
    vLayout->setContentsMargins(spacing.edgeMargin, 0, spacing.edgeMargin, spacing.bottomMargin);
    vLayout->setSpacing(spacing.verticalGap);
    vLayout->setSizeConstraint(QLayout::SetFixedSize);
    vLayout->addWidget(topRow, 1);

    QLabel* titleLabel = new QLabel(title, container);
    titleLabel->setObjectName("toolbarTitle_" + toolbar->objectName());
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    // 不再在 CSS 里硬写 font-size，让 setFont(titleFont) 生效并可被响应式重排刷新
    titleLabel->setStyleSheet(
            "QLabel { color: #6B6B6B; padding: 0; "
            "background-color: transparent; border: none; font-family: 'PingFang SC'; }"
    );
    vLayout->addWidget(titleLabel, 0);

    QToolBar* wrapper = new QToolBar(this);
    wrapper->setObjectName("wrapper_" + toolbar->objectName());
    wrapper->setMovable(true);
    wrapper->setFloatable(true);
    wrapper->setMinimumHeight(totalH);
    wrapper->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QWidgetAction* wrapperAction = new QWidgetAction(wrapper);
    wrapperAction->setDefaultWidget(container);
    wrapper->addAction(wrapperAction);
    wrapper->setMinimumWidth(container->sizeHint().width() + spacing.btnGap * 2);

    this->addToolBar(area, wrapper);
}

void igQtMainWindow::relayoutToolbarWrappers() {
    QList<QToolBar*> wrappers;
    const QStringList orderedNames = {
            "wrapper_toolBar_meshfile",
            "wrapper_toolBar_3",
            "wrapper_toolBar_2",
            "wrapper_toolBar_4"
    };

    for (const QString& name : orderedNames) {
        if (QToolBar* tb = this->findChild<QToolBar*>(name)) {
            wrappers.push_back(tb);
        }
    }
    if (wrappers.isEmpty()) return;

    // 每次重排前先清掉舊分行點，避免重複斷行導致排版漂移
    for (QToolBar* tb : wrappers) {
        this->removeToolBarBreak(tb);
    }

    const int availableWidth = qMax(320, this->width() - 40);
    const int iconSize = resolveToolbarIconSizeForWidget(this);
    const ToolbarSpacingMetrics spacing = metricsForIconSize(iconSize);
    const int gap = spacing.groupGap;
    const int rowGap = spacing.rowGap;
    int usedWidth = 0;

    for (QToolBar* tb : wrappers) {
        if (!tb) continue;
        const int needWidth = qMax(tb->minimumWidth(), tb->sizeHint().width());
        bool startsNewRow = false;
        if (usedWidth > 0 && (usedWidth + needWidth) > availableWidth) {
            this->insertToolBarBreak(tb);
            startsNewRow = true;
            usedWidth = 0;
        }
        tb->setStyleSheet(QStringLiteral("#%1 { margin-top: %2px; border: none; }")
                                  .arg(tb->objectName())
                                  .arg(startsNewRow ? rowGap : 0));
        usedWidth += needWidth + gap;
    }
}

void igQtMainWindow::UpdateIcons()
{
    // 依螢幕寬度與 DPI 動態縮放，避免高縮放或低解析度下 toolbar 後段按鈕被擠壓。
    const int iconSize = resolveToolbarIconSizeForWidget(this);

    for (QToolBar* tb : this->findChildren<QToolBar*>()) {
        if (tb->objectName().startsWith("wrapper_"))
            continue;
        tb->setIconSize(QSize(iconSize, iconSize));
        tb->setMinimumHeight(iconSize + qMax(6, iconSize / 6));
    }


}

void igQtMainWindow::hookResponsiveEvents() {
    if (m_ResponsiveHooked) return;
    m_ResponsiveHooked = true;

    // 防抖 timer：合并 100ms 内的多次 resize 触发。
    m_ResizeDebounceTimer = new QTimer(this);
    m_ResizeDebounceTimer->setSingleShot(true);
    connect(m_ResizeDebounceTimer, &QTimer::timeout, this,
            [this]() { applyResponsiveToolbarLayout(); });

    // 屏幕切换（拖到不同显示器）→ 重新排。
    if (auto wh = this->windowHandle()) {
        connect(wh, &QWindow::screenChanged, this,
                [this](QScreen*) { applyResponsiveToolbarLayout(); });
    }
}

int igQtMainWindow::totalWrapperWidth() const {
    int total = 0;
    for (QToolBar* tb : this->findChildren<QToolBar*>()) {
        if (!tb->objectName().startsWith("wrapper_")) continue;
        total += qMax(tb->minimumWidth(), tb->sizeHint().width());
    }
    return total;
}

void igQtMainWindow::applyResponsiveToolbarLayout() {
    // 只按当前窗口宽度选一次 iconSize，然后由 relayoutToolbarWrappers 决定是否换行
    const int iconSize = resolveToolbarIconSizeForWidget(this);
    applyToolbarIconSize(iconSize);
    relayoutToolbarWrappers();
}

void igQtMainWindow::applyToolbarIconSize(int iconSize) {
    const QSize iconQSize(iconSize, iconSize);
    const ToolbarSpacingMetrics spacing = metricsForIconSize(iconSize);

    // 1) 普通 toolbar（非 wrapper_ 前缀）——只需 setIconSize，Qt 会级联到其中的 QToolButton。
    for (QToolBar* tb : this->findChildren<QToolBar*>()) {
        if (tb->objectName().startsWith("wrapper_"))
            continue;
        tb->setIconSize(iconQSize);
        tb->setMinimumHeight(iconSize + qMax(6, iconSize / 6));
    }

    // 2) 两行按钮容器（+X/-X/+Y/-Y/+Z/-Z 那种）——按钮尺寸/最小最大值都是硬算的，
    //    需要在 icon 尺寸变化时刷新，否则容器会保留旧尺寸把 toolbar 撑大。
    const int gridSpacing = qMax(4, iconSize / 6);
    const int targetIcon = qMax(20, static_cast<int>(iconSize * 0.65));
    const int rowHeight = targetIcon + 8;
    const QSize btnSize(rowHeight, rowHeight);
    for (QWidget* container : this->findChildren<QWidget*>("twoRowViewGrid")) {
        for (QToolButton* btn : container->findChildren<QToolButton*>(
                     QString(), Qt::FindDirectChildrenOnly)) {
            btn->setIconSize(QSize(targetIcon, targetIcon));
            btn->setMinimumSize(btnSize);
            btn->setMaximumSize(btnSize);
        }
        container->setMinimumHeight(2 * rowHeight + gridSpacing);
        container->setMinimumWidth(3 * rowHeight + 2 * gridSpacing);
        if (auto* lay = container->layout()) { lay->setSpacing(gridSpacing); }
        container->updateGeometry();
    }

    // 3) 带标题的容器（toolbarContainer_ 前缀）——里面的 topRow 装的都是 QToolButton；
    //    这些按钮当初 setIconSize 用的是 toolbar 的 iconSize（现在已经改过了），
    //    需要显式刷一遍，同时更新最小尺寸/间距。
    //    注意：twoRowViewGrid 可能被 addToolbarTitle 迁移到 toolbarContainer_ 下面，
    //    这里必须跳过其中的按钮，否则会把上一步刷成 targetIcon 的又改回 iconSize。
    for (QWidget* container : this->findChildren<QWidget*>()) {
        if (!container->objectName().startsWith("toolbarContainer_"))
            continue;
        for (QToolButton* b : container->findChildren<QToolButton*>()) {
            bool insideTwoRow = false;
            for (QWidget* p = b->parentWidget(); p && p != container; p = p->parentWidget()) {
                if (p->objectName() == "twoRowViewGrid") {
                    insideTwoRow = true;
                    break;
                }
            }
            if (insideTwoRow) continue;
            b->setIconSize(iconQSize);
            b->setMinimumSize(iconSize + 2 * spacing.buttonPadding,
                              iconSize + 2 * spacing.buttonPadding);
        }
        // 顶部按钮行 layout 间距同步刷新
        if (auto topRow = container->findChild<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
            if (auto* hlay = qobject_cast<QHBoxLayout*>(topRow->layout())) {
                hlay->setContentsMargins(spacing.edgeMargin, 0, spacing.edgeMargin, 0);
                hlay->setSpacing(spacing.btnGap);
            }
        }
        // 标题标签字号也跟着 iconSize 缩放
        const int titlePt = titlePointSizeForIcon(iconSize);
        for (QLabel* lbl : container->findChildren<QLabel*>()) {
            if (!lbl->objectName().startsWith("toolbarTitle_"))
                continue;
            QFont f = lbl->font();
            f.setPointSize(titlePt);
            lbl->setFont(f);
            lbl->updateGeometry();
        }
        // 外层垂直 layout 的间距/下边距也同步刷新
        if (auto* vlay = qobject_cast<QVBoxLayout*>(container->layout())) {
            vlay->setContentsMargins(spacing.edgeMargin, 0, spacing.edgeMargin, spacing.bottomMargin);
            vlay->setSpacing(spacing.verticalGap);
        }
        // 容器整体最小尺寸重算：图标高 + 垂直间距 + 标题高 + 下边距
        const int titleTextH = QFontMetrics(QFont(QStringLiteral("PingFang SC"), titlePt)).height();
        const int totalH = iconSize + spacing.verticalGap + titleTextH + spacing.bottomMargin;
        container->setMinimumHeight(totalH);
        container->updateGeometry();
    }

    // 4) 包装 toolbar 也刷新最小高度/宽度（保证 sizeHint 用得上新尺寸）
    for (QToolBar* tb : this->findChildren<QToolBar*>()) {
        if (!tb->objectName().startsWith("wrapper_"))
            continue;
        if (auto* container = tb->findChild<QWidget*>()) {
            tb->setMinimumHeight(container->minimumSizeHint().height());
            tb->setMinimumWidth(container->minimumSizeHint().width());
        }
        tb->updateGeometry();
    }
    // 注意：不在这里 relayoutToolbarWrappers()——是否换行由外层
    // applyResponsiveToolbarLayout() 统一决定（我们的目标是一直保持一行）
}

void igQtMainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    // 窗口首次真正显示后，屏幕 handle 才稳定；此时挂 hook + 跑一次布局，
    // 修正启动阶段按"主屏宽度"猜的档位。
    hookResponsiveEvents();
    applyResponsiveToolbarLayout();
}
