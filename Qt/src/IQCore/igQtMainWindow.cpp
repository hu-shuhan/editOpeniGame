#include "IQCore/igQtMainWindow.h"
//
// Created by m_ky on 2024/4/10.
//

#include "MeshMetrics/iGameVolumeMeshMetricsFilter.h"
#include "Deformation/iGameStressDeformationFilterCode.h"

#include "DataProcessing/Tests/iGameGradient.h"
#include "FeatureExtraction/iGameAdvancedGradientFilter.h"
#include "DataProcessing/Tests/iGameSimplification2.h"
#include "DataProcessing/Tests/iGameSurfaceSimplification.h"
#include "DataProcessing/Tests/meshsimplifier/meshsimplifier.h"
#include "DataProcessing/Tests/simplifier.h"
#include "DataProcessing/iGameMeshSimplificationFilter.h"
#include "DataProcessing/iGameMeshSimplificationFilterPro.h"
#include "DataProcessing/iGameMeshTriangulationFilter.h"
#include "DataProcessing/iGameSurfaceMeshTopologyChecker.h"
#include "DataProcessing/Simplification/iGameMeshSaliency.h"
#include "DataProcessing/Simplification/iGameMeshSimplificationWithAttributes.h"
#include "DataProcessing/iGameVolumeMeshSimplification.h"
#include "DataProcessing/iGameMeshTetrahedralize.h"

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
#include <IQCore/igQtRemoteModelLibrary.h>
#include <IQWidgets/ColorManager/igQtColorManagerWidget.h>
#include <IQWidgets/igQtAiChat/igQtAiChatWidget.h>
#include <IQWidgets/igQtAiChat/igQtCommandManager.h>
#include <IQWidgets/igQtCharts.h>
#include <IQWidgets/igQtDeformationWidget.h>
#include <IQWidgets/igQtElevationFilterPanel.h>
#include <IQWidgets/igQtModelClipWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQWidgets/igQtModelInformationWidget.h>
#include <IQWidgets/igQtParallelCoordinatesWidget.h>
#include <IQWidgets/igQtTensorWidget.h>
#include <IQWidgets/igQtVariableCorrelationWidget.h>
#include <IQWidgets/igQtPartFocusWidget.h>
#include <IQWidgets/igQtAttributeSelectWidget.h>
#include <IQComponents/Dialog/igQtBoxSettingDialog.h>
#include <IQComponents/Dialog/igQtChromeFramelessDialog.h>
#include <iGameBlockMapping.h>
#include <P3SAM/iGameP3SAMSegmenter.h>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <limits>
#include <QButtonGroup>
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
#include <QPainterPath>
#include <QRegion>
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

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QSplitter>
#include <QPointer>
#include <QTimer>
#include <QTextStream>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QApplication>
#include <IQWidgets/igQtRenderWidget.h>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QStyle>
#include <QFontMetrics>
#include <QSettings>
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QStringList>
#include <QFile>
#include <limits>
#include <QMenu>
#include <QAction>

#include <cmath>


#include "ui_igQtVariableCorrelationWidget.h"
#include <IQWidgets/igQtRoundedCornerHelper.h>

namespace {
const QColor kFloatingCoverColor(0x1E, 0x1E, 0x1E);

//
void applyRoundedMask(QWidget* w, int radius) {
    if (!w || radius <= 0) return;
    const QSize sz = w->size();
    if (sz.isEmpty()) return;

    const int ss = 4;
    QImage big(sz.width() * ss, sz.height() * ss, QImage::Format_ARGB32_Premultiplied);
    if (big.isNull()) return;
    big.fill(Qt::transparent);
    {
        QPainter p(&big);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::black);
        p.drawRoundedRect(QRectF(0, 0, big.width(), big.height()), qreal(radius) * ss, qreal(radius) * ss);
    }
    const QImage small = big.scaled(sz, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    w->setMask(QRegion(QBitmap::fromImage(small.createAlphaMask())));
}

// ---------------------------------------------------------------------------
// 「数据转换」辅助：把一个数据对象就地转换，并回传统计信息。
//   1) 用核心既有的「就地转换」filter（不改动核心代码）；转换后单元属性会变成同名点属性，
//      必须强制重算可绘制数据，否则画面/色条还是旧的；
//   2) filter 在“该方向没有属性”时会空转并返回 true，所以这里先自己数候选属性，
//      否则“什么都没做”会被当成转换成功（那就会出现点了菜单毫无反应、也不报错）；
//   3) 只处理自身带网格数据的对象：PVD 这类复合模型的父容器只是空壳，
//      调用方（数据转换）会把**当前帧挂载的**对象逐个传进来。
struct ConvertApplyResult {
    int candidateAttrs{0};    // 命中方向的属性数（转点数据=单元属性；转单元数据=点属性）
    int convertedAttrs{0};    // 真正转换成功的属性数
    bool filterFailed{false}; // filter 执行失败
};

ConvertApplyResult ConvertDataObjectInPlace(iGame::DataObject::Pointer object, bool toPointData) {
    using namespace iGame;
    ConvertApplyResult result;
    if (object == nullptr) { return result; }
    if (object->GetPoints() == nullptr || object->GetCellArray() == nullptr) { return result; }
    auto attrs = object->GetAttributeSet();
    if (attrs == nullptr) { return result; }

    for (int i = 0; i < attrs->GetNumberOfAttributes(); ++i) {
        auto& attr = attrs->GetAttribute(i);
        if (attr.isDeleted || attr.pointer == nullptr) { continue; }
        if (toPointData ? (attr.attachmentType == IG_CELL) : (attr.attachmentType == IG_POINT)) {
            ++result.candidateAttrs;
        }
    }
    if (result.candidateAttrs == 0) { return result; } // 该方向没有属性：空转不算转换

    bool ok = false;
    if (toPointData) {
        ConvertToPointDataFilter::Pointer filter = ConvertToPointDataFilter::New();
        filter->SetInput(object);
        ok = filter->Execute();
    } else {
        ConvertToCellDataFilter::Pointer filter = ConvertToCellDataFilter::New();
        filter->SetInput(object);
        ok = filter->Execute();
    }
    if (!ok) {
        result.filterFailed = true;
        qDebug() << "Convert filter failed on object" << QString::fromStdString(object->GetName())
                 << (toPointData ? "ToPointData" : "ToCellData");
        return result;
    }
    result.convertedAttrs = result.candidateAttrs;
    if (auto drawObject = DynamicCast<DrawObject>(object)) {
        drawObject->ForceReConvertToDrawableData();
    }
    return result;
}

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

// 工具栏单排适配的可调参数
constexpr int kToolbarIconMin = 24;             // 图标尺寸下限（与 resolveToolbarIconSize 的 clamp 下限一致）
constexpr int kToolbarIconMax = 56;             // 图标尺寸上限（放大填充时允许略超分档表，让宽屏更饱满）
constexpr double kToolbarFillRatio = 0.92;      // 单排填充目标：工具栏总宽达到可用宽度的 92% 左右即停止放大
constexpr int kToolbarButtonTextMinWidth = 48;
constexpr int kToolbarButtonTextMaxLines = 3;

// 按钮字号（逻辑像素）随图标尺寸联动。
// 注意：必须用像素单位而非 pt，否则在高 DPI 缩放下文字宽度会随 DPI 放大，导致换行/压宽度失效。
int toolbarButtonFontPixel(int iconSize) {
    if (iconSize >= 46) return 12;
    if (iconSize >= 38) return 11;
    if (iconSize >= 30) return 10;
    if (iconSize >= 26) return 9;
    return 8;
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
    border: none;
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
    border: none;
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

constexpr int kFallbackStyleMode = 12;

bool isStyleModeAvailable(int mode) {
    return mode >= 12 && mode <= 15;
}

int normalizeStyleMode(int mode) {
    return isStyleModeAvailable(mode) ? mode : kFallbackStyleMode;
}


bool isFloatingCardStyle(int mode) { return mode >= 12 && mode <= 15; }

bool isLightStyle(int mode) { return mode == 2 || mode == 13; }

bool isModernDenseStyle(int mode) { return mode >= 9; }

int styleColorFamily(int mode) {
    switch (mode) {
        case 2:  case 13: return 2;
        case 9:  case 14: return 9;
        case 10: case 15: return 10;
        case 11: return 11;
        default: return 12;
    }
}

QString loadQssResource(const QString& path) {
    QFile qssFile(path);
    if (!qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "iGameVis: failed to load style sheet" << path;
        return QString();
    }
    return QString::fromUtf8(qssFile.readAll());
}

struct FloatingCardPalette {
    const char* backdrop;
    const char* cardBg;
    const char* cardBorder;
};

FloatingCardPalette floatingCardPalette(int mode) {
    switch (styleColorFamily(mode)) {
        case 2:  return { "#EAEEF3", "#F1F3F7", "#CBD2DC" };
        case 9:  return { "#121316", "#22262C", "#31363D" };
        case 10: return { "#14161A", "#20242A", "#2C3038" };
        default: return { "#1E1E1E", "#1E1E1E", "#2D2D30" };
    }
}
}

// 「数据转换」：**就地**转换当前帧挂载的数据，转换完模型树里仍然只有这一个模型——
// 不新增行，而是把该模型改名成转换后的名字（`原名[_fN]_PointData/_CellData`）。
// 说明：
//   - 目标：复合模型（PVD 等）= 当前挂载的子块（即当前帧，不是第一帧）；普通模型 = 自身；
//   - 转换是就地的，所以原来那份数据本身已经变成点/单元数据（不是独立副本）；
//   - 转换后同步刷新属性行的挂载类型图标与画面（否则看起来像"没反应"）。
int igQtMainWindow::createConvertedFrameModel(bool toPointData, QString& reason, QStringList& createdNames) {
    using namespace iGame;
    reason.clear();
    createdNames.clear();

    if (rendererWidget == nullptr || modelTreeWidget == nullptr) {
        reason = QStringLiteral("界面尚未就绪");
        return 0;
    }
    auto scene = rendererWidget->GetScene();
    auto model = scene ? scene->GetCurrentModel() : nullptr;
    if (model == nullptr || model->GetDataObject() == nullptr) {
        reason = QStringLiteral("请先加载模型");
        return 0;
    }
    auto top = model->GetDataObject();

    // 1) 目标：复合模型（PVD 等）→ 当前挂载的子块，也就是“当前帧”；普通模型 → 它自己
    std::vector<DataObject::Pointer> targets;
    if (top->HasSubDataObject()) {
        for (auto it = top->SubDataObjectIteratorBegin();
             it != top->SubDataObjectIteratorEnd(); ++it) {
            auto sub = it->second;
            if (sub == nullptr) { continue; }
            if (sub->GetPoints() == nullptr || sub->GetCellArray() == nullptr) { continue; }
            targets.push_back(sub);
        }
    } else if (top->GetPoints() != nullptr && top->GetCellArray() != nullptr) {
        targets.push_back(top);
    }
    if (targets.empty()) {
        reason = QStringLiteral("当前帧没有带网格数据的对象（子块可能尚未加载完成），无法转换。");
        return 0;
    }

    // Invalidate the C/S snapshot before editing shared mesh arrays.
    // Keep the visible model; only its reusable remote cache entry expires.
    if (fileLoader) {
        fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("In-place data conversion"));
    }

    // 2) 就地转换（共享模式下，转换结果就属于这份数据本身）
    int convertedAttrs = 0;
    int convertFailed = 0;
    int noCandidates = 0;
    for (auto& target : targets) {
        const auto res = ConvertDataObjectInPlace(target, toPointData);
        if (res.convertedAttrs <= 0) {
            if (res.filterFailed) { ++convertFailed; } else { ++noCandidates; }
            continue;
        }
        convertedAttrs += res.convertedAttrs;
    }
    if (convertedAttrs <= 0) {
        if (convertFailed > 0) {
            reason = QStringLiteral("转换 filter 执行失败，详见日志。");
        } else if (noCandidates > 0) {
            reason = toPointData ? QStringLiteral("当前帧没有单元属性（该帧只有点属性），无需转换。")
                                 : QStringLiteral("当前帧没有点属性，无需转换。");
        } else {
            reason = QStringLiteral("当前帧没有可转换的数据。");
        }
        return 0;
    }

    // 3) 复合模型：父容器上的属性是各子块属性的“占位登记”，同步挂载类型并重算值域
    if (top->HasSubDataObject()) {
        if (auto parentAttrs = top->GetAttributeSet()) {
            for (int i = 0; i < parentAttrs->GetNumberOfAttributes(); ++i) {
                auto& parentAttr = parentAttrs->GetAttribute(i);
                if (parentAttr.isDeleted || parentAttr.pointer == nullptr) { continue; }
                const std::string name = parentAttr.pointer->GetName();
                for (auto it = top->SubDataObjectIteratorBegin();
                     it != top->SubDataObjectIteratorEnd(); ++it) {
                    auto sub = it->second;
                    if (sub == nullptr) { continue; }
                    auto subAttrs = sub->GetAttributeSet();
                    if (subAttrs == nullptr) { continue; }
                    const int subIndex = subAttrs->GetAttributeIndex(name);
                    if (subIndex < 0) { continue; }
                    parentAttr.attachmentType = subAttrs->GetAttribute(subIndex).attachmentType;
                    break;
                }
            }
        }
        top->ReCollectSubDataObjectDataRange();
        top->UpdateSubDataObjectDataRange();
        if (auto drawObject = DynamicCast<DrawObject>(top)) {
            drawObject->ForceReConvertToDrawableData();
        }
    }
    // 原行的属性图标/提示按新挂载类型就地刷新（只改图标与提示，不重建行，不会丢子块行）
    modelTreeWidget->refreshAttributeBadges(top);

    // 4) 模型树里只保留这一个模型：把该模型改名成转换后的名字（含发生转换的帧号）
    int frameIndex = 0;
    int frameCount = 1;
    if (auto frames = top->PeekTimeFrames()) {
        frameCount = static_cast<int>(frames->GetTimeNum());
        if (frameCount > 0) {
            frameIndex = ui->widget_Animation != nullptr ? ui->widget_Animation->currentFrameIndex() : 0;
            frameIndex = std::max(0, std::min(frameIndex, frameCount - 1));
        }
    }
    // 名字规则：基准名 + [_f<帧号>] + _PointData/_CellData。
    // 之前已经转换过的话，名字里已经带了这两种后缀，这里先剥干净再拼，否则每转一次都会
    // 多叠一段（`1_f1_PointData` → `1_f1_f12_CellData` → …）。
    QString baseName = QString::fromStdString(top->GetName());
    bool hadDataTypeSuffix = false;
    for (const auto& suffix : {QStringLiteral("_PointData"), QStringLiteral("_CellData")}) {
        if (baseName.endsWith(suffix)) {
            baseName.chop(suffix.size());
            hadDataTypeSuffix = true;
            break;
        }
    }
    // 只有名字确实带过我们加的数据类型后缀时才剪掉结尾的 `_f<数字>`，
    // 免得误伤本来就叫 `xxx_f12` 的文件名。
    if (hadDataTypeSuffix) {
        const int underscore = baseName.lastIndexOf(QLatin1Char('_'));
        if (underscore > 0 && underscore + 2 < baseName.size() &&
            baseName.at(underscore + 1) == QLatin1Char('f')) {
            bool digitsOnly = true;
            for (int i = underscore + 2; i < baseName.size(); ++i) {
                if (!baseName.at(i).isDigit()) {
                    digitsOnly = false;
                    break;
                }
            }
            if (digitsOnly) { baseName.chop(baseName.size() - underscore); }
        }
    }
    if (baseName.isEmpty()) { baseName = QStringLiteral("Model"); }
    QString convertedName = baseName;
    if (frameCount > 1) { convertedName += QStringLiteral("_f%1").arg(frameIndex + 1); }
    convertedName += toPointData ? QStringLiteral("_PointData") : QStringLiteral("_CellData");

    const QString finalName = modelTreeWidget->renameModelRow(top, convertedName);
    createdNames << finalName;
    qDebug() << "Convert in place, model renamed to:" << finalName << "converted attrs:" << convertedAttrs
             << (toPointData ? "ToPointData" : "ToCellData");
    rendererWidget->update();
    return static_cast<int>(createdNames.size());
}

igQtMainWindow::igQtMainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    qApp->setStyleSheet(qApp->styleSheet() + QString::fromUtf8(kGlobalSpinBoxDarkQss));

    m_originalStyleSheet = this->styleSheet();

    QSettings settings(QStringLiteral("iGame"), QStringLiteral("iGameVis"));
    const int savedMode = settings.value(QStringLiteral("ui/styleMode"), kFallbackStyleMode).toInt();
    m_styleMode = normalizeStyleMode(qBound(0, savedMode, 15));
    igQtRenderWidget::setGlobalStyleMode(m_styleMode);
    const QString initialQss = styleSheetForMode(m_styleMode);
    if (!initialQss.isEmpty()) {
        this->setStyleSheet(initialQss);
    } else {
        m_styleMode = kFallbackStyleMode;
    }

    // 设置窗口标题为iGameVis
    this->setWindowTitle("iGameVis");
    // 使用无边框窗口并自定义标题栏
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);
    initCustomTitleBar();
    initAllUnDefinedComponents();
    if (isFloatingCardStyle(m_styleMode)) applyFloatingCards(true);
    UpdateIcons();
    initAllComponents();
    initAllFilters();

    initAllSources();
    initAllInteractor();
    updateRecentFilePaths();
    initToolbarComponent();  // 内部会重建 toolBar_4 的 3×2 轴网格 + 4 组「按钮行+标题」容器 + 单排宽度拟合

    updateRecentFilePaths();


    connect(modelTreeWidget, &igQtModelDialogWidget::Update, rendererWidget, &igQtRenderWidget::update);

    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [this]() {
        if (!m_projectChip) return;
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        if (model && model->GetDataObject()) {
            std::string name = model->GetDataObject()->GetName();
            m_projectChip->setText(QString::fromStdString(name.empty() ? "模型" : name));
        } else {
            m_projectChip->setText(QStringLiteral("iGameVis"));
        }
    });

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
    m_titleBar->setAttribute(Qt::WA_StyledBackground, true);
    m_titleBar->setFixedHeight(50);
    // 标题栏 QSS 见 iGameQtMainWindow.ui 中 MainWindow.styleSheet（QWidget#CustomTitleBar 等）

    auto* mainLayout = new QVBoxLayout(m_titleBar);
    mainLayout->setContentsMargins(8, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 顶部一行：图标 + 标题 + 按钮
    QWidget* topRow = new QWidget(m_titleBar);
    auto* topLayout = new QHBoxLayout(topRow);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(4);

    m_logoIconLabel = new QLabel(topRow);
    m_logoIconLabel->setObjectName(QStringLiteral("AppLogoLabel"));
    m_logoIconLabel->setFixedSize(28, 28);
    m_logoIconLabel->setAttribute(Qt::WA_StyledBackground, true);
    QPixmap pm = windowIcon().pixmap(18, 18);
    m_logoIconLabel->setPixmap(pm);
    m_logoIconLabel->setScaledContents(true);
    m_logoIconLabel->setAlignment(Qt::AlignCenter);

    m_brandBox = new QWidget(topRow);
    m_brandBox->setObjectName(QStringLiteral("TitleBrandBox"));
    m_brandBox->setAttribute(Qt::WA_StyledBackground, true);
    auto* brandLayout = new QHBoxLayout(m_brandBox);
    brandLayout->setContentsMargins(5, 2, 10, 2);
    brandLayout->setSpacing(6);
    brandLayout->addWidget(m_logoIconLabel);

    m_titleLabel = new QLabel(m_brandBox);
    m_titleLabel->setObjectName(QStringLiteral("CustomTitleLabel"));
    m_titleLabel->setText(this->windowTitle());
    brandLayout->addWidget(m_titleLabel);

    topLayout->addWidget(m_brandBox, 0, Qt::AlignVCenter);

    m_topMenuLayout = topLayout;
    if (ui->menuBar) {
        ui->menuBar->setParent(topRow);
        ui->menuBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        ui->menuBar->setFixedHeight(28);
        ui->menuBar->show();
        m_topMenuLayout->insertWidget(1, ui->menuBar, 0, Qt::AlignVCenter);
    }

    topLayout->addStretch(1);

    m_projectChip = new QLabel(QStringLiteral("iGameVis"), topRow);
    m_projectChip->setObjectName(QStringLiteral("ProjectChip"));
    m_projectChip->setAttribute(Qt::WA_StyledBackground, true);
    m_projectChip->setFixedHeight(30);
    m_projectChip->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(m_projectChip, 0);

    // 按钮区域（尺寸与 Windows 标题栏按钮比例相近：较宽、易点）
    m_btnMinimize = new QPushButton(topRow);
    m_btnMinimize->setObjectName(QStringLiteral("MinimizeButton"));
    m_btnMaximize = new QPushButton(topRow);
    m_btnMaximize->setObjectName(QStringLiteral("MaximizeButton"));
    m_btnClose = new QPushButton(QStringLiteral("×"), topRow);
    m_btnClose->setObjectName(QStringLiteral("CloseButton"));

    m_styleToggleButton = new QPushButton(topRow);
    m_styleToggleButton->setObjectName(QStringLiteral("StyleToggleButton"));
    m_styleToggleButton->setCursor(Qt::PointingHandCursor);
    m_styleToggleButton->setToolTip(QStringLiteral("切换界面风格"));

    const QSize captionBtnSize(46, 30);
    const QSize styleToggleSize(116, 30);
    m_btnMinimize->setFixedSize(captionBtnSize);
    m_btnMaximize->setFixedSize(captionBtnSize);
    m_btnClose->setFixedSize(captionBtnSize);
    m_styleToggleButton->setFixedSize(styleToggleSize);

    m_btnMinimize->setIcon(QIcon(QStringLiteral(":/Ticon/Icons/window_minimize_white.svg")));
    m_btnMinimize->setIconSize(QSize(12, 12));
    m_btnMaximize->setIconSize(QSize(12, 12));
    m_btnMaximize->setText(QString());
    m_btnMaximize->setFlat(true);
    m_btnMinimize->setFlat(true);
    m_btnClose->setFlat(true);

    topLayout->addWidget(m_styleToggleButton, 0);

    m_rightDivider = new QFrame(topRow);
    m_rightDivider->setObjectName(QStringLiteral("RightDivider"));
    m_rightDivider->setFixedSize(1, 24);
    m_rightDivider->setFrameShape(QFrame::NoFrame);
    m_rightDivider->setAttribute(Qt::WA_StyledBackground, true);
    topLayout->addWidget(m_rightDivider, 0);

    topLayout->addWidget(m_btnMinimize, 0);
    topLayout->addWidget(m_btnMaximize, 0);
    topLayout->addWidget(m_btnClose, 0);

    // 添加顶部行到主布局
    mainLayout->addWidget(topRow, 0);

    m_titleAccentLine = new QFrame(m_titleBar);
    m_titleAccentLine->setObjectName(QStringLiteral("TitleBarAccentLine"));
    m_titleAccentLine->setFixedHeight(2);
    m_titleAccentLine->setFrameShape(QFrame::NoFrame);
    m_titleAccentLine->setAttribute(Qt::WA_StyledBackground, true);
    mainLayout->addWidget(m_titleAccentLine, 0);

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

    createStyleMenu();

    applyStyleMode(m_styleMode);

    // 监听全局鼠标释放，防止拖动状态在某些场景下卡住
    qApp->installEventFilter(this);
    updateMaximizeButtonIcon();
}

bool igQtMainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (!m_titleBar) return QMainWindow::eventFilter(watched, event);

    if (isFloatingCardStyle(m_styleMode) && event->type() == QEvent::Resize) {
        if (watched == rendererWidget) {
            applyRoundedMask(rendererWidget, 8);
        } else if (watched == m_floatingTreeDock) {
            const int radius = (modelTreeWidget && modelTreeWidget->isTreeDockCollapsed()) ? 12 : 8;
            applyRoundedMask(m_floatingTreeDock, radius);
        }
    }

    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
        const QVariant btnProp = watched->property("igToolbarButton");
        if (btnProp.isValid()) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                if (event->type() == QEvent::MouseButtonRelease) {
                    auto* btn = qobject_cast<QToolButton*>(btnProp.value<QObject*>());
                    auto* host = qobject_cast<QWidget*>(watched);
                    if (btn && host && host->rect().contains(me->pos())) { btn->click(); }
                }
                return true;
            }
        }
    }

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
    const bool light = isLightStyle(m_styleMode);
    const QString restoreIcon = light ? QStringLiteral(":/Ticon/Icons/window_restore_dark.svg")
                                      : QStringLiteral(":/Ticon/Icons/window_restore_white.svg");
    const QString maximizeIcon = light ? QStringLiteral(":/Ticon/Icons/window_maximize_dark.svg")
                                       : QStringLiteral(":/Ticon/Icons/window_maximize_white.svg");
    m_btnMaximize->setIcon(QIcon(isMaximized() ? restoreIcon : maximizeIcon));
    m_btnMaximize->setIconSize(isMaximized() ? QSize(15, 15) : QSize(12, 12));
    m_btnMaximize->setText(QString());
}

void igQtMainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    // 立刻决定要不要换行（视觉上跟手）；同时 100ms 防抖跑一次全量重排（含 iconSize 重算）
    relayoutToolbarWrappers();
    if (m_ResizeDebounceTimer) { m_ResizeDebounceTimer->start(100); }
    updateViewRailPosition();
}

igQtMainWindow::~igQtMainWindow() {
    // Release independent cache ownership before QObject destroys the renderer.
    if (fileLoader) {
        fileLoader->CancelRemotePackage();
        fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("Client shutdown"));
    }
    // 清理命令管理器
    if (commandManager) {
        commandManager->stopConnection();
        delete commandManager;
        commandManager = nullptr;
    }
}
void igQtMainWindow::initArgs(const QStringList& args) {
    int argc = args.size();
    QString remotePackage;
    QString remoteHost = QStringLiteral("127.0.0.1");
    quint16 remotePort = 34567;
    QString remoteCache = QStringLiteral("D:/iGameVis-cs-cache");
    int cacheRepeat = 1;
    QString cacheBenchmarkOutput;
    const bool cpuPreload = args.contains(QStringLiteral("--remote-cpu-preload-first")) ||
                            args.contains(QStringLiteral("--remote-cpu-preload-only"));
    const bool residentCache = !args.contains(QStringLiteral("--remote-no-memory-cache"));
    if (cpuPreload && (!residentCache || args.contains(QStringLiteral("--remote-cache-retain-gpu")))) {
        igError("CPU preload requires memory caching and cannot be combined with --remote-cache-retain-gpu");
        return;
    }
    fileLoader->SetRemoteMemoryCacheEnabled(residentCache);
    fileLoader->SetRemoteCpuOnlyCacheEnabled(!args.contains(QStringLiteral("--remote-cache-retain-gpu")));
    fileLoader->SetRemoteCacheStrictValidation(
            args.contains(QStringLiteral("--remote-cache-repeat")) ||
            args.contains(QStringLiteral("--remote-cache-benchmark-json")));
    for (int i = 1; i < argc; ++i) {
        const QString& cur_arg = args[i].toLower();
        if (cur_arg == "--filepath" && ++i < argc) {
            const QString& filePath = args[i];
            const QByteArray utf8Path = filePath.toUtf8();
            fileLoader->OpenFile(std::string(utf8Path.constData(), static_cast<std::size_t>(utf8Path.size())));
        } else if (cur_arg == "--remote-package" && ++i < argc) {
            remotePackage = args[i];
        } else if (cur_arg == "--remote-host" && ++i < argc) {
            remoteHost = args[i];
        } else if (cur_arg == "--remote-port" && ++i < argc) {
            bool ok = false;
            const uint value = args[i].toUInt(&ok);
            if (ok && value > 0 && value <= 65535) {
                remotePort = static_cast<quint16>(value);
            } else {
                igError("[PackageTransfer] Invalid --remote-port value: {}",
                        args[i].toStdString());
                return;
            }
        } else if (cur_arg == "--remote-cache" && ++i < argc) {
            remoteCache = args[i];
        } else if (cur_arg == "--remote-cpu-cache-limit-gib" && ++i < argc) {
            bool ok = false;
            const quint64 gib = args[i].toULongLong(&ok);
            if (!ok || gib < 1 || gib > 4096) { igError("Invalid CPU cache limit (1..4096 GiB)"); return; }
            fileLoader->SetRemoteMemoryCacheLimitBytes(gib * 1024ull * 1024 * 1024);
        } else if (cur_arg == "--remote-cache-repeat" && ++i < argc) {
            bool ok = false;
            cacheRepeat = args[i].toInt(&ok);
            if (!ok || cacheRepeat < 1 || cacheRepeat > 20) {
                igError("[RemoteOpenBenchmark] --remote-cache-repeat must be in [1,20]");
                return;
            }
        } else if (cur_arg == "--remote-cache-benchmark-json" && ++i < argc) {
            cacheBenchmarkOutput = args[i];
        }
    }
    if (residentCache) {
        // Static-model mode is conservative around application editing tools.
        // Raw-pointer edits outside these paths must explicitly invalidate the cache.
        connect(ui->menu_filters, &QMenu::triggered, this, [this](QAction*) {
            fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("Algorithm action entered"));
        });
        connect(ui->menu_clip, &QMenu::triggered, this, [this](QAction*) {
            fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("Clipping action entered"));
        });
        for (QAction* action : {ui->action_deformation, ui->action_SelectView,
                                ui->action_AiChat,
                                ui->action_ExportAnimation}) {
            connect(action, &QAction::triggered, this, [this, action]() {
                fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("Editing/tool action: ") + action->objectName());
            });
        }
    }
    if (!remotePackage.isEmpty()) {
        if (residentCache && (cpuPreload || cacheRepeat > 1 || !cacheBenchmarkOutput.isEmpty())) {
            ConfigureRemoteCacheBenchmark(remotePackage, remoteHost, remotePort, remoteCache,
                                           cacheRepeat, cacheBenchmarkOutput);
        } else {
            fileLoader->OpenRemotePackage(remoteHost, remotePort, remotePackage, remoteCache);
        }
    }
}
void igQtMainWindow::initAllUnDefinedComponents() {
    rendererWidget = new igQtModelDrawWidget(this);
    igQtOpenGLManager::Instance()->setQtRenderWidget(rendererWidget);
    //    rendererWidget->setParent(this);
    fileLoader = new igQtFileLoader(this);
    fileLoader->SetRemoteCacheRenderWidget(rendererWidget);
    remoteModelLibrary = new igQtRemoteModelLibrary(fileLoader, this);
    remoteModelLibrary->hide();
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
            const int targetW = qMax(curW + 40, 320);
            this->resizeDocks({m_leftFieldDock}, {targetW}, Qt::Horizontal);
        }
    });

    QTimer::singleShot(100, this, [this]() {
        if (rendererWidget && modelTreeWidget) {
            if (m_styleMode == 6) {
                applyWorkspaceLayout(true);
            } else if (m_styleMode == 7 || m_styleMode == 8) {
                applyViewRail(true);
                modelTreeWidget->positionTreeDockToRendererCorner(rendererWidget);
            } else {
                modelTreeWidget->positionTreeDockToRendererCorner(rendererWidget);
            }
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

    // 高程 (Elevation) 实时参数面板：入口对话框首次执行后 BindSession 绑定并显示，
    // 面板内可实时调整低/高点、按轴铺满包围盒、修改标量范围并应用（交互对齐 ParaView）
    ElevationFilterPanel = new igQtElevationFilterPanel(this);
    this->addDockWidget(Qt::RightDockWidgetArea, ElevationFilterPanel);
    ElevationFilterPanel->hide();
    // 「应用」成功后就地刷新：滤波器复用同一输出对象，模型树不堆叠新节点
    connect(ElevationFilterPanel, &igQtElevationFilterPanel::elevationApplied, this,
            [this](iGame::DataObject::Pointer output) {
        if (!output) return;
        modelTreeWidget->updateAllAttriubute(output);
        if (auto drawObj = DynamicCast<DrawObject>(output)) { drawObj->ForceReConvertToDrawableData(); }
        rendererWidget->update();
    });
    // 参数校验/执行失败时弹提示（与菜单入口一致的暗色无边框提示框）
    connect(ElevationFilterPanel, &igQtElevationFilterPanel::applyFailed, this, [this](const QString& reason) {
        showDarkFramelessMessage(QStringLiteral("高程 (elevation)"), reason);
    });

}
void igQtMainWindow::initToolbarComponent() {
    // 用 QToolButton 行+标题替代 QToolBar（避免 QToolBar 进 layout 导致图标不渲染）
    const int preferredIcon = resolveToolbarIconSizeForWidget(this);
    rebuildToolbarRow(preferredIcon);
    // 宽度拟合 + 换行兜底
    relayoutToolbarWrappers();
}

void igQtMainWindow::initAllComponents() {
    auto* remoteLibraryAction = new QAction(
            ui->action_LoadFile->icon(), QStringLiteral("Remote Model Library..."), this);
    remoteLibraryAction->setObjectName(QStringLiteral("action_RemoteModelLibrary"));
    ui->menu_file->insertAction(ui->menu_RecentFiles->menuAction(), remoteLibraryAction);
    connect(remoteLibraryAction, &QAction::triggered, this, [this]() {
        remoteModelLibrary->show();
        remoteModelLibrary->raise();
        remoteModelLibrary->activateWindow();
    });
    auto* clearRemoteMemory = new QAction(QStringLiteral("Clear C/S Memory Cache"), this);
    clearRemoteMemory->setObjectName(QStringLiteral("action_ClearRemoteMemoryCache"));
    clearRemoteMemory->setToolTip(QStringLiteral(
            "Release the cached dataset; visible models and disk cache files are kept."));
    ui->menu_file->insertAction(ui->menu_RecentFiles->menuAction(), clearRemoteMemory);
    connect(fileLoader, &igQtFileLoader::RemotePackageRunningChanged, clearRemoteMemory,
            [clearRemoteMemory](bool running) { clearRemoteMemory->setEnabled(!running); });
    connect(clearRemoteMemory, &QAction::triggered, this, [this]() {
        fileLoader->InvalidateRemoteMemoryCache(QStringLiteral("User cleared C/S memory cache"));
        statusBar()->showMessage(QStringLiteral(
                "C/S memory cache cleared. Visible models and disk cache files are unchanged."), 7000);
    });

    connect(ui->action_ShowOrientationAxes, &QAction::triggered, this, [&](bool checked){
        iGame::SceneManager::Instance()->GetCurrentScene()->ToggleAxes();
        iGame::SceneManager::Instance()->GetCurrentScene()->Update();
   });
    ui->action_ShowOrientationAxes->setCheckable(true);
    ui->action_ShowOrientationAxes->setChecked(true);
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

    auto showSurfaceTopologyReport = [this](const SurfaceMesh::Pointer& mesh, const QString& stage) {
        const auto report = SurfaceMeshTopologyChecker::Check(mesh);
        auto formatIds = [](const std::vector<igIndex>& ids) {
            QString text;
            const int displayCount = std::min<int>(8, static_cast<int>(ids.size()));
            for (int i = 0; i < displayCount; ++i) {
                if (i != 0) { text += QStringLiteral(", "); }
                text += QString::number(ids[i]);
            }
            if (static_cast<int>(ids.size()) > displayCount) { text += QStringLiteral(" ..."); }
            return text;
        };

        QString text = QStringLiteral("检查阶段：%1\n点数：%2  边数：%3  面数：%4\n\n")
                               .arg(stage)
                               .arg(report.pointCount)
                               .arg(report.edgeCount)
                               .arg(report.faceCount);
        auto appendIssue = [&](const QString& name, const std::vector<igIndex>& ids) {
            text += QStringLiteral("%1：%2").arg(name).arg(ids.size());
            if (!ids.empty()) { text += QStringLiteral("（示例 ID：%1）").arg(formatIds(ids)); }
            text += QLatin1Char('\n');
        };

        appendIssue(QStringLiteral("非三角形面"), report.nonTriangleFaceIds);
        appendIssue(QStringLiteral("越界索引面"), report.invalidIndexFaceIds);
        appendIssue(QStringLiteral("退化三角形"), report.degenerateFaceIds);
        appendIssue(QStringLiteral("零面积三角形"), report.zeroAreaFaceIds);
        appendIssue(QStringLiteral("重复三角形"), report.duplicateFaceIds);
        appendIssue(QStringLiteral("无效边"), report.invalidEdgeIds);
        appendIssue(QStringLiteral("孤立边"), report.isolatedEdgeIds);
        appendIssue(QStringLiteral("非流形边"), report.nonManifoldEdgeIds);
        appendIssue(QStringLiteral("无效边—面邻接"), report.invalidAdjacencyEdgeIds);

        text += report.IsValid()
                        ? QStringLiteral("\n结论：拓扑检查通过，可进行传统表面网格简化。")
                        : QStringLiteral("\n结论：拓扑检查未通过。请根据示例 ID 定位并清理异常单元。 ");
        showDarkFramelessMessage(report.IsValid() ? QStringLiteral("拓扑检查通过")
                                                  : QStringLiteral("拓扑检查失败"),
                                     text, report.IsValid());
    };

    // ParaView 风格的标准 Filter 目录：同一个 QAction 同时出现在“常用”、
    // “按名称”和功能分类中。后续接入算法时只需替换这一处 triggered 回调。
    enum class StandardFilterCategory {
        DataAttributes,
        Geometry,
        Extraction,
        Sampling,
        Transform,
        Composite,
        MeshQuality
    };
    struct StandardFilterEntry {
        const char* id;
        const char* chineseName;
        StandardFilterCategory category;
        bool common;
    };

    // 保持按 id 的字母顺序，既方便“按名称”浏览，也方便检查清单是否完整。
    static const StandardFilterEntry standardFilterEntries[] = {
        {"angular_periodic", "角度周期", StandardFilterCategory::Transform, false},
        {"append_location_attributes", "附加位置属性", StandardFilterCategory::DataAttributes, false},
        {"append_reduce", "附加并归约", StandardFilterCategory::Composite, false},
        {"axis_aligned_reflection", "轴对齐反射", StandardFilterCategory::Transform, false},
        {"axis_aligned_transform", "轴对齐变换", StandardFilterCategory::Transform, false},
        {"boundary_mesh_quality", "边界网格质量", StandardFilterCategory::MeshQuality, false},
        {"cell_centers", "单元中心", StandardFilterCategory::Geometry, false},
        {"cell_quality", "单元质量", StandardFilterCategory::MeshQuality, true},
        {"cell_size", "单元尺寸", StandardFilterCategory::Geometry, false},
        {"clean_cells_to_grid", "清理单元为网格", StandardFilterCategory::Geometry, false},
        {"clean_poly_data", "清理多边形数据", StandardFilterCategory::Geometry, false},
        {"clean_to_grid", "清理为网格", StandardFilterCategory::Geometry, true},
        {"convert_to_vertex", "转换为顶点", StandardFilterCategory::Geometry, false},
        {"coordinates", "坐标", StandardFilterCategory::DataAttributes, false},
        {"count_cell_faces", "单元面数统计", StandardFilterCategory::Geometry, false},
        {"count_cell_vertices", "单元顶点数统计", StandardFilterCategory::Geometry, false},
        {"deflect_normals", "偏转法向量", StandardFilterCategory::DataAttributes, false},
        {"elevation", "高程", StandardFilterCategory::DataAttributes, true},
        {"extract_cells_by_region", "按区域提取单元", StandardFilterCategory::Extraction, false},
        {"extract_cells_by_type", "按类型提取单元", StandardFilterCategory::Extraction, false},
        {"extract_component", "提取分量", StandardFilterCategory::DataAttributes, false},
        {"extract_edges", "提取边", StandardFilterCategory::Geometry, true},
        {"extract_location", "提取位置", StandardFilterCategory::Extraction, false},
        {"extract_subset", "提取子集", StandardFilterCategory::Extraction, true},
        {"feature_edges", "特征边", StandardFilterCategory::Geometry, true},
        {"feature_edges_region_ids", "特征边区域标识符", StandardFilterCategory::DataAttributes, false},
        {"force_static_mesh", "强制静态网格", StandardFilterCategory::Geometry, false},
        {"generate_ids", "生成标识符", StandardFilterCategory::DataAttributes, false},
        {"ghost_cells", "幽灵单元", StandardFilterCategory::DataAttributes, false},
        {"global_point_and_cell_ids", "全局点与单元标识符", StandardFilterCategory::DataAttributes, false},
        {"iso_volume", "等值体", StandardFilterCategory::Extraction, true},
        {"mask", "掩码", StandardFilterCategory::Extraction, false},
        {"mask_points", "点掩码", StandardFilterCategory::Extraction, false},
        {"merge_vector_components", "合并向量分量", StandardFilterCategory::DataAttributes, false},
        {"mesh_quality", "网格质量", StandardFilterCategory::MeshQuality, true},
        {"multiblock_surface_as_multiblock", "多块表面保留多块结构", StandardFilterCategory::Composite, false},
        {"outline_corners", "轮廓角", StandardFilterCategory::Extraction, false},
        {"overlapping_cells_detector", "重叠单元检测", StandardFilterCategory::Geometry, false},
        {"pass_arrays", "传递数组", StandardFilterCategory::DataAttributes, true},
        {"point_and_cell_ids", "点与单元标识符", StandardFilterCategory::DataAttributes, false},
        {"point_line_interpolator", "点线插值器", StandardFilterCategory::Sampling, false},
        {"point_plane_interpolator", "点平面插值器", StandardFilterCategory::Sampling, false},
        {"point_set_to_octree_image", "点集转八叉树图像", StandardFilterCategory::Geometry, false},
        {"point_volume_interpolator", "点体积插值器", StandardFilterCategory::Sampling, false},
        {"probe", "探测", StandardFilterCategory::Sampling, true},
        {"probe_location", "位置探测", StandardFilterCategory::Sampling, false},
        {"process_ids", "进程标识符", StandardFilterCategory::DataAttributes, false},
        {"random_attributes", "随机属性", StandardFilterCategory::DataAttributes, false},
        {"random_vectors", "随机向量", StandardFilterCategory::DataAttributes, false},
        {"reflect", "反射", StandardFilterCategory::Transform, false},
        {"remove_ghost_information", "移除幽灵信息", StandardFilterCategory::DataAttributes, false},
        {"resample_to_image", "重采样到图像", StandardFilterCategory::Sampling, true},
        {"resample_to_line", "重采样到直线", StandardFilterCategory::Sampling, false},
        {"shrink", "收缩", StandardFilterCategory::Geometry, true},
        {"slice_with_plane", "平面切片", StandardFilterCategory::Extraction, true},
        {"surface_normals", "表面法向量", StandardFilterCategory::DataAttributes, true},
        {"threshold", "阈值", StandardFilterCategory::Extraction, true},
        {"transform", "变换", StandardFilterCategory::Transform, true},
        {"triangle_strips", "三角形条带", StandardFilterCategory::Geometry, false},
        {"validate_cells", "验证单元", StandardFilterCategory::Geometry, false},
        {"volume_of_revolution", "旋转体", StandardFilterCategory::Geometry, false},
    };

    QMenu* standardFilters =
            ui->menu_filters->addMenu(QStringLiteral("标准过滤器（Standard Filters）"));

    QMenu* commonFilters = standardFilters->addMenu(QStringLiteral("Common（常用）"));
    QMenu* alphabeticalFilters = standardFilters->addMenu(QStringLiteral("Alphabetical（按名称）"));
    standardFilters->addSeparator();

    QMenu* dataAttributeFilters =
            standardFilters->addMenu(QStringLiteral("Data Attributes & IDs（数据属性与标识）"));
    QMenu* geometryFilters =
            standardFilters->addMenu(QStringLiteral("Geometry & Mesh（几何与网格）"));
    QMenu* extractionFilters =
            standardFilters->addMenu(QStringLiteral("Extraction & Selection（提取与选择）"));
    QMenu* samplingFilters =
            standardFilters->addMenu(QStringLiteral("Sampling & Interpolation（采样与插值）"));
    QMenu* transformFilters =
            standardFilters->addMenu(QStringLiteral("Transform（变换）"));
    QMenu* compositeFilters =
            standardFilters->addMenu(QStringLiteral("Composite Data（复合数据）"));
    QMenu* meshQualityFilters =
            standardFilters->addMenu(QStringLiteral("Mesh Quality（网格质量）"));

    // 项目已有的 Filter 入口放在标准过滤器目录底部，执行逻辑保持不变。
    standardFilters->addSeparator();
    QMenu* mesh_processing =
            standardFilters->addMenu(QStringLiteral("Data Processing（数据处理）"));
    QMenu* convert = standardFilters->addMenu(QStringLiteral("Convert（数据转换）"));
    QMenu* view = standardFilters->addMenu(QStringLiteral("Feature Extraction（特征提取）"));

    // 全局下拉菜单为 12pt；标准 Filter 数量较多，局部缩小到 10pt，
    // 保留其他菜单的原有字号与样式。
    standardFilters->setStyleSheet(QStringLiteral("QMenu { font-size: 10pt; }"));

    auto categoryMenu = [&](StandardFilterCategory category) -> QMenu* {
        switch (category) {
            case StandardFilterCategory::DataAttributes: return dataAttributeFilters;
            case StandardFilterCategory::Geometry: return geometryFilters;
            case StandardFilterCategory::Extraction: return extractionFilters;
            case StandardFilterCategory::Sampling: return samplingFilters;
            case StandardFilterCategory::Transform: return transformFilters;
            case StandardFilterCategory::Composite: return compositeFilters;
            case StandardFilterCategory::MeshQuality: return meshQualityFilters;
        }
        return standardFilters;
    };

    auto currentFilterInput = [this](const QString& title) -> DataObject::Pointer {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) {
            showDarkFramelessMessage(title, QStringLiteral("请先加载并选择一个模型。"));
            return nullptr;
        }
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!obj) {
            showDarkFramelessMessage(title, QStringLiteral("当前模型没有可用数据。"));
            return nullptr;
        }
        return obj;
    };

    auto refreshFilterResult = [this](DataObject::Pointer input, DataObject::Pointer output,
                                      const QString& title, bool inPlace = false) {
        if (!output) {
            showDarkFramelessMessage(title, QStringLiteral("Filter 未产生有效输出。"));
            return;
        }

        if (inPlace || output.GetPointer() == input.GetPointer()) {
            modelTreeWidget->updateAllAttriubute(output);
            if (auto drawObj = DynamicCast<DrawObject>(output)) { drawObj->ForceReConvertToDrawableData(); }
        } else {
            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
        }
        rendererWidget->update();
        showDarkFramelessMessage(title, QStringLiteral("Filter 执行完成。"), true);
    };

    auto connectStandardFilterAction = [&](QAction* action, const QString& filterId) -> bool {
        if (connectImportedFilterAction(action, filterId)) return true;
        if (filterId == QStringLiteral("coordinates")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("坐标 (coordinates)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                auto filter = PointCoordinatesFilter::New();
                filter->SetInput(obj);
                filter->SetArrayName("Coordinates");
                if (!filter->Execute()) {
                    showDarkFramelessMessage(title, QStringLiteral("提取点坐标失败。"));
                    return;
                }
                // PointCoordinatesFilter returns an independent deep-copy output.
                // Add it to the model tree instead of treating it as an in-place update.
                refreshFilterResult(obj, filter->GetOutput(), title);
            });
            return true;
        }

        if (filterId == QStringLiteral("cell_centers")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("单元中心 (cell_centers)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                auto filter = CellCenterFilter::New();
                filter->SetInput(obj);
                if (!filter->Execute()) {
                    showDarkFramelessMessage(title, QStringLiteral("计算单元中心失败。"));
                    return;
                }
                refreshFilterResult(obj, filter->GetOutput(), title);
            });
            return true;
        }

        if (filterId == QStringLiteral("random_vectors")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("随机向量 (random_vectors)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                dialog->setFilterDescription(QStringLiteral("为每个点生成 BrownianVectors 三分量随机向量。"));
                int minId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                 QStringLiteral("最小速度"), "0");
                int maxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                 QStringLiteral("最大速度"), "1");
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    bool okMin = false, okMax = false;
                    const double minSpeed = dialog->getDouble(minId, okMin);
                    const double maxSpeed = dialog->getDouble(maxId, okMax);
                    if (!okMin || !okMax || maxSpeed < minSpeed) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效速度范围。"));
                        return;
                    }
                    auto filter = RandomVectorsFilter::New();
                    filter->SetInput(obj);
                    filter->SetMinimumSpeed(minSpeed);
                    filter->SetMaximumSpeed(maxSpeed);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QStringLiteral("生成随机向量失败。"));
                        return;
                    }
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("remove_ghost_information")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("移除 Ghost 信息 (remove_ghost_information)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                auto filter = RemoveGhostInformationFilter::New();
                filter->SetInput(obj);
                if (!filter->Execute()) {
                    showDarkFramelessMessage(title, QStringLiteral("当前数据不支持移除 Ghost 信息，或 Ghost 数组不合法。"));
                    return;
                }
                refreshFilterResult(obj, filter->GetOutput(), title, !filter->WasModified());
            });
            return true;
        }

        if (filterId == QStringLiteral("elevation")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("高程 (elevation)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                // 防御：包围盒无效时无法按轴预填默认低/高点
                const auto& bb = obj->GetBoundingBox();
                if (bb.isNull()) {
                    showDarkFramelessMessage(title, QStringLiteral("输入模型包围盒无效，无法计算高程。"));
                    return;
                }

                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                dialog->setFilterDescription(QStringLiteral("沿低点到高点的标尺线段计算投影高程标量（t 饱和于 [0,1] 后映射到标量范围），语义与 ParaView Elevation 一致。"));

                // X/Y/Z 轴按钮行（占参数区第一行）：互斥选中，默认选中 X；
                // 点击按包围盒铺满低/高点（被选轴取 min/max，其余轴取中心）
                auto* axisRow = new QWidget(dialog);
                auto* axisLayout = new QHBoxLayout(axisRow);
                axisLayout->setContentsMargins(0, 0, 0, 0);
                axisLayout->setSpacing(6);
                axisLayout->addWidget(new QLabel(QStringLiteral("投影轴"), axisRow));
                auto* axisGroup = new QButtonGroup(axisRow);
                axisGroup->setExclusive(true);
                QPushButton* axisBtns[3] = {};
                for (int i = 0; i < 3; ++i) {
                    auto* btn = new QPushButton(QString(QChar('X' + i)), axisRow);
                    btn->setCheckable(true);
                    axisGroup->addButton(btn);
                    axisLayout->addWidget(btn);
                    axisBtns[i] = btn;
                }
                axisBtns[0]->setChecked(true);
                dialog->addRowWidget(axisRow);

                // 8 个参数：低点 xyz、高点 xyz、标量范围下限/上限（默认按包围盒 X 轴铺满，范围 [0,1]）
                const auto center = bb.center();
                auto num = [](double v) { return QString::number(v); };
                std::array<int, 3> lowIds{}, highIds{};
                for (int i = 0; i < 3; ++i) {
                    lowIds[i] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("低点 ") + QString(QChar('X' + i)),
                                                     num(i == 0 ? bb.min[0] : center[i]));
                }
                for (int i = 0; i < 3; ++i) {
                    highIds[i] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("高点 ") + QString(QChar('X' + i)),
                                                      num(i == 0 ? bb.max[0] : center[i]));
                }
                int rangeLowId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("标量范围下限"), "0");
                int rangeHighId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("标量范围上限"), "1");

                // 按轴回填输入框（回填规则与参数面板 fillRangeByAxis 一致）
                auto fillByAxis = [dialog, bb, center, lowIds, highIds](int axis) {
                    for (int i = 0; i < 3; ++i) {
                        const double lowV = (i == axis) ? bb.min[i] : center[i];
                        const double highV = (i == axis) ? bb.max[i] : center[i];
                        if (auto* line = qobject_cast<QLineEdit*>(dialog->getWidget(lowIds[i]))) {
                            line->setText(QString::number(lowV));
                        }
                        if (auto* line = qobject_cast<QLineEdit*>(dialog->getWidget(highIds[i]))) {
                            line->setText(QString::number(highV));
                        }
                    }
                };
                for (int i = 0; i < 3; ++i) {
                    connect(axisBtns[i], &QPushButton::clicked, dialog, [fillByAxis, i]() { fillByAxis(i); });
                }
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    // 读取低点/高点 xyz（任一解析失败即中止）
                    bool okAll = true;
                    double lowPt[3] = {}, highPt[3] = {};
                    for (int i = 0; i < 3; ++i) {
                        bool okL = false, okH = false;
                        lowPt[i] = dialog->getDouble(lowIds[i], okL);
                        highPt[i] = dialog->getDouble(highIds[i], okH);
                        okAll = okAll && okL && okH;
                    }
                    bool okRL = false, okRH = false;
                    const double rLow = dialog->getDouble(rangeLowId, okRL);
                    const double rHigh = dialog->getDouble(rangeHighId, okRH);
                    if (!okAll || !okRL || !okRH) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效的低点、高点和标量范围。"));
                        return;
                    }
                    // 前置校验：低点与高点重合 -> 投影方向为零向量；标量范围必须下限 < 上限
                    const double vx = highPt[0] - lowPt[0], vy = highPt[1] - lowPt[1], vz = highPt[2] - lowPt[2];
                    if (vx * vx + vy * vy + vz * vz == 0.0) {
                        showDarkFramelessMessage(title, QStringLiteral("低点不能与高点重合。"));
                        return;
                    }
                    if (rLow >= rHigh) {
                        showDarkFramelessMessage(title, QStringLiteral("标量范围下限必须小于上限。"));
                        return;
                    }

                    auto filter = ElevationFilter::New();
                    filter->SetInput(obj);
                    filter->SetLowPoint(lowPt[0], lowPt[1], lowPt[2]);
                    filter->SetHighPoint(highPt[0], highPt[1], highPt[2]);
                    filter->SetScalarRange(rLow, rHigh);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QStringLiteral("生成高程标量失败。"));
                        return;
                    }
                    // 首次执行：独立输出挂模型树；随后绑定参数面板会话，供持续实时调整（对齐 ParaView Properties）
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    ElevationFilterPanel->BindSession(obj, filter);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("mask") || filterId == QStringLiteral("mask_points")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = filterId == QStringLiteral("mask")
                                              ? QStringLiteral("掩码 (mask)")
                                              : QStringLiteral("点掩码 (mask_points)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                dialog->setFilterDescription(QStringLiteral("按步长或随机方式采样点，输出点/顶点集合。"));
                int ratioId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                   QStringLiteral("采样步长 OnRatio"), "2");
                int maxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                 QStringLiteral("最大点数（0 为不限）"), "0");
                int randomId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                    QStringLiteral("随机采样"), "false");
                int seedId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                  QStringLiteral("随机种子"), "1");
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    bool okRatio = false, okMax = false, okRandom = false, okSeed = false;
                    const int ratio = dialog->getInt(ratioId, okRatio);
                    const int maxPoints = dialog->getInt(maxId, okMax);
                    const bool randomMode = dialog->getChecked(randomId, okRandom);
                    const int seed = dialog->getInt(seedId, okSeed);
                    if (!okRatio || !okMax || !okRandom || !okSeed || ratio <= 0 || maxPoints < 0) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效采样参数。"));
                        return;
                    }
                    auto filter = MaskPointsFilter::New();
                    filter->SetInput(obj);
                    filter->SetOnRatio(ratio);
                    filter->SetMaximumNumberOfPoints(maxPoints);
                    filter->SetRandomMode(randomMode);
                    filter->SetRandomModeType(MaskPointsFilter::RANDOM_SAMPLING);
                    filter->SetRandomSeed(static_cast<unsigned int>(std::max(seed, 0)));
                    filter->SetGenerateVertices(true);
                    filter->SetSingleVertexPerCell(true);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QStringLiteral("点掩码执行失败。该实现目前要求输入为非结构网格。"));
                        return;
                    }
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("feature_edges")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("特征边 (feature_edges)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                int angleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                   QStringLiteral("特征角度"), "30");
                int boundaryId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                      QStringLiteral("边界边"), "true");
                int featureId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                     QStringLiteral("折痕边"), "true");
                int nonManifoldId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                         QStringLiteral("非流形边"), "true");
                int manifoldId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                      QStringLiteral("流形边"), "false");
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    bool okAngle = false, okBoundary = false, okFeature = false, okNonManifold = false, okManifold = false;
                    auto filter = FeatureEdgesFilter::New();
                    filter->SetInput(obj);
                    filter->SetFeatureAngle(dialog->getDouble(angleId, okAngle));
                    filter->SetBoundaryEdges(dialog->getChecked(boundaryId, okBoundary));
                    filter->SetFeatureEdges(dialog->getChecked(featureId, okFeature));
                    filter->SetNonManifoldEdges(dialog->getChecked(nonManifoldId, okNonManifold));
                    filter->SetManifoldEdges(dialog->getChecked(manifoldId, okManifold));
                    if (!okAngle || !okBoundary || !okFeature || !okNonManifold || !okManifold) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效特征边参数。"));
                        return;
                    }
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QStringLiteral("提取特征边失败。"));
                        return;
                    }
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("extract_subset")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("提取子集 (extract_subset)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                auto structured = DynamicCast<StructuredMesh>(obj);
                if (!structured) {
                    showDarkFramelessMessage(title, QStringLiteral("Extract Subset 需要结构化网格输入。"));
                    return;
                }
                igIndex* size = structured->GetDimensionSize();
                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                int minI = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("min I"), "0");
                int maxI = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("max I"),
                                                QString::number(size[0] - 1));
                int minJ = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("min J"), "0");
                int maxJ = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("max J"),
                                                QString::number(size[1] - 1));
                int minK = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("min K"), "0");
                int maxK = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("max K"),
                                                QString::number(size[2] - 1));
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    bool ok[6] = {};
                    const int voi[6] = {
                            dialog->getInt(minI, ok[0]), dialog->getInt(maxI, ok[1]),
                            dialog->getInt(minJ, ok[2]), dialog->getInt(maxJ, ok[3]),
                            dialog->getInt(minK, ok[4]), dialog->getInt(maxK, ok[5])};
                    if (!ok[0] || !ok[1] || !ok[2] || !ok[3] || !ok[4] || !ok[5]) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效 VOI 范围。"));
                        return;
                    }
                    auto filter = ExtractSubsetFilter::New();
                    filter->SetInput(obj);
                    filter->SetVOI(voi[0], voi[1], voi[2], voi[3], voi[4], voi[5]);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QStringLiteral("提取结构化网格子集失败。"));
                        return;
                    }
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("outline_corners")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("轮廓角点 (outline_corners)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                int factorId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                    QStringLiteral("角长度比例"), "0.2");
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    const double factor = dialog->getDouble(factorId, ok);
                    if (!ok || factor <= 0.0) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效角长度比例。"));
                        return;
                    }
                    auto filter = OutlineCornerFilter::New();
                    filter->SetInput(obj);
                    filter->SetCornerFactor(static_cast<float>(factor));
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QString::fromStdString(filter->GetMessage()));
                        return;
                    }
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("probe") || filterId == QStringLiteral("probe_location")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = filterId == QStringLiteral("probe")
                                              ? QStringLiteral("探测 (probe)")
                                              : QStringLiteral("位置探测 (probe_location)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                const auto& bounds = obj->GetBoundingBox();
                const double cx = 0.5 * (bounds.min[0] + bounds.max[0]);
                const double cy = 0.5 * (bounds.min[1] + bounds.max[1]);
                const double cz = 0.5 * (bounds.min[2] + bounds.max[2]);
                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(title);
                int xId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("X"), QString::number(cx));
                int yId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("Y"), QString::number(cy));
                int zId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("Z"), QString::number(cz));
                int radiusId = -1;
                int countId = -1;
                if (filterId == QStringLiteral("probe")) {
                    radiusId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                    QStringLiteral("采样半径"), QString::number(bounds.diag() * 0.05));
                    countId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                   QStringLiteral("采样点数"), "100");
                }
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    bool okX = false, okY = false, okZ = false;
                    const double x = dialog->getDouble(xId, okX);
                    const double y = dialog->getDouble(yId, okY);
                    const double z = dialog->getDouble(zId, okZ);
                    if (!okX || !okY || !okZ) {
                        showDarkFramelessMessage(title, QStringLiteral("请输入有效探测位置。"));
                        return;
                    }
                    auto query = PointSet::New();
                    query->SetName(obj->GetName() + "_probe");
                    Point center(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                    if (filterId == QStringLiteral("probe")) {
                        bool okRadius = false, okCount = false;
                        const double radius = dialog->getDouble(radiusId, okRadius);
                        const int count = dialog->getInt(countId, okCount);
                        if (!okRadius || !okCount || radius < 0.0 || count <= 0) {
                            showDarkFramelessMessage(title, QStringLiteral("请输入有效采样半径和点数。"));
                            return;
                        }
                        ProbeFilter::GenerateSpherePoints(query, center, static_cast<float>(radius), count);
                    } else {
                        query->GetPoints()->AddPoint(center);
                    }
                    auto filter = ProbeFilter::New();
                    filter->SetInput(0, obj);
                    filter->SetInput(1, query);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(title, QStringLiteral("探测失败。当前数据可能没有可定位单元。"));
                        return;
                    }
                    refreshFilterResult(obj, filter->GetOutput(), title);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("convert_to_vertex")) {
            connect(action, &QAction::triggered, this, [=, this](bool) {
                const QString title = QStringLiteral("转换为顶点 (convert_to_vertex)");
                auto obj = currentFilterInput(title);
                if (!obj) return;
                auto filter = ConvertToVertexFilter::New();
                filter->SetInput(obj);
                if (!filter->Execute()) {
                    showDarkFramelessMessage(title, QStringLiteral("转换为顶点单元失败。"));
                    return;
                }
                refreshFilterResult(obj, filter->GetOutput(), title);
            });
            return true;
        }

        return false;
    };

    for (const StandardFilterEntry& entry: standardFilterEntries) {
        const QString filterId = QString::fromLatin1(entry.id);
        const QString actionText = QStringLiteral("%1（%2）")
                                           .arg(filterId, QString::fromUtf8(entry.chineseName));
        QAction* action = new QAction(actionText, standardFilters);
        action->setObjectName(QStringLiteral("action_filter_%1").arg(filterId));
        action->setData(filterId);
        action->setStatusTip(QStringLiteral("Filter 标识：%1").arg(filterId));

        alphabeticalFilters->addAction(action);
        categoryMenu(entry.category)->addAction(action);
        if (entry.common) commonFilters->addAction(action);

        if (connectStandardFilterAction(action, filterId)) continue;

        connect(action, &QAction::triggered, this, [this, action, filterId]() {
            showDarkFramelessMessage(
                    QStringLiteral("Filter 尚未接入"),
                    QStringLiteral("%1\n\n菜单入口已经创建，对应算法尚未接入项目。\nFilter 标识：%2")
                            .arg(action->text(), filterId),
                    true);
        });
    }
    ui->menu_filters->addSeparator();

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
                const QString detail = QString::fromStdString(filter->GetErrorMessage());
                result = detail.isEmpty()
                                 ? QStringLiteral("表面网格简化未能完成。")
                                 : detail;
                result += QStringLiteral(
                        "\n\n处理建议：\n"
                        "1. 检查并清理重复面、退化三角形和非流形边；\n"
                        "2. 降低简化比例，或取消“检查网格全部标量”后重试；\n"
                        "3. 若仍无法处理，可改用“快速表面简化”。");
                showDarkFramelessMessage(QStringLiteral("表面网格简化失败"), result);
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

    connect(mesh_processing->addAction(QStringLiteral("Fast Surface Simplification（快速表面简化）")), &QAction::triggered, this, [&](bool checked) {
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

    // connect(mesh_processing->addAction(QStringLiteral("表面简化 (Surface Simplification)")),
    //     &QAction::triggered, this, [&](bool checked) {
    //     auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

    //     SurfaceMesh::Pointer mesh;
    //     if (obj->GetDataObjectType() == IG_SURFACE_MESH) {
    //         mesh = DynamicCast<SurfaceMesh>(obj);
    //     } else if (obj->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
    //         mesh = DynamicCast<UnstructuredMesh>(obj)->TransferToSurfaceMesh();
    //     }

    //     MeshTriangulationFilter::Pointer triangulation = MeshTriangulationFilter::New();
    //     triangulation->SetInput(mesh);
    //     if (!triangulation->Execute()) return false;
    //     mesh = DynamicCast<SurfaceMesh>(triangulation->GetOutput());

    //     igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
    //     int reductionId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Reduction (0..1)", "0.05");
    //     int faceCountId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Target Face Count", "0");

    //     dialog->show();
    //     dialog->setApplyFunctor([=, this]() {
    //         bool ok;
    //         QString result = "";

    //         std::vector<FVector> V;
    //         std::vector<int> F;
    //         std::vector<std::vector<float>> A;
    //         std::vector<float> AW;

    //         float minv[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    //         float maxv[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    //         for (size_t i = 0; i < mesh->GetNumberOfPoints(); ++i) {
    //             auto& v = mesh->GetPoint(i);

    //             V.push_back({v[0], v[1], v[2]});

    //             for (int j = 0; j < 3; ++j) {
    //                 float vj = v[j];

    //                 minv[j] = minv[j] > vj ? vj : minv[j];
    //                 maxv[j] = maxv[j] < vj ? vj : maxv[j];
    //             }
    //         }

    //         float extent = 0.f;

    //         extent = (maxv[0] - minv[0]) < extent ? extent : (maxv[0] - minv[0]);
    //         extent = (maxv[1] - minv[1]) < extent ? extent : (maxv[1] - minv[1]);
    //         extent = (maxv[2] - minv[2]) < extent ? extent : (maxv[2] - minv[2]);

    //         float scale = extent == 0 ? 0.f : 1.f / extent;

    //         for (size_t i = 0; i < mesh->GetNumberOfPoints(); ++i) {
    //             V[i].x = (V[i].x - minv[0]) * scale;
    //             V[i].y = (V[i].y - minv[1]) * scale;
    //             V[i].z = (V[i].z - minv[2]) * scale;
    //         }

    //         igIndex ids[3];
    //         for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
    //             mesh->GetFacePointIds(i, ids);
    //             F.push_back(ids[0]);
    //             F.push_back(ids[1]);
    //             F.push_back(ids[2]);
    //         }

    //         for (int i = 0; i < mesh->GetAttributeSet()->GetNumberOfAttributes(); i++) {
    //             auto& attr = mesh->GetAttributeSet()->GetAttribute(i);
    //             int dim = attr.pointer->GetDimension();
    //             for (int d = 0; d < dim; d++) {
    //                 double val_max = -FLT_MAX;
    //                 double val_min = FLT_MAX;
    //                 for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
    //                     double val = attr.pointer->GetValue(j * dim + d);
    //                     val_max = val_max < val ? val : val_max;
    //                     val_min = val_min > val ? val : val_min;
    //                 }
    //                 if (val_min == val_max) {
    //                     std::vector<float> data(mesh->GetNumberOfPoints(), 0.f);
    //                     A.push_back(std::move(data));
    //                     AW.push_back(0.f);
    //                 } else {
    //                     std::vector<float> data;
    //                     for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
    //                         data.push_back(attr.pointer->GetValue(j * dim + d));
    //                     }
    //                     //for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
    //                     //    data.push_back((attr.pointer->GetValue(j * dim + d)));
    //                     //}
    //                     A.push_back(std::move(data));
    //                     AW.push_back(1.0f / (val_max - val_min));
    //                 }
    //             }
    //         }

    //         clock_t start = clock();
    //         MeshSaliencyCalculator saliencyCalculator(V, F);
    //         saliencyCalculator.Execute();

    //         MeshSimplifierWithAttributes simplifier(V, F, A, AW, saliencyCalculator.NormalCurvature,
    //                                                 dialog->getDouble(reductionId, ok));

    //         simplifier.SetUseVertexImportance(false);
    //         simplifier.SetUseDynamicAttributePenalty(true);

    //         simplifier.IsOptimizedPosition = true;
    //         simplifier.Execute();

    //         clock_t end = clock();
    //         std::cout << "Time taken: " << double(end - start) / CLOCKS_PER_SEC << " seconds." << std::endl;


    //         auto newMesh = SurfaceMesh::New();
    //         for (const auto& v: V) {
    //             newMesh->AddPoint(Point(v.x * extent + minv[0], v.y * extent + minv[1], v.z * extent + minv[2]));
    //         }
    //         //for (const auto& v: V) {
    //         //    newMesh->AddPoint(Point(v.x, v.y, v.z));
    //         //}
    //         auto CellArray = CellArray::New();
    //         for (int i = 0; i < F.size() / 3; i++) { CellArray->AddCellId3(F[i * 3], F[i * 3 + 1], F[i * 3 + 2]); }
    //         newMesh->SetFaces(CellArray);

    //         int count = 0;
    //         auto Attributes = AttributeSet::New();
    //         for (int i = 0; i < mesh->GetAttributeSet()->GetNumberOfAttributes(); i++) {
    //             auto& attr = mesh->GetAttributeSet()->GetAttribute(i);
    //             int dim = attr.pointer->GetDimension();
    //             auto arr = FloatArray::New();
    //             arr->SetDimension(dim);
    //             arr->Resize(mesh->GetNumberOfPoints());
    //             arr->SetName(attr.pointer->GetName());

    //             for (int d = 0; d < dim; d++) {
    //                 for (size_t j = 0; j < mesh->GetNumberOfPoints(); j++) {
    //                     arr->SetValue(j * dim + d, A[count + d][j]);
    //                 }
    //             }
    //             count += dim;

    //             Attributes->AddAttribute(attr.type, attr.attachmentType, arr);
    //         }
    //         newMesh->SetAttributeSet(Attributes);

    //         modelTreeWidget->addDataObjectToModelTree(newMesh, Algorithm);
    //         rendererWidget->update();
    //         dialog->close();
    //     });
    // });

    connect(mesh_processing->addAction(QStringLiteral("Surface Triangulation（表面三角化）")), &QAction::triggered, this, [this, showSurfaceTopologyReport](bool checked) {
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

        MeshTriangulationFilter::Pointer triangulation = MeshTriangulationFilter::New();
        triangulation->SetInput(obj);
        if (triangulation->Execute()) {
            auto mesh = DynamicCast<SurfaceMesh>(triangulation->GetOutput());

            modelTreeWidget->addDataObjectToModelTree(mesh, Algorithm);
            rendererWidget->update();
            showSurfaceTopologyReport(mesh, QStringLiteral("表面三角化后"));
        }
    });

    connect(mesh_processing->addAction(QStringLiteral("Surface Extraction（表面提取）")), &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!obj) return;

        auto filter = ConvertToSurfaceMeshFilter::New();
        filter->SetInput(obj);
        filter->SetConvertMethod(ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_MESH);
        if (!filter->Execute()) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前数据类型不支持表面提取。"));
            return;
        }

        auto surface = filter->GetSurfaceMesh();
        if (!surface) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("表面提取失败。"));
            return;
        }
        if (surface.GetPointer() == obj.GetPointer()) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前模型已经是表面网格，无需提取。"));
            return;
        }
        if (surface->GetNumberOfFaces() == 0) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("提取结果为空，当前模型没有可提取的表面单元。"));
            return;
        }

        surface->SetName(obj->GetName() + "_surface");
        modelTreeWidget->addDataObjectToModelTree(surface, Algorithm);
        rendererWidget->update();
    });

    connect(mesh_processing->addAction(QStringLiteral("Tetrahedralize（四面体化）")), &QAction::triggered, this, [&](bool checked) {
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!obj) return;

        MeshTetrahedralize::Pointer filter = MeshTetrahedralize::New();
        filter->SetInput(obj);
        filter->Execute();
        auto newMesh = filter->GetOutput();

        modelTreeWidget->addDataObjectToModelTree(newMesh, Algorithm);
        rendererWidget->update();
    });

    connect(mesh_processing->addAction(QStringLiteral("Volume Mesh Simplification（体网格简化）")), &QAction::triggered, this, [&](bool checked) {
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        auto in = DynamicCast<DataObject>(obj);
        if (!in) return;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
        dialog->setFilterTitle("四面体边坍缩简化");
        dialog->setFilterDescription("基于ADQ的边坍缩体网格简化（保留属性）");

        int reductionId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "简化比例 (0..1)", "0.5");
        int tetCountId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "目标四面体数量", "0");
        /*int boundaryPenaltyId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "边界惩罚", "100.0");
        int lambdaId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Lambda", "0.1");
        int preserveId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "保留边界", "false");*/
        int allAttrId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "使用所有点属性", "true");
        /*int stretchId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "拉伸因子", "10.0");
        int aspectId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "最大纵横比", "30.0");*/

        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            // ─── 1. 获取当前场景对象 ───
            auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
            if (!obj) {
                QString result = QString("未选择任何模型");
                showDarkFramelessMessage(QStringLiteral("错误"), result);
                dialog->close();
                return;
            }


            // ─── 2. 判断是否为纯四面体体网格 ───
            bool isPureTetMesh = false;

            if (obj->GetDataObjectType() == IG_VOLUME_MESH) {
                auto mesh = DynamicCast<VolumeMesh>(obj);
                if (mesh) {
                    isPureTetMesh = true;
                    igIndex ids[IGAME_CELL_MAX_SIZE];
                    const IGsize nVol = mesh->GetNumberOfVolumes();
                    for (IGsize i = 0; i < nVol; ++i) {
                        if (mesh->GetVolumePointIds(i, ids) != 4) {
                            isPureTetMesh = false;
                            break;
                        }
                    }
                }
            } else if (obj->GetDataObjectType() == IG_UNSTRUCTURED_MESH) {
                auto um = DynamicCast<UnstructuredMesh>(obj);
                if (um) {
                    isPureTetMesh = true;
                    const IGsize nCells = um->GetNumberOfCells();
                    for (IGsize i = 0; i < nCells; ++i) {
                        if (um->GetCellType(i) != IG_TETRA) {
                            isPureTetMesh = false;
                            break;
                        }
                    }
                }
            }

            if (!isPureTetMesh) {
                /*QMessageBox::information(this, "非纯四面体网格",
                                         "该简化算法只支持纯四面体体网格。\n"
                                         "请先执行「Tetrahedralize」将当前对象四面体化。");*/
                
                QString result = QString("该简化算法只支持纯四面体体网格。\n请先执行「Tetrahedralize」将当前对象四面体化。");
                showDarkFramelessMessage(QStringLiteral("非纯四面体网格"), result);
                    
                
                dialog->close();
                return;
            }


            bool ok = false;

            TetraEdgeSimplification::Pointer filter = TetraEdgeSimplification::New();
            filter->SetInput(in);
            filter->SetTargetReduction(float(dialog->getDouble(reductionId, ok)));
            filter->SetTargetTetraCount(dialog->getInt(tetCountId, ok));
            //filter->SetBoundaryPenalty(dialog->getDouble(boundaryPenaltyId, ok));
            //filter->SetLambda(dialog->getDouble(lambdaId, ok));
            //filter->SetPreserveBoundary(dialog->getChecked(preserveId, ok));
            filter->SetUseAllPointAttributes(dialog->getChecked(allAttrId, ok));
            //filter->SetStretchFactor(dialog->getDouble(stretchId, ok));
            //filter->SetMaxAspectRatio(dialog->getDouble(aspectId, ok));

            if (!filter->Execute()) {
                QMessageBox::information(this, "执行出错", "边坍缩简化失败");
                dialog->close();
                return;
            }

            auto out = filter->GetOutput(0);
            if (!out) {
                QMessageBox::information(this, "执行出错", "未生成输出结果");
                dialog->close();
                return;
            }

            modelTreeWidget->addDataObjectToModelTree(out, Algorithm);
            rendererWidget->update();
            dialog->close();
        });
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
    // 转换就地作用于「当前帧」的数据（普通模型=自身；PVD 等复合模型=当前挂载的所有子块），
    // 因为不再 addDataObjectToModelTree()，所以不会再出现“转换后多出一个同名模型”的问题。
    connect(convert->addAction(QStringLiteral("Convert To Point Data（转换为点数据）")), &QAction::triggered, this, [&](bool checked) {
        QString reason;
        QStringList names;
        const int created = createConvertedFrameModel(true, reason, names);
        if (created <= 0) {
            showDarkFramelessMessage(QStringLiteral("转换未完成"),
                                     reason.isEmpty() ? QStringLiteral("未能完成转换。") : reason);
            return;
        }
        showDarkFramelessMessage(
                QStringLiteral("转换完成"),
                QStringLiteral("已就地转换当前帧，模型改名为「%1」")
                        .arg(names.join(QStringLiteral("、"))),
                true);
    });
    connect(convert->addAction(QStringLiteral("Convert To Cell Data（转换为单元数据）")), &QAction::triggered, this, [&](bool checked) {
        QString reason;
        QStringList names;
        const int created = createConvertedFrameModel(false, reason, names);
        if (created <= 0) {
            showDarkFramelessMessage(QStringLiteral("转换未完成"),
                                     reason.isEmpty() ? QStringLiteral("未能完成转换。") : reason);
            return;
        }
        showDarkFramelessMessage(
                QStringLiteral("转换完成"),
                QStringLiteral("已就地转换当前帧，模型改名为「%1」")
                        .arg(names.join(QStringLiteral("、"))),
                true);
    });


    // ===== IsoVolume 等值面体提取 =====
    connect(extractionFilters->addAction(QStringLiteral("等值面体提取 (IsoVolume)")), &QAction::triggered, this,
            [&](bool checked) {
                if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("提示"), QStringLiteral("请先加载一个模型"));
                    return;
                }
                auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
                if (!obj || !obj->GetAttributeSet()) return;
                auto attrs = obj->GetAttributeSet()->GetAllPointAttributes();
                if (!attrs || attrs->GetNumberOfElements() == 0) {
                    showDarkFramelessMessage(QStringLiteral("提示"),
                                             QStringLiteral("当前模型没有点标量数据，无法进行等值面体提取"));
                    return;
                }

                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("等值面体提取 (IsoVolume)"));
                dialog->setFilterDescription(QStringLiteral("提取标量值落在 [lower, upper] 区间内的体数据"));

                std::vector<QString> arrNames;
                for (igIndex a = 0; a < attrs->GetNumberOfElements(); ++a) {
                    arrNames.push_back(QString::fromStdString(attrs->GetElement(a).pointer->GetName()));
                }
                int arrayId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                                                   QStringLiteral("点属性数组"), arrNames);

                std::vector<QString> comps0{QStringLiteral("分量 0")};
                int compId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                                                  QStringLiteral("标量分量"), comps0);
                int lowerId =
                        dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("lower"), "0");
                int upperId =
                        dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("upper"), "0");

                // 按 (数组, 分量) 刷新分量下拉框与 lower/upper 默认值
                auto updateRange = [=](int arrayIdx, int compIdx) {
                    if (arrayIdx < 0 || arrayIdx >= (int) attrs->GetNumberOfElements()) return;
                    auto& at = attrs->GetElement(arrayIdx);
                    auto arr = at.pointer;
                    if (!arr) return;
                    const int d = arr->GetDimension();
                    QComboBox* compCombo = dynamic_cast<QComboBox*>(dialog->getWidget(compId));
                    if (compCombo) {
                        compCombo->blockSignals(true);
                        compCombo->clear();
                        const int n = (d > 0 ? d : 1);
                        for (int i = 0; i < n; ++i) { compCombo->addItem(QStringLiteral("分量 %1").arg(i)); }
                        compCombo->setCurrentIndex(compIdx >= 0 && compIdx < n ? compIdx : 0);
                        compCombo->blockSignals(false);
                    }
                    const int c = compCombo ? compCombo->currentIndex() : 0;
                    auto range = at.GetDataRange();
                    double smin = 0.0, smax = 1.0;
                    if (range) {
                        const int base = 2 + 2 * c;
                        if (range->GetNumberOfElements() >= base + 2) {
                            smin = range->GetValue(base);
                            smax = range->GetValue(base + 1);
                        } else if (range->GetNumberOfElements() >= 2) {
                            smin = range->GetValue(0);
                            smax = range->GetValue(1);
                        }
                    }
                    if (smax <= smin) smax = smin + 1.0;
                    if (auto lo = dynamic_cast<QLineEdit*>(dialog->getWidget(lowerId))) {
                        lo->setText(QString::number(smin + (smax - smin) / 3.0));
                    }
                    if (auto up = dynamic_cast<QLineEdit*>(dialog->getWidget(upperId))) {
                        up->setText(QString::number(smin + (smax - smin) * 2.0 / 3.0));
                    }
                };
                updateRange(0, 0);

                QComboBox* arrCombo = dynamic_cast<QComboBox*>(dialog->getWidget(arrayId));
                QComboBox* compComboW = dynamic_cast<QComboBox*>(dialog->getWidget(compId));
                if (arrCombo) {
                    connect(arrCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                            [updateRange](int idx) { updateRange(idx, 0); });
                }
                if (compComboW) {
                    connect(compComboW, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                            [updateRange, arrCombo](int ci) {
                                const int ai = arrCombo ? arrCombo->currentIndex() : 0;
                                updateRange(ai, ci);
                            });
                }
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool okArr = false, okComp = false, okLower = false, okUpper = false;
                    int arrIdx = dialog->getComboIndex(arrayId, okArr);
                    int comp = dialog->getComboIndex(compId, okComp);
                    double lower = dialog->getDouble(lowerId, okLower);
                    double upper = dialog->getDouble(upperId, okUpper);
                    if (!okLower || !okUpper) {
                        showDarkFramelessMessage(QStringLiteral("提示"),
                                                 QStringLiteral("lower / upper 请输入有效数字"));
                        return;
                    }
                    if (lower > upper) { std::swap(lower, upper); }
                    if (arrIdx < 0 || arrIdx >= (int) attrs->GetNumberOfElements()) {
                        showDarkFramelessMessage(QStringLiteral("提示"), QStringLiteral("请选择有效的点属性数组"));
                        return;
                    }
                    auto array = attrs->GetElement(arrIdx).pointer;

                    auto filter = IsoVolumeFilter::New();
                    filter->SetInput(obj);
                    filter->SetIsoScalarData(array, lower, upper, comp);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(QStringLiteral("警告"),
                                                 QStringLiteral("等值面体提取执行失败，请检查数据与区间"));
                        return;
                    }
                    auto out = filter->GetOutput();
                    if (!out) return;
                    out->SetName(obj->GetName() + "_isovolume");
                    if (auto outMesh = DynamicCast<UnstructuredMesh>(out)) {
                        showDarkFramelessMessage(QStringLiteral("等值面体提取结果"),
                                                 QStringLiteral("输出 %1 点 / %2 单元（区间 [%3, %4]，含点合并）")
                                                         .arg(outMesh->GetNumberOfPoints())
                                                         .arg(outMesh->GetNumberOfCells())
                                                         .arg(lower)
                                                         .arg(upper));
                    }
                    modelTreeWidget->addDataObjectToModelTree(out, Algorithm);
                    rendererWidget->update();
                    dialog->close();
                });
            });

    auto runAdvancedGradient = [this]() {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        auto attrSet = data->GetAttributeSet();
        if (!attrSet || attrSet->GetNumberOfAttributes() <= 0) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("当前模型没有可用属性"));
            return;
        }

        // 自动选择一个属性：优先使用当前选中属性；
        // 如果没有选中属性，则优先选择向量属性（dim==3），否则选第一个属性
        int index = data->GetAttributeIndex();
        if (index < 0 || index >= static_cast<int>(attrSet->GetNumberOfAttributes())) {
            index = -1;
            for (int i = 0; i < static_cast<int>(attrSet->GetNumberOfAttributes()); ++i) {
                auto& attr = attrSet->GetAttribute(i);
                if (attr.pointer && attr.pointer->GetDimension() == 3) {
                    index = i;
                    break;
                }
            }
            if (index < 0) index = 0;
        }

        // 自动确定分量：标量用 0；向量如果没有选中分量，默认用第 0 个分量
        int component = data->GetCurrentAttributeDimension();
        int attrDim = attrSet->GetAttribute(index).pointer->GetDimension();
        if (attrDim == 1) {
            component = 0;
        } else if (component < 0) {
            component = 0;
        }

        AdvancedGradientFilter::Pointer filter = AdvancedGradientFilter::New();
        filter->SetInput(data);
        filter->SetAttributeByIndex(index);
        filter->SetCurrentAttributeDimension(component);
        // - 标量输入 → 3 分量向量
        // - 向量输入 → 9 分量张量
        // 输出点属性
        filter->SetComputeGradientTensor(true);
        filter->SetOutputToPointData(true);
        if (filter->Execute()) {
            auto drawObject = DynamicCast<DrawObject>(data);
            if (drawObject) {
                drawObject->GetColorMapper()->SetRangeStable(false);
            }
            modelTreeWidget->updateAllAttriubute(data);
            if (drawObject) {
                auto item = modelTreeWidget->getItemFromObject(data);
                if (item && item->childCount() > 0) {
                    // 选中新生成的 gradient 属性
                    int newIndex = static_cast<int>(data->GetAttributeSet()->GetNumberOfAttributes() - 1);
                    // 默认显示 magnitude，不显示 x/g0
                    int viewDim = -1;
                    item->setExpanded(true);
                    auto child = item->child(newIndex);
                    if (child) {
                        item->setCurrentChild(child);
                        item->setSelected(false);
                        item->viewAttribute(newIndex, viewDim);
                        // 强制重新生成可绘制颜色数据，避免只切换属性索引但颜色未刷新
                        drawObject->ForceReConvertToDrawableData();
                        child->setSelected(true);
                        modelTreeWidget->setCurrentItem(child);
                        rendererWidget->update();


                    }
                }
            }
        } else {
            std::string message = filter->GetMessage();
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
        }
    };

    QAction* gradient = view->addAction(QStringLiteral("Compute Gradient（计算梯度）"));
    connect(gradient, &QAction::triggered, this, [this, runAdvancedGradient](bool) {
        runAdvancedGradient();
    });

    QAction* laplacian = view->addAction(QStringLiteral("Compute Laplacian（计算拉普拉斯）"));
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

    QAction* curvature = view->addAction(QStringLiteral("Compute Curvature（计算曲率）"));
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

    QAction* vortex = view->addAction(QStringLiteral("Compute Vorticity（计算涡量）"));
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

    QAction* vortexPrection = view->addAction(QStringLiteral("Predict Vortex（涡旋预测）"));
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

    QMenu* attrDiffMenu = view->addMenu(QStringLiteral("属性差值(AttrDiffPerFrame)"));
    auto funcAttrDiff = [this](int mode) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!data) return;
        auto frames = data->PeekTimeFrames();
        const int frameNum = frames ? static_cast<int>(frames->GetTimeNum()) : 0;
        if (frameNum <= 1) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("时间帧序列数不足"));
            return;
        }
        int index = data->GetAttributeIndex();
        if (index < 0) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("模型树未选择属性"));
            return;
        }
        std::string attrName;
        if (auto attrSet = data->GetAttributeSet()) {
            if (index < static_cast<int>(attrSet->GetNumberOfAttributes())) {
                if (auto ptr = attrSet->GetAttribute(index).pointer) {
                    attrName = ptr->GetName();
                }
            }
        }
        if (attrName.empty()) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("属性无效"));
            return;
        }
        // 算过的帧尽量保存在缓存中
        ui->widget_Animation->setPreferredCacheNum(frameNum);
        if (frames->GetMaxCacheSize() < static_cast<unsigned int>(frameNum)) {
            frames->EnableCache(frameNum);
        }

        ui->widget_Animation->SetDiffMode(mode);
        int diffIndex = -1;
        for (int j = 0; j < frameNum; ++j) {
            data->UpdateAnimation(j);
            diffIndex = ui->widget_Animation->EnsureTimeDifferenceForCurrentFrame(data, attrName, j);
            if (diffIndex < 0) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("第 %1 帧的属性差值计算失败").arg(j + 1));
                return;
            }
        }
        // 重建属性子节点
        modelTreeWidget->updateAllAttriubute(data);
        if(auto drawObj = DynamicCast<DrawObject>(data)) {
            drawObj->ConvertToDrawableData();
        }
        // 自动选中diff
        auto item = modelTreeWidget->getItemFromObject(data);
        if (item && item->childCount() > diffIndex) {
            item->setExpanded(true);
            auto child = item->child(diffIndex);
            if (child) {
                item->setCurrentChild(child);
                item->setSelected(false);
                item->viewAttribute(diffIndex, -1);
                child->setSelected(true);
                modelTreeWidget->setCurrentItem(child);
            }
         }
        // 在模型树操作之后,setCurrentItem触发CurrendModelChanged→initAnimationComponents，
        // 若提前开启会被当成尚未绑定模型而立即关掉。
        ui->widget_Animation->SetDiffAutoCompute(true, attrName);
    };
    QAction* attrDiffSigned = attrDiffMenu->addAction(QStringLiteral("带符号差"));
    connect(attrDiffSigned, &QAction::triggered, this, [funcAttrDiff](bool) { funcAttrDiff(0); });
    QAction* attrDiffAbs = attrDiffMenu->addAction(QStringLiteral("绝对差"));
    connect(attrDiffAbs, &QAction::triggered, this, [funcAttrDiff](bool) { funcAttrDiff(1); });
    QAction* attrDiffRel = attrDiffMenu->addAction(QStringLiteral("相对变化率"));
    connect(attrDiffRel, &QAction::triggered, this, [funcAttrDiff](bool) { funcAttrDiff(2); });

    QAction* lagrangeUnstructedMesh_visualization = convert->addAction(
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
        testAction->setVisible(false);
        connect(testAction, &QAction::triggered, this, [&](bool checked) {
#define TEST_MAP_BACK
#ifdef TEST_MAP_BACK
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (!model || !model->GetDataObject()) {
                std::cout << "[Block Mapping Test] No model selected." << std::endl;
                return;
            }

            auto obj = iGame::FileIO::ReadFile("C:\\Users\\26280\\Downloads\\output_p600000_pr4000_t0.95_s42_npp.vtk");
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
    // PartSegmentation零件分割
    {
        QAction* partSegmentationAction{};
        partSegmentationAction = new QAction(this);
        partSegmentationAction->setObjectName(QString::fromUtf8("零件分割"));
        partSegmentationAction->setText(QString::fromUtf8("零件分割"));
        ui->menu_filters->addAction(partSegmentationAction);
        partSegmentationAction->setVisible(true);
        connect(partSegmentationAction, &QAction::triggered, this, [&](bool checked) {
            // 弹出 IP/Port 配置对话框
            QSettings settings("iGame", "iGameVis");
            QString savedHost = settings.value("P3SAM/host", "127.0.0.1").toString();
            int     savedPort = settings.value("P3SAM/port", 8765).toInt();

            igQtChromeFramelessDialog cfgDlg(this);
            cfgDlg.setDialogTitle(QStringLiteral("零件分割 - 服务器配置"));
            cfgDlg.setMaximizeEnabled(false);

            auto* body = new QWidget(cfgDlg.contentHost());
            body->setAttribute(Qt::WA_StyledBackground, true);
            body->setStyleSheet(
                "QWidget { background-color: transparent; color: #EAEAEA; }"
                "QLabel { color: #D8D8D8; }"
                "QLineEdit { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A;"
                "            padding: 4px 6px; border-radius: 3px; }"
                "QLineEdit:focus { border: 1px solid #5A7FA8; }"
                "QPushButton { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A;"
                "              padding: 6px 16px; border-radius: 4px; }"
                "QPushButton:hover { background-color: #3A3A3A; }"
                "QPushButton:pressed { background-color: #252526; }");

            auto* form = new QFormLayout(body);
            form->setContentsMargins(12, 12, 12, 12);
            form->setSpacing(10);
            auto* hostEdit = new QLineEdit(savedHost, body);
            auto* portEdit = new QLineEdit(QString::number(savedPort), body);
            portEdit->setValidator(new QIntValidator(1, 65535, body));
            form->addRow(QStringLiteral("服务器 IP："), hostEdit);
            form->addRow(QStringLiteral("端口："), portEdit);
            auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, body);
            form->addRow(btns);

            cfgDlg.setContentWidget(body);
            cfgDlg.resize(300, 160);

            bool accepted = false;
            QObject::connect(btns, &QDialogButtonBox::accepted, &cfgDlg, [&]() { accepted = true; cfgDlg.accept(); });
            QObject::connect(btns, &QDialogButtonBox::rejected, &cfgDlg, &QDialog::reject);
            cfgDlg.exec();
            if (!accepted) return;

            QString host = hostEdit->text().trimmed();
            int     port = portEdit->text().toInt();
            if (host.isEmpty()) host = "127.0.0.1";
            settings.setValue("P3SAM/host", host);
            settings.setValue("P3SAM/port", port);

            // 检查当前模型
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (!model) {
                std::cout << "[PartSegmentation] No model selected." << std::endl;
                return;
            }

            auto dataObj = model->GetDataObject();

            std::cout << "[PartSegmentation] Starting P3SAM segmentation..." << std::endl;
            P3SAMSegmenter::Pointer segmenter = P3SAMSegmenter::New();
            segmenter->SetServerHost(host.toStdString());
            segmenter->SetServerPort(port);
            segmenter->SetInput(dataObj);
            segmenter->SetSimplificationRatio(0.1f);
            segmenter->SetPostProcess(false);
            segmenter->SetTimeout(300000);  // 5分钟超时

            if (!segmenter->Execute()) {
                std::cout << "[PartSegmentation] Segmentation failed: "
                          << segmenter->GetErrorMessage() << std::endl;
                return;
            }

            modelTreeWidget->updateAllAttriubute(dataObj);
            std::cout << "[PartSegmentation] Segmentation complete! Parts: "
                      << segmenter->GetPartCount() << std::endl;
        });
    }
    // 零件分割（自文件）
    {
        QAction* partSegmentationAction_fromFile{};
        partSegmentationAction_fromFile = new QAction(this);
        partSegmentationAction_fromFile->setObjectName(QString::fromUtf8("零件分割(自文件)"));
        partSegmentationAction_fromFile->setText(QString::fromUtf8("零件分割(自文件)"));
        ui->menu_filters->addAction(partSegmentationAction_fromFile);
        partSegmentationAction_fromFile->setVisible(true);
        connect(partSegmentationAction_fromFile, &QAction::triggered, this, [&](bool checked) {
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (!model) {
                std::cout << "[PartSegFromFile] No model selected." << std::endl;
                return;
            }
            auto dataObj = model->GetDataObject();

            QString path = QFileDialog::getOpenFileName(
                this, QStringLiteral("选择分割结果文件"), QString(), "VTK Files (*.vtk);;All Files (*)");
            if (path.isEmpty()) return;

            DataObject::Pointer segmented = FileIO::ReadFile(path.toStdString());
            UnstructuredMesh::Pointer segMesh = DynamicCast<UnstructuredMesh>(segmented);
            if (!segMesh) {
                std::cout << "[PartSegFromFile] File is not an UnstructuredMesh: "
                          << path.toStdString() << std::endl;
                return;
            }

            IntArray::Pointer resultArray;
            IGenum type = dataObj->GetDataObjectType();
            switch (type) {
                case IG_SURFACE_MESH: {
                    SurfaceMesh::Pointer sm = DynamicCast<SurfaceMesh>(dataObj);
                    if (!sm) { std::cout << "[PartSegFromFile] Failed to cast to SurfaceMesh." << std::endl; return; }
                    resultArray = BlockMapping::GetMappingBlockCellsArray(sm, segMesh);
                    break;
                }
                case IG_UNSTRUCTURED_MESH: {
                    UnstructuredMesh::Pointer um = DynamicCast<UnstructuredMesh>(dataObj);
                    if (!um) { std::cout << "[PartSegFromFile] Failed to cast to UnstructuredMesh." << std::endl; return; }
                    resultArray = BlockMapping::GetMappingBlockCellsArray(um, segMesh);
                    break;
                }
                case IG_VOLUME_MESH: {
                    VolumeMesh::Pointer vm = DynamicCast<VolumeMesh>(dataObj);
                    if (!vm) { std::cout << "[PartSegFromFile] Failed to cast to VolumeMesh." << std::endl; return; }
                    resultArray = BlockMapping::GetMappingBlockCellsArray(vm, segMesh);
                    break;
                }
                default:
                    std::cout << "[PartSegFromFile] Unsupported mesh type." << std::endl;
                    return;
            }

            if (!resultArray) {
                std::cout << "[PartSegFromFile] Block mapping failed." << std::endl;
                return;
            }

            resultArray->SetName("part_id");
            dataObj->SetBlockMapping(resultArray);
            modelTreeWidget->updateAllAttriubute(dataObj);
            std::cout << "[PartSegFromFile] Done." << std::endl;
        });
    }
    // 零件聚焦弹窗
    {
        QAction* partFocusAction = new QAction(this);
        partFocusAction->setObjectName(QString::fromUtf8("零件聚焦"));
        partFocusAction->setText(QString::fromUtf8("零件聚焦"));
        ui->menu_filters->addAction(partFocusAction);
        partFocusAction->setVisible(true);
        connect(partFocusAction, &QAction::triggered, this, [&]() {
            if (!partFocusDialog) {
                partFocusDialog = new igQtChromeFramelessDialog(this);
                partFocusDialog->setDialogTitle(QStringLiteral("零件聚焦"));
                partFocusDialog->setMaximizeEnabled(false);
                partFocusWidget = new igQtPartFocusWidget(partFocusDialog->contentHost());
                connect(partFocusWidget, &igQtPartFocusWidget::SIGNAL_SelectedPartsChanged, this,
                        [this](const QVector<int>& partIds) {
                            auto model = rendererWidget->GetScene()->GetCurrentModel();
                            ui->widget_SearchInfo->setCurrentModel(model);
                            ui->widget_SearchInfo->setSelectedPartIds(partIds);
                        });
                partFocusDialog->setContentWidget(partFocusWidget);
                partFocusDialog->resize(340, 380);
            }
            partFocusWidget->SetScene(rendererWidget->GetScene(), rendererWidget);
            partFocusDialog->show();
            partFocusDialog->raise();
            partFocusDialog->activateWindow();
        });
    }
    // 报告生成
    {
        QAction* reportGenerateAction = new QAction(this);
        reportGenerateAction->setObjectName(QString::fromUtf8("报告生成"));
        reportGenerateAction->setText(QString::fromUtf8("报告生成"));
        ui->menu_help->addAction(reportGenerateAction);
        reportGenerateAction->setVisible(true);
        connect(reportGenerateAction, &QAction::triggered, this, [&]() {
            if (!reportGenerateDialog) {
                reportGenerateDialog = new igQtChromeFramelessDialog(this);
                reportGenerateDialog->setDialogTitle(QStringLiteral("报告生成"));
                reportGenerateDialog->setMaximizeEnabled(false);
                reportGenerateWidget = new igQtAttributeSelectWidget(reportGenerateDialog->contentHost());
                reportGenerateWidget->SetMaxSelectableCount(1);
                reportGenerateDialog->setContentWidget(reportGenerateWidget);
                reportGenerateDialog->resize(340, 380);
            }
            reportGenerateWidget->RefreshAttributeList();
            reportGenerateDialog->show();
            reportGenerateDialog->raise();
            reportGenerateDialog->activateWindow();
        });
    }
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
    connect(fileLoader, &igQtFileLoader::RemoteCachedDatasetReattach, this,
            [this](DataObject::Pointer object) {
        if (!modelTreeWidget->getItemFromObject(object)) {
            modelTreeWidget->addDataObjectToModelTree(object, ItemSource::File);
        }
    }, Qt::DirectConnection);
    connect(fileLoader, &igQtFileLoader::RemoteCachedDatasetDetach, this,
            [this](DataObject::Pointer object, bool* detached) {
        auto* item = modelTreeWidget->getItemFromObject(object);
        if (!item) { return; }
        modelTreeWidget->setCurrentItem(item);
        modelTreeWidget->deleteCurrentModel();
        *detached = modelTreeWidget->getItemFromObject(object) == nullptr;
        igDebug("[RemoteMemoryCache] Detach tree verification: item_present={}",
                modelTreeWidget->getItemFromObject(object) != nullptr);
    }, Qt::DirectConnection);
    connect(fileLoader, &igQtFileLoader::RemoteRenderRequested,
            rendererWidget, &igQtRenderWidget::RequestCompletedFrame);
    connect(fileLoader, &igQtFileLoader::RemoteRenderCancelled,
            rendererWidget, &igQtRenderWidget::CancelCompletedFrame);
    connect(rendererWidget, &igQtRenderWidget::CompletedFrame,
            fileLoader, &igQtFileLoader::NotifyRemoteFrameCompleted);
    connect(fileLoader, &igQtFileLoader::RemoteCachedModelSelected, this,
            [this](DataObject::Pointer object) {
        auto item = modelTreeWidget->getItemFromObject(object);
        if (item) {
            item->setExpanded(true);
            // Selection only. Do NOT call viewAttribute()/FinishReading():
            // those would regenerate the complete scalar color arrays.
            modelTreeWidget->setCurrentItem(item);
        }
        rendererWidget->update();
        rendererWidget->getColorBarWidget()->update();
    });
    // connect(rendererWidget, &igQtModelDrawWidget::insertToModelListView,
    // ui->modelTreeView, &igQtModelListView::InsertModel);

    connect(fileLoader, &igQtFileLoader::NewModel, modelTreeWidget, &igQtModelDialogWidget::addDataObjectToModelTree);
    connect(fileLoader, &igQtFileLoader::FinishReading, this, &igQtMainWindow::updateRecentFilePaths);
    connect(ui->action_DeleteMesh, &QAction::triggered, modelTreeWidget, &igQtModelDialogWidget::deleteCurrentModel);
    connect(ui->action_DeleteMesh, &QAction::triggered, fileLoader, &igQtFileLoader::ReleaseDetachedRemoteGpuResources);

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
    connect(fileLoader, &igQtFileLoader::FinishReading, DeformationWidget, [this](){
        DeformationWidget->updateInfo();
    });

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
    // 删除高程输出节点时自动关闭右侧 Elevation 参数面板
    connect(this->modelTreeWidget, &igQtModelDialogWidget::ModelDeleted,
            ElevationFilterPanel, &igQtElevationFilterPanel::onModelDeleted);

    // Update animation controls when model changes
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged,
            ui->widget_Animation, &igQtAnimationWidget::initAnimationComponents);
    // Update Deformation Info when model changes
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged,
            DeformationWidget, &igQtDeformationWidget::updateInfo);

    // Refresh the stream-tracer vector list when the current model changes.
    // 只在面板可见时刷新：pickSourceModel/isUsableSource 会遍历全部单元，
    // 模型树点击很频繁，大网格上无谓地跑一遍代价不小。
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [this]() {
        if (!ui->widget_FlowField || !ui->widget_FlowField->isVisible()) return;
        ui->widget_FlowField->refreshVectorCombo();
    });

    // Rebind the contour-extraction panel when the current model changes
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [this]() {
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        if (!scene) return;
        auto model = scene->GetCurrentModel();
        if (!model) return;
        auto dataObject = model->GetDataObject();
        if (!dataObject) return;
        ui->widget_ContourExtract->SetOriginDataObject(dataObject);
    });

    /* Model Tree signal connect END.*/

    connect(ui->widget_ScalarField, &igQtScalarViewWidget::ChangeShowColorManager, this, [&]() {
        if (this->ColorManagerWidget->isHidden()) {
            // 打开前从当前模型 ColorMapper 同步色条（无模型会提示）
            this->ColorManagerWidget->resetColorRange();
            this->ColorManagerWidget->show();
            this->ColorManagerWidget->raise();
            this->ColorManagerWidget->activateWindow();
        } else {
            this->ColorManagerWidget->hide();
        }
    });

    connect(this->ColorManagerWidget, &igQtColorManagerWidget::UpdateColorBarFinished, this, [&]() {
        // SetColorMap + Modified 后刷新顶点色与图例
        ui->widget_ScalarField->updateDrawStyle();
        if (this->rendererWidget && this->rendererWidget->getColorBarWidget()) {
            this->rendererWidget->getColorBarWidget()->update();
        }
        if (this->rendererWidget) { this->rendererWidget->update(); }
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
    if (!ui || !ui->menu_RecentFiles || !fileLoader) return;

    QMenu* menu = ui->menu_RecentFiles;
    const QList<QAction*> oldActions = menu->actions();
    for (QAction* a : oldActions) {
        if (a && a->property("igRecentFileEntry").toBool()) menu->removeAction(a);
    }

    const QList<QAction*> list = fileLoader->GetRecentActionList();
    for (int i = list.size() - 1; i >= 0; --i) {
        QAction* a = list.at(i);
        if (!a) continue;
        if (menu->actions().contains(a)) continue;
        a->setProperty("igRecentFileEntry", true);
        menu->addAction(a);
    }
}

void igQtMainWindow::applyTopMenuButtonStyle() {
    const bool menuLight = isLightStyle(m_styleMode);
    if (m_brandBox) {
        m_brandBox->setStyleSheet(menuLight
                ? QStringLiteral("QWidget#TitleBrandBox { background-color: rgba(0,0,0,0.05); border: 1px solid rgba(0,0,0,0.10); border-radius: 6px; }")
                : QStringLiteral("QWidget#TitleBrandBox { background-color: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.10); border-radius: 6px; }"));
    }
    if (ui && ui->menuBar) {
        ui->menuBar->setStyleSheet(menuLight
                ? QStringLiteral("QMenuBar { background: transparent; color: #1F2A3A; border: none; }"
                                 "QMenuBar::item { background: transparent; padding: 3px 10px; }"
                                 "QMenuBar::item:selected { background-color: rgba(0,0,0,0.08); border-radius: 4px; }")
                : QStringLiteral("QMenuBar { background: transparent; color: #D4D4D4; border: none; }"
                                 "QMenuBar::item { background: transparent; padding: 3px 10px; }"
                                 "QMenuBar::item:selected { background-color: rgba(255,255,255,0.12); border-radius: 4px; }"));
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

QString igQtMainWindow::styleSheetForMode(int mode) const {
    if (mode == 12) return loadQssResource(QStringLiteral(":/Styles/iGameVisFloatingDark.qss"));
    if (mode == 13) return loadQssResource(QStringLiteral(":/Styles/iGameVisFloatingLight.qss"));
    if (mode == 14) return loadQssResource(QStringLiteral(":/Styles/iGameVisFloatingGraphiteModern.qss"));
    if (mode == 15) return loadQssResource(QStringLiteral(":/Styles/iGameVisFloatingMatteGraphite.qss"));
    return m_originalStyleSheet;
}

QString igQtMainWindow::styleToggleButtonQss() const {
    const int fam = styleColorFamily(m_styleMode);
    if (fam == 1) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: rgba(56, 189, 248, 0.12);"
                " color: #7FD4FF;"
                " border: 1px solid rgba(56, 189, 248, 0.45);"
                " border-radius: 8px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: rgba(56, 189, 248, 0.22);"
                " border-color: rgba(56, 189, 248, 0.70);"
                "}");
    }
    if (fam == 2) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: rgba(37, 99, 235, 0.10);"
                " color: #2563EB;"
                " border: 1px solid rgba(37, 99, 235, 0.45);"
                " border-radius: 8px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: rgba(37, 99, 235, 0.18);"
                " border-color: rgba(37, 99, 235, 0.70);"
                "}");
    }
    if (fam == 4) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: rgba(77, 208, 225, 0.14);"
                " color: #5CE1F0;"
                " border: 1px solid rgba(77, 208, 225, 0.48);"
                " border-radius: 8px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: rgba(77, 208, 225, 0.24);"
                " border-color: rgba(77, 208, 225, 0.72);"
                "}");
    }
    if (fam == 6 || fam == 8) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: rgba(77, 208, 225, 0.14);"
                " color: #5CE1F0;"
                " border: 1px solid rgba(77, 208, 225, 0.48);"
                " border-radius: 8px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: rgba(77, 208, 225, 0.24);"
                " border-color: rgba(77, 208, 225, 0.72);"
                "}");
    }
    if (fam == 9) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: rgba(108, 142, 174, 0.12);"
                " color: #9FB6C9;"
                " border: 1px solid rgba(108, 142, 174, 0.38);"
                " border-radius: 6px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: rgba(108, 142, 174, 0.22);"
                " border-color: rgba(108, 142, 174, 0.60);"
                "}");
    }
    if (fam == 10) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: rgba(42, 48, 58, 0.30);"
                " color: #A2A8B0;"
                " border: 1px solid #2C3038;"
                " border-radius: 4px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: #2A303A;"
                " border-color: #3A414C;"
                "}");
    }
    if (fam == 11 || fam == 12) {
        return QStringLiteral(
                "QPushButton#StyleToggleButton {"
                " background-color: #252526;"
                " color: #858585;"
                " border: 1px solid #2D2D30;"
                " border-radius: 4px;"
                " font-size: 10pt;"
                " padding: 0 12px;"
                "}"
                "QPushButton#StyleToggleButton:hover {"
                " background-color: #2A2A2C;"
                " border-color: #37373D;"
                "}");
    }
    return QStringLiteral(
            "QPushButton#StyleToggleButton {"
            " background-color: #2A2A2A;"
            " color: #CCCCCC;"
            " border: 1px solid #3C3C3C;"
            " border-radius: 6px;"
            " font-size: 10pt;"
            " padding: 0 10px;"
            "}"
            "QPushButton#StyleToggleButton:hover {"
            " background-color: #3A3A3A;"
            " border-color: #555555;"
            "}");
}

QString igQtMainWindow::styleModeDisplayName(int mode) const {
    if (mode == 12) return QStringLiteral("✦ 深灰");
    if (mode == 13) return QStringLiteral("✦ 浅白");
    if (mode == 14) return QStringLiteral("✦ 石墨");
    if (mode == 15) return QStringLiteral("✦ 哑光");
    return QStringLiteral("✦ 深灰");
}

void igQtMainWindow::createStyleMenu() {
    if (m_styleMenu) return;
    m_styleMenu = new QMenu(m_styleToggleButton);
    static const int kStyleModes[] = {12, 13, 14, 15};
    for (const int mode : kStyleModes) {
        QAction* act = m_styleMenu->addAction(styleModeDisplayName(mode));
        act->setCheckable(true);
        act->setData(mode);
        connect(act, &QAction::triggered, this, [this, mode]() {
            applyStyleMode(mode);
            QSettings settings(QStringLiteral("iGame"), QStringLiteral("iGameVis"));
            settings.setValue(QStringLiteral("ui/styleMode"), mode);
        });
    }
    m_styleToggleButton->setMenu(m_styleMenu);
}

void igQtMainWindow::updateTitleBarIcons() {
    const bool light = isLightStyle(m_styleMode);
    if (m_btnMinimize) {
        m_btnMinimize->setIcon(QIcon(light ? QStringLiteral(":/Ticon/Icons/window_minimize_dark.svg")
                                           : QStringLiteral(":/Ticon/Icons/window_minimize_white.svg")));
    }
    updateMaximizeButtonIcon();
}

void igQtMainWindow::applyStyleMode(int mode) {
    mode = normalizeStyleMode(qBound(0, mode, 15));
    if (styleSheetForMode(mode).isEmpty()) {
        mode = kFallbackStyleMode;
    }

    if (m_styleMode == 6 && mode != 6 && modelTreeWidget) {
        applyWorkspaceLayout(false);
    }
    if (isFloatingCardStyle(m_styleMode) && !isFloatingCardStyle(mode)) {
        if (modelTreeWidget && modelTreeWidget->isTreeDockCollapsed()) {
            modelTreeWidget->setTreeDockCollapsed(false);
        }
        applyFloatingCards(false);
    }
    if ((m_styleMode == 7 || m_styleMode == 8) && mode != 7 && mode != 8) {
        applyViewRail(false);
    }
    m_styleMode = mode;
    igQtRenderWidget::setGlobalStyleMode(m_styleMode);
    this->setStyleSheet(styleSheetForMode(mode));

    if (m_styleToggleButton) {
        m_styleToggleButton->setText(styleModeDisplayName(mode));
        m_styleToggleButton->setStyleSheet(styleToggleButtonQss());
    }
    applyTopMenuButtonStyle();
    QString accent = QStringLiteral("#4DD0E1");
    switch (styleColorFamily(mode)) {
        case 0:  accent = QStringLiteral("#3C3C3C"); break;
        case 1:  accent = QStringLiteral("#38BDF8"); break;
        case 2:  accent = QStringLiteral("#2563EB"); break;
        case 3:  accent = QStringLiteral("#4A4E54"); break;
        case 9:  accent = QStringLiteral("#6C8EAE"); break;
        case 10: accent = QStringLiteral("#3A414C"); break;
        case 11: accent = QStringLiteral("#37373D"); break;
        case 12: accent = QStringLiteral("#37373D"); break;
        default: accent = QStringLiteral("#4DD0E1"); break;
    }
    const bool isLight = isLightStyle(mode);

    if (m_logoIconLabel) {
        QString grad;
        if (isLight) {
            grad = QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #FFFFFF, stop:1 #E9EFF9)");
        } else if (styleColorFamily(mode) == 10) {
            grad = QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #2A303A, stop:1 #1C1F25)");
        } else if (styleColorFamily(mode) >= 11) {
            grad = QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #2A2A2C, stop:1 #1E1E1E)");
        } else {
            grad = QStringLiteral("qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #4DD0E1, stop:1 #2B7CD3)");
        }
        m_logoIconLabel->setStyleSheet(
                QStringLiteral("QLabel#AppLogoLabel { background: %1; border-radius: 8px; padding: 3px; }")
                        .arg(grad));
    }

    if (m_projectChip) {
        if (isLight) {
            m_projectChip->setStyleSheet(
                    "QLabel#ProjectChip { color: #1F2A3A; background-color: rgba(37,99,235,0.08);"
                    " border: 1px solid rgba(37,99,235,0.30); border-radius: 999px; padding: 3px 10px; font-size: 10pt; }");
        } else {
            m_projectChip->setStyleSheet(
                    QStringLiteral("QLabel#ProjectChip { color: #B8C0CA; background-color: rgba(255,255,255,0.05);"
                                   " border: 1px solid %1; border-radius: 999px; padding: 3px 10px; font-size: 10pt; }")
                            .arg(accent));
        }
    }

    if (m_titleAccentLine) {
        m_titleAccentLine->setStyleSheet(
                QStringLiteral("QFrame#TitleBarAccentLine { background-color: %1; }").arg(accent));
    }

    if (m_rightDivider) {
        m_rightDivider->setStyleSheet(
                isLight
                        ? "QFrame#RightDivider { background-color: rgba(0,0,0,0.12); }"
                        : "QFrame#RightDivider { background-color: rgba(255,255,255,0.12); }");
    }
    if (m_styleMenu) {
        const QList<QAction*> acts = m_styleMenu->actions();
        for (QAction* act : acts) {
            act->setChecked(act->data().toInt() == mode);
        }
    }
    updateTitleBarIcons();
    if (rendererWidget) rendererWidget->applyThemeBackground();
    if (isFloatingCardStyle(m_styleMode)) applyFloatingCardPalette();
    if (modelTreeWidget) modelTreeWidget->refreshStyle();

    if (mode == 6 && modelTreeWidget) {
        applyWorkspaceLayout(true);
    }
    if (mode == 12) {
        applyFloatingCards(true);
    }
    if (mode == 7 || mode == 8) {
        applyViewRail(true);
    }

    const bool wrappersExist = (this->findChild<QToolBar*>(QStringLiteral("wrapper_toolBar_meshfile")) != nullptr);
    if (wrappersExist && !m_toolbarRebuilding) {
        rebuildToolbarRow(m_currentToolbarIconSize);
        relayoutToolbarWrappers();
    }

    {
        const QWidgetList tops = QApplication::topLevelWidgets();
        for (QWidget* top : tops) {
            if (!top) continue;
            QList<QWidget*> widgets = top->findChildren<QWidget*>();
            widgets.prepend(top);
            for (QWidget* w : widgets) {
                if (!w) continue;
                if (!w->property("igPanelBaseQss").toString().isEmpty()) {
                    igQtPanelTheme::refresh(w);
                }
            }
            if (top != this) {
                QEvent styleEvent(QEvent::StyleChange);
                QCoreApplication::sendEvent(top, &styleEvent);
            }
        }
    }
}

void igQtMainWindow::applyFloatingCardPalette() {
    if (!isFloatingCardStyle(m_styleMode)) return;
    const FloatingCardPalette cardPal = floatingCardPalette(m_styleMode);
    const QString backdrop = QString::fromLatin1(cardPal.backdrop);

    if (rendererWidget) {
        QPalette rendererPal = rendererWidget->palette();
        rendererPal.setColor(QPalette::Window, QColor(backdrop));
        rendererWidget->setPalette(rendererPal);
        rendererWidget->setStyleSheet(QStringLiteral("background-color: %1;").arg(backdrop));
    }
    if (m_floatingCardWidget) {
        m_floatingCardWidget->setStyleSheet(
                QStringLiteral("QWidget#FloatingCard { background-color: %1; border: 1px solid %2;"
                               " border-radius: 8px; }")
                        .arg(QString::fromLatin1(cardPal.cardBg), QString::fromLatin1(cardPal.cardBorder)));
    }
}

void igQtMainWindow::applyFloatingCards(bool enabled) {
    if (enabled && m_centralCardContainer) {
        applyFloatingCardPalette();
        return;
    }
    if (!enabled && !m_centralCardContainer) return;

    if (enabled) {
        if (!m_centralCardContainer && rendererWidget) {
            auto* container = new QWidget(this);
            container->setObjectName(QStringLiteral("CentralCardContainer"));
            container->setAttribute(Qt::WA_StyledBackground, true);
            auto* lay = new QVBoxLayout(container);
            lay->setContentsMargins(11, 6, 11, 6);
            lay->setSpacing(0);
            lay->addWidget(rendererWidget);
            m_centralCardContainer = container;
            this->setCentralWidget(m_centralCardContainer);
        }
        if (rendererWidget) {
            rendererWidget->setAutoFillBackground(true);
            applyFloatingCardPalette();
            applyRoundedMask(rendererWidget, 8);
        }
        if (modelTreeWidget) {
            QDockWidget* props = modelTreeWidget->getPropertiesDock();
            if (!props) return;
            m_propertiesOriginalMinWidth = props->minimumWidth();
            props->setMinimumWidth(200);
            {
                auto* emptyTitle = new QWidget(props);
                emptyTitle->setFixedHeight(0);
                props->setTitleBarWidget(emptyTitle);
            }
            QWidget* original = props->widget();
            m_floatingCardOriginalWidget = original;
            if (original) {
                props->setAttribute(Qt::WA_TranslucentBackground, true);
                props->setAutoFillBackground(false);
                props->setStyleSheet(
                        "QDockWidget#LayerPropertiesDock { background-color: transparent; border: none; }");
                auto* outer = new QWidget(props);
                outer->setObjectName(QStringLiteral("FloatingCardOuter"));
                outer->setAttribute(Qt::WA_StyledBackground, true);
                outer->setAttribute(Qt::WA_TranslucentBackground, true);
                outer->setAutoFillBackground(false);
                auto* outerLayout = new QVBoxLayout(outer);
                outerLayout->setContentsMargins(11, 6, 11, 6);
                outerLayout->setSpacing(0);
                original->setParent(outer);
                original->setObjectName(QStringLiteral("FloatingCard"));
                original->setContentsMargins(0, 0, 0, 0);
                outerLayout->addWidget(original);
                props->setWidget(outer);
                m_floatingCardWidget = original;
                applyFloatingCardPalette();
                igQtDetachRoundedCorners(original);
            }
            m_floatingCardDock = props;
            QTimer::singleShot(0, this, [this, props]() {
                if (props) this->resizeDocks({props}, {210}, Qt::Horizontal);
            });
            if (QDockWidget* treeDock = modelTreeWidget->getTreeDock()) {
                m_floatingTreeDock = treeDock;
                applyRoundedMask(treeDock, 8);

                QWidget* originalTree = treeDock->widget();
                m_floatingTreeOriginalWidget = originalTree;

                if (originalTree) {
                    auto* treeOuter = new QWidget(treeDock);
                    treeOuter->setObjectName(QStringLiteral("FloatingTreeOuter"));
                    treeOuter->setAttribute(Qt::WA_TranslucentBackground, true);
                    auto* treeLayout = new QVBoxLayout(treeOuter);
                    treeLayout->setContentsMargins(0, 20, 0, 0);
                    treeLayout->setSpacing(0);
                    originalTree->setParent(treeOuter);
                    originalTree->setContentsMargins(0, 0, 0, 0);
                    treeLayout->addWidget(originalTree);
                    treeDock->setWidget(treeOuter);
                    m_floatingTreeWrapper = treeOuter;
                }
            }
        }

    } else {
        if (m_centralCardContainer && rendererWidget) {
            QWidget* old = m_centralCardContainer;
            m_centralCardContainer = nullptr;
            this->setCentralWidget(rendererWidget);
            old->deleteLater();
        }
        if (rendererWidget) {
            rendererWidget->setCornerCover(0, QColor());
            rendererWidget->setMask(QRegion());
        }        if (m_floatingCardWidget) {
            igQtDetachRoundedCorners(m_floatingCardWidget);
            m_floatingCardWidget->setMask(QRegion());
            m_floatingCardWidget = nullptr;
        }
        if (m_floatingCardDock) {
            m_floatingCardDock->setMask(QRegion());
            m_floatingCardDock = nullptr;
        }
        if (m_floatingTreeDock) {
            QDockWidget* treeDock = m_floatingTreeDock;
            igQtDetachRoundedCorners(treeDock);
            if (m_floatingTreeOriginalWidget) {
                QWidget* original = m_floatingTreeOriginalWidget;
                QWidget* wrapper = treeDock->widget();
                original->setContentsMargins(0, 0, 0, 0);
                original->setParent(treeDock);
                treeDock->setWidget(original);
                if (wrapper && wrapper != original) wrapper->deleteLater();
                m_floatingTreeOriginalWidget = nullptr;
                m_floatingTreeWrapper = nullptr;
            }
            treeDock->setMask(QRegion());
            m_floatingTreeDock = nullptr;
        }
        if (modelTreeWidget) {
            QDockWidget* props = modelTreeWidget->getPropertiesDock();
            if (props) {
                const int restoreW = m_propertiesOriginalMinWidth > 0 ? m_propertiesOriginalMinWidth : 220;
                m_propertiesOriginalMinWidth = 0;
                props->setMinimumWidth(restoreW);
                if (m_floatingCardOriginalWidget) {
                    QWidget* original = m_floatingCardOriginalWidget;
                    QWidget* wrapper = props->widget();
                    original->setObjectName(QString());
                    original->setContentsMargins(0, 0, 0, 0);
                    original->setStyleSheet(QString());
                    original->setParent(props);
                    props->setWidget(original);
                    if (wrapper && wrapper != original) wrapper->deleteLater();
                    m_floatingCardOriginalWidget = nullptr;
                } else if (QWidget* w = props->widget()) {
                    w->setObjectName(QString());
                    w->setContentsMargins(0, 0, 0, 0);
                    w->setStyleSheet(QString());
                }
                props->setStyleSheet(QString());
                props->setAttribute(Qt::WA_TranslucentBackground, false);
                props->setAutoFillBackground(true);
                props->setTitleBarWidget(nullptr);
                QTimer::singleShot(0, this, [this, props, restoreW]() {
                    if (props) this->resizeDocks({props}, {restoreW}, Qt::Horizontal);
                });
            }
        }
    }
}

void igQtMainWindow::applyWorkspaceLayout(bool enabled) {
    if (!modelTreeWidget) return;

    QDockWidget* propertiesDock = modelTreeWidget->getPropertiesDock();
    if (!propertiesDock) return;

    const QList<QDockWidget*> analysisDocks = {
            ui->dockWidget_ScalarField,
            ui->dockWidget_VectorField,
            ui->dockWidget_FlowField,
            ui->dockWidget_TensorField,
            ui->dockWidget_ParallelCoordinatesField,
            ui->dockWidget_VariableCorrelationField,
            ui->dockWidget_VariableDensityField,
            ui->dockWidget_DataChangeField,
            ui->dockWidget_SelectionField,
            ui->dockWidget_ContextPreservingShowField,
            ui->dockWidget_QualityDetection,
            ui->dockWidget_EditMode,
            ui->dockWidget_ModelList,
            ui->dockWidget_ContourExtract
    };

    if (enabled) {
        for (QDockWidget* d : analysisDocks) {
            if (d) this->addDockWidget(Qt::RightDockWidgetArea, d);
        }
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ParallelCoordinatesField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_VariableCorrelationField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_VariableDensityField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_DataChangeField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ContextPreservingShowField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_QualityDetection);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_EditMode);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ModelList);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ContourExtract);

        this->addDockWidget(Qt::LeftDockWidgetArea, propertiesDock);
        if (m_leftFieldDock) this->addDockWidget(Qt::LeftDockWidgetArea, m_leftFieldDock);
        this->splitDockWidget(m_leftFieldDock, propertiesDock, Qt::Vertical);

        QTimer::singleShot(0, this, [this, propertiesDock]() {
            if (ui->dockWidget_SelectionField) {
                this->resizeDocks({ui->dockWidget_SelectionField}, {300}, Qt::Horizontal);
            }
            if (propertiesDock) {
                this->resizeDocks({propertiesDock}, {260}, Qt::Horizontal);
            }
        });

        modelTreeWidget->positionTreeDockToRendererCorner(rendererWidget);
    } else {
        for (QDockWidget* d : analysisDocks) {
            if (d) this->addDockWidget(Qt::LeftDockWidgetArea, d);
        }
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ParallelCoordinatesField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_VariableCorrelationField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_VariableDensityField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_DataChangeField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ContextPreservingShowField);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_QualityDetection);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_EditMode);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ModelList);
        this->tabifyDockWidget(ui->dockWidget_SelectionField, ui->dockWidget_ContourExtract);

        this->addDockWidget(Qt::LeftDockWidgetArea, m_leftFieldDock);
        this->addDockWidget(Qt::LeftDockWidgetArea, propertiesDock);
        this->splitDockWidget(m_leftFieldDock, propertiesDock, Qt::Vertical);

        if (rendererWidget) {
            modelTreeWidget->positionTreeDockToRendererCorner(rendererWidget);
        }
    }
}

void igQtMainWindow::applyViewRail(bool enabled) {
    if (!rendererWidget) return;

    if (enabled && !m_viewDock) {
        m_viewDock = new QDockWidget(QStringLiteral("视图"), this);
        m_viewDock->setObjectName(QStringLiteral("ViewDock"));
        m_viewDock->setAllowedAreas(Qt::RightDockWidgetArea);
        m_viewDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
        m_viewDock->setMinimumWidth(40);
        m_viewDock->setMaximumWidth(42);

        QWidget* railWidget = new QWidget(m_viewDock);
        QVBoxLayout* railLayout = new QVBoxLayout(railWidget);
        railLayout->setContentsMargins(2, 2, 2, 2);
        railLayout->setSpacing(3);

        auto addRailButton = [&](QAction* act, const QString& tip) {
            if (!act) return;
            QToolButton* b = new QToolButton(railWidget);
            b->setDefaultAction(act);
            b->setIconSize(QSize(18, 18));
            b->setToolButtonStyle(Qt::ToolButtonIconOnly);
            b->setAutoRaise(true);
            b->setFocusPolicy(Qt::NoFocus);
            b->setFixedSize(26, 26);
            b->setToolTip(tip);
            b->setCursor(Qt::PointingHandCursor);
            b->setStyleSheet(
                    "QToolButton { border-radius: 0; }"
                    "QToolButton:hover { background-color: rgba(255,255,255,0.10); }");
            railLayout->addWidget(b, 0, Qt::AlignHCenter);
        };

        addRailButton(ui->action_ShowOrientationAxes, QStringLiteral("定向轴"));
        addRailButton(ui->action_ResetCameraView, QStringLiteral("重置视角"));
        addRailButton(ui->action_UseOrthographic, QStringLiteral("正交投影"));
        addRailButton(ui->action_setViewToIsometric, QStringLiteral("等轴测视图"));
        addRailButton(ui->action_setViewToPositiveX, QStringLiteral("+X 方向"));
        addRailButton(ui->action_setViewToNegativeX, QStringLiteral("-X 方向"));
        addRailButton(ui->action_setViewToPositiveY, QStringLiteral("+Y 方向"));
        addRailButton(ui->action_setViewToNegativeY, QStringLiteral("-Y 方向"));
        addRailButton(ui->action_setViewToPositiveZ, QStringLiteral("+Z 方向"));
        addRailButton(ui->action_setViewToNegativeZ, QStringLiteral("-Z 方向"));
        addRailButton(ui->action_rotateNinetyCounterClockwise, QStringLiteral("逆时针 90°"));
        addRailButton(ui->action_rotateNinetyClockwise, QStringLiteral("顺时针 90°"));
        addRailButton(ui->action_ShowCenter, QStringLiteral("显示中心"));
        addRailButton(ui->action_PickCenter, QStringLiteral("选择中心"));

        m_viewDock->setWidget(railWidget);
        this->addDockWidget(Qt::RightDockWidgetArea, m_viewDock);
    }
    if (m_viewDock) {
        m_viewDock->setVisible(enabled);
        if (enabled) updateViewRailPosition();
    }
}

void igQtMainWindow::updateViewRailPosition() {
    if (m_viewDock && m_viewDock->isVisible()) {
        QTimer::singleShot(0, this, [this]() {
            if (m_viewDock) this->resizeDocks({m_viewDock}, {40}, Qt::Horizontal);
        });
    }
}

QString igQtMainWindow::toolbarButtonQss(int fontPx) const {
    const int fam = styleColorFamily(m_styleMode);
    if (fam == 1) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 8px; margin: 0; padding: 2px; font-size: %1px; color: #B8C4D4; }"
                "QToolButton:hover { background-color: rgba(56, 189, 248, 0.10); border-color: rgba(56, 189, 248, 0.30); color: #E8EEF7; }"
                "QToolButton:pressed { background-color: rgba(56, 189, 248, 0.18); }"
                "QToolButton:checked { background-color: rgba(56, 189, 248, 0.16); border-color: rgba(56, 189, 248, 0.45); color: #38BDF8; }")
                .arg(fontPx);
    }
    if (fam == 2) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 8px; margin: 0; padding: 2px; font-size: %1px; color: #33405B; }"
                "QToolButton:hover { background-color: rgba(37, 99, 235, 0.08); border-color: rgba(37, 99, 235, 0.30); color: #1F2A3A; }"
                "QToolButton:pressed { background-color: rgba(37, 99, 235, 0.16); }"
                "QToolButton:checked { background-color: rgba(37, 99, 235, 0.14); border-color: rgba(37, 99, 235, 0.45); color: #2563EB; }")
                .arg(fontPx);
    }
    if (fam == 4) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 6px; margin: 0; padding: 1px; font-size: %1px; color: #B8C0CA; }"
                "QToolButton:hover { background-color: rgba(77, 208, 225, 0.14); border-color: rgba(77, 208, 225, 0.36); color: #FFFFFF; }"
                "QToolButton:pressed { background-color: rgba(77, 208, 225, 0.26); }"
                "QToolButton:checked { background-color: rgba(77, 208, 225, 0.20); border-color: rgba(77, 208, 225, 0.52); color: #5CE1F0; }")
                .arg(fontPx);
    }
    if (fam == 6 || fam == 8) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 6px; margin: 0; padding: 1px; font-size: %1px; color: #B8C0CA; }"
                "QToolButton:hover { background-color: rgba(77, 208, 225, 0.14); border-color: rgba(77, 208, 225, 0.36); color: #FFFFFF; }"
                "QToolButton:pressed { background-color: rgba(77, 208, 225, 0.26); }"
                "QToolButton:checked { background-color: rgba(77, 208, 225, 0.20); border-color: rgba(77, 208, 225, 0.52); color: #5CE1F0; }")
                .arg(fontPx);
    }
    if (fam == 9) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 5px; margin: 0; padding: 5px; font-size: %1px; color: #C3CBD5; }"
                "QToolButton:hover { background-color: rgba(108, 142, 174, 0.10); border-color: rgba(108, 142, 174, 0.24); color: #E9EDF2; }"
                "QToolButton:pressed { background-color: rgba(108, 142, 174, 0.18); }"
                "QToolButton:checked { background-color: rgba(108, 142, 174, 0.16); border-color: rgba(108, 142, 174, 0.40); color: #9FB6C9; }")
                .arg(fontPx);
    }
    if (fam == 10) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 4px; margin: 0; padding: 5px; font-size: %1px; color: #A2A8B0; }"
                "QToolButton:hover { background-color: #242830; border-color: #2A303A; color: #E2E4E8; }"
                "QToolButton:pressed { background-color: #2A303A; }"
                "QToolButton:checked { background-color: #2A303A; border-color: #3A414C; color: #E2E4E8; }")
                .arg(fontPx);
    }
    if (fam == 11 || fam == 12) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 4px; margin: 0; padding: 5px; font-size: %1px; color: #858585; }"
                "QToolButton:hover { background-color: #2A2A2C; border-color: #37373D; color: #CCCCCC; }"
                "QToolButton:pressed { background-color: #404045; }"
                "QToolButton:checked { background-color: #37373D; border-color: #404045; color: #CCCCCC; }")
                .arg(fontPx);
    }
    return QStringLiteral(
            "QToolButton { border: none; margin: 0; padding: 1px; font-size: %1px; }"
            "QToolButton:hover { background-color: #3A3A3A; border-radius: 2px; }"
            "QToolButton:pressed { background-color: #4A4A4A; }")
            .arg(fontPx);
}

QString igQtMainWindow::twoRowGridButtonQss() const {
    const int fam = styleColorFamily(m_styleMode);
    if (fam == 1) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 6px; margin: 0; padding: 0; }"
                "QToolButton:hover { background-color: rgba(56, 189, 248, 0.10); border-color: rgba(56, 189, 248, 0.30); }"
                "QToolButton:pressed { background-color: rgba(56, 189, 248, 0.18); }"
                "QToolButton:checked { background-color: rgba(56, 189, 248, 0.16); border-color: rgba(56, 189, 248, 0.45); }");
    }
    if (fam == 2) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 6px; margin: 0; padding: 0; }"
                "QToolButton:hover { background-color: rgba(37, 99, 235, 0.08); border-color: rgba(37, 99, 235, 0.30); }"
                "QToolButton:pressed { background-color: rgba(37, 99, 235, 0.16); }"
                "QToolButton:checked { background-color: rgba(37, 99, 235, 0.14); border-color: rgba(37, 99, 235, 0.45); }");
    }
    if (fam == 4) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 6px; margin: 0; padding: 0; }"
                "QToolButton:hover { background-color: rgba(77, 208, 225, 0.14); border-color: rgba(77, 208, 225, 0.36); }"
                "QToolButton:pressed { background-color: rgba(77, 208, 225, 0.26); }"
                "QToolButton:checked { background-color: rgba(77, 208, 225, 0.20); border-color: rgba(77, 208, 225, 0.52); }");
    }
    if (fam == 6 || fam == 8) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 6px; margin: 0; padding: 0; }"
                "QToolButton:hover { background-color: rgba(77, 208, 225, 0.14); border-color: rgba(77, 208, 225, 0.36); }"
                "QToolButton:pressed { background-color: rgba(77, 208, 225, 0.26); }"
                "QToolButton:checked { background-color: rgba(77, 208, 225, 0.20); border-color: rgba(77, 208, 225, 0.52); }");
    }
    if (fam == 9) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 5px; margin: 0; padding: 3px; }"
                "QToolButton:hover { background-color: rgba(108, 142, 174, 0.10); border-color: rgba(108, 142, 174, 0.24); }"
                "QToolButton:pressed { background-color: rgba(108, 142, 174, 0.18); }"
                "QToolButton:checked { background-color: rgba(108, 142, 174, 0.16); border-color: rgba(108, 142, 174, 0.40); }");
    }
    if (fam == 10) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 4px; margin: 0; padding: 3px; }"
                "QToolButton:hover { background-color: #242830; border-color: #2A303A; }"
                "QToolButton:pressed { background-color: #2A303A; }"
                "QToolButton:checked { background-color: #2A303A; border-color: #3A414C; }");
    }
    if (fam == 11 || fam == 12) {
        return QStringLiteral(
                "QToolButton { border: 1px solid transparent; border-radius: 4px; margin: 0; padding: 3px; }"
                "QToolButton:hover { background-color: #2A2A2C; border-color: #37373D; }"
                "QToolButton:pressed { background-color: #404045; }"
                "QToolButton:checked { background-color: #37373D; border-color: #404045; }");
    }
    return QStringLiteral(
            "QToolButton { border: none; margin: 0; padding: 0; }"
            "QToolButton:hover { background-color: #3A3A3A; border-radius: 2px; }"
            "QToolButton:pressed { background-color: #4A4A4A; }");
}

QString igQtMainWindow::toolbarItemQss() const {
    const int fam = styleColorFamily(m_styleMode);
    QString hover;
    switch (fam) {
        case 1:  hover = QStringLiteral("rgba(56, 189, 248, 0.10)"); break;
        case 2:  hover = QStringLiteral("rgba(37, 99, 235, 0.08)"); break;
        case 4:  hover = QStringLiteral("rgba(77, 208, 225, 0.14)"); break;
        case 6:  hover = QStringLiteral("rgba(77, 208, 225, 0.14)"); break;
        case 8:  hover = QStringLiteral("rgba(77, 208, 225, 0.14)"); break;
        case 9:  hover = QStringLiteral("rgba(108, 142, 174, 0.10)"); break;
        case 10: hover = QStringLiteral("#242830"); break;
        case 11: hover = QStringLiteral("#2A2A2C"); break;
        case 12: hover = QStringLiteral("#2A2A2C"); break;
        default: hover = QStringLiteral("#3A3A3A"); break;
    }
    return QStringLiteral(
            "QWidget#toolbarButtonItem { background: transparent; border: none; border-radius: 4px; }"
            "QWidget#toolbarButtonItem:hover { background-color: %1; }")
            .arg(hover);
}

QString igQtMainWindow::toolbarTitleLabelQss() const {
    const int fam = styleColorFamily(m_styleMode);
    if (fam == 1) {
        return QStringLiteral(
                "QLabel { color: #8A99AC; padding: 3px 12px; background-color: rgba(255, 255, 255, 0.03); "
                "border: 1px solid rgba(148, 163, 184, 0.18); border-radius: 6px; font-family: 'PingFang SC'; }");
    }
    if (fam == 2) {
        return QStringLiteral(
                "QLabel { color: #5A6577; padding: 3px 12px; background-color: rgba(0, 0, 0, 0.03); "
                "border: 1px solid #D9DEE7; border-radius: 6px; font-family: 'PingFang SC'; }");
    }
    if (fam == 4) {
        return QStringLiteral(
                "QLabel { color: #5E6670; padding: 2px 8px; background-color: rgba(77, 208, 225, 0.06); "
                "border: 1px solid rgba(77, 208, 225, 0.24); border-radius: 6px; font-family: 'PingFang SC'; }");
    }
    if (fam == 6 || fam == 8) {
        return QStringLiteral(
                "QLabel { color: #5E6670; padding: 2px 8px; background-color: rgba(77, 208, 225, 0.06); "
                "border: 1px solid rgba(77, 208, 225, 0.24); border-radius: 6px; font-family: 'PingFang SC'; }");
    }
    if (fam == 9) {
        return QStringLiteral(
                "QLabel { color: #8B96A3; padding: 2px 4px; background-color: transparent; "
                "border: none; border-radius: 0; font-family: 'PingFang SC'; }");
    }
    if (fam == 10) {
        return QStringLiteral(
                "QLabel { color: #7E858E; padding: 2px 4px; background-color: transparent; "
                "border: none; border-radius: 0; font-family: 'PingFang SC'; }");
    }
    if (fam == 11 || fam == 12) {
        return QStringLiteral(
                "QLabel { color: #858585; padding: 2px 4px; background-color: transparent; "
                "border: none; border-radius: 0; font-family: 'PingFang SC'; }");
    }
    return QStringLiteral(
            "QLabel { color: #9A9A9A; padding: 3px 12px; background-color: rgba(255, 255, 255, 0.03); "
            "border: 1px solid #3C3C3C; border-radius: 6px; font-family: 'PingFang SC'; }");
}

QString igQtMainWindow::toolbarCaptionLabelQss(int fontPx) const {
    const int fam = styleColorFamily(m_styleMode);
    if (fam == 1) {
        return QStringLiteral(
                "QLabel { color: #B8C4D4; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    if (fam == 2) {
        return QStringLiteral(
                "QLabel { color: #4A5568; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    if (fam == 4) {
        return QStringLiteral(
                "QLabel { color: #B8C0CA; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    if (fam == 6 || fam == 8) {
        return QStringLiteral(
                "QLabel { color: #B8C0CA; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    if (fam == 9) {
        return QStringLiteral(
                "QLabel { color: #C7D0DA; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    if (fam == 10) {
        return QStringLiteral(
                "QLabel { color: #9AA1AA; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    if (fam == 11 || fam == 12) {
        return QStringLiteral(
                "QLabel { color: #858585; padding: 0; background-color: transparent; border: none; "
                "font-family: 'PingFang SC'; font-size: %1px; }")
                .arg(fontPx);
    }
    return QStringLiteral(
            "QLabel { color: #D2D2D2; padding: 0; background-color: transparent; border: none; "
            "font-family: 'PingFang SC'; font-size: %1px; }")
            .arg(fontPx);
}

QString igQtMainWindow::toolbarSeamColor() const {
    const int fam = styleColorFamily(m_styleMode);
    switch (fam) {
        case 0:  return QStringLiteral("#3C3C3C");
        case 1:  return QStringLiteral("#2E3D52");
        case 2:  return QStringLiteral("#CBD2DC");
        case 4:  return QStringLiteral("#343B43");
        case 5:  return QStringLiteral("#3A414D");
        case 6:  return QStringLiteral("#343B43");
        case 8:  return QStringLiteral("#343B43");
        case 9:  return QStringLiteral("#2B2F36");
        case 10: return QStringLiteral("#2C3038");
        case 11: return QStringLiteral("#2D2D30");
        case 12: return QStringLiteral("#2D2D30");
        default: return QStringLiteral("#3E4550");
    }
}

QString igQtMainWindow::toolbarAccentColor() const {
    const int fam = styleColorFamily(m_styleMode);
    switch (fam) {
        case 0:  return QStringLiteral("#007ACC");
        case 1:  return QStringLiteral("#38BDF8");
        case 2:  return QStringLiteral("#2563EB");
        case 3:  return QStringLiteral("#4A4E54");
        case 7:  return QStringLiteral("#4A4E54");
        case 5:  return QStringLiteral("#60CDFF");
        case 9:  return QStringLiteral("#3F5568");
        case 10: return QStringLiteral("#2A303A");
        case 11: return QStringLiteral("#37373D");
        case 12: return QStringLiteral("#37373D");
        default: return QStringLiteral("#4DD0E1");
    }
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
    int gridSpacing = qMax(2, iconSize.height() / 12);
    if (isModernDenseStyle(m_styleMode)) gridSpacing += 2;
    grid->setSpacing(gridSpacing);
    grid->setContentsMargins(0, 0, 0, 0);

    // 两行视图按钮：小图标时按钮也相应做小，避免网格占用过多宽度
    const int targetIcon = qMax(12, qMin(16, static_cast<int>(iconSize.height() * 0.45)));
    const int rowHeight = targetIcon + 6 + (isModernDenseStyle(m_styleMode) ? 2 : 0);
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
        btn->setStyleSheet(twoRowGridButtonQss());
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

}

void igQtMainWindow::addToolbarTitle(QToolBar* toolbar, const QString& title, int iconSizePx) {
    if (!toolbar)
        return;

    Qt::ToolBarArea area = this->toolBarArea(toolbar);
    if (area == Qt::NoToolBarArea)
        area = Qt::TopToolBarArea; // 重建时原 toolbar 已不在 QMainWindow 管理下，统一回到顶部区域
    const QSize iconSize(iconSizePx, iconSizePx);
    ToolbarSpacingMetrics spacing = metricsForIconSize(iconSize.width());
    if (isModernDenseStyle(m_styleMode)) {
        spacing.btnGap += 2;
        spacing.edgeMargin += 2;
        spacing.buttonPadding += 3;
        spacing.bottomMargin += 2;
    }
    const QList<QAction*> actions = toolbar->actions();
    const int fontPx = toolbarButtonFontPixel(iconSizePx);

    QFont titleFont(QStringLiteral("PingFang SC"));
    titleFont.setPointSize(titlePointSizeForIcon(iconSizePx));
    const bool compact = (m_styleMode == 6);
    const int titleTextH = compact ? 0 : QFontMetrics(titleFont).height();

    QWidget* container = new QWidget(this);
    container->setObjectName("toolbarContainer_" + toolbar->objectName());
    container->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    container->setAttribute(Qt::WA_StyledBackground, true);

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
                hLayout->addWidget(w, 0, Qt::AlignLeft | Qt::AlignVCenter);
                continue;
            }
        }

        if (compact) {
            QToolButton* b = new QToolButton(topRow);
            b->setDefaultAction(act);
            b->setIconSize(iconSize);
            b->setToolButtonStyle(Qt::ToolButtonIconOnly);
            b->setAutoRaise(true);
            b->setFocusPolicy(Qt::NoFocus);
            b->setFixedSize(iconSize.width(), iconSize.height());
            b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            b->setToolTip(act->text());
            b->setStyleSheet(toolbarButtonQss(fontPx));
            hLayout->addWidget(b, 0, Qt::AlignLeft | Qt::AlignVCenter);
            continue;
        }

        QFont btnFont(QStringLiteral("PingFang SC"));
        btnFont.setPixelSize(fontPx);
        const QFontMetrics fm(btnFont);
        const int textMaxW = qMax(kToolbarButtonTextMinWidth, iconSize.width() + 16);
        const QString rawText = act->text();
        const QString wrappedText = wrapToolbarButtonText(rawText, textMaxW, fm);
        const int textLines = wrappedText.isEmpty() ? 1 : wrappedText.count(QLatin1Char('\n')) + 1;
        // 按钮宽度按「换行后最宽的一行」计算，避免换行后按钮仍然过宽
        int widestTextW = fm.horizontalAdvance(rawText);
        if (wrappedText.contains(QLatin1Char('\n'))) {
            widestTextW = 0;
            const QStringList textLinesList = wrappedText.split(QLatin1Char('\n'));
            for (const QString& line : textLinesList) {
                widestTextW = qMax(widestTextW, fm.horizontalAdvance(line));
            }
        }
        const int btnW = qMax(iconSize.width(), widestTextW) + 2 * spacing.buttonPadding;
        const int textLineH = qMax(fm.height(), fm.lineSpacing());
        const int captionH = textLines * textLineH + 2;

        QWidget* item = new QWidget(topRow);
        QVBoxLayout* itemLayout = new QVBoxLayout(item);
        itemLayout->setContentsMargins(spacing.buttonPadding, 0, spacing.buttonPadding, 0);
        itemLayout->setSpacing(2);
        itemLayout->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        QToolButton* b = new QToolButton(item);
        b->setDefaultAction(act);
        b->setIconSize(iconSize);
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setAutoRaise(true);
        b->setFocusPolicy(Qt::NoFocus);
        b->setFixedSize(iconSize.width(), iconSize.height());
        b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        b->setStyleSheet(twoRowGridButtonQss());
        itemLayout->addWidget(b, 0, Qt::AlignHCenter);

        QLabel* caption = new QLabel(wrappedText, item);
        caption->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        caption->setFixedSize(qMax(iconSize.width(), widestTextW), captionH);
        caption->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        caption->setStyleSheet(toolbarCaptionLabelQss(fontPx));
        itemLayout->addWidget(caption, 0, Qt::AlignHCenter);

        item->setFixedSize(btnW, iconSize.height() + captionH + 2);
        item->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        item->setObjectName(QStringLiteral("toolbarButtonItem"));
        item->setAttribute(Qt::WA_StyledBackground, true);
        item->setStyleSheet(toolbarItemQss());
        item->setCursor(Qt::PointingHandCursor);
        item->setToolTip(act->text());
        item->setProperty("igToolbarButton", QVariant::fromValue<QObject*>(b));
        b->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        caption->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        item->installEventFilter(this);

        hLayout->addWidget(item, 0, Qt::AlignLeft | Qt::AlignVCenter);
    }

    // 容器高度：按钮换行后变高，用 topRow 的实际 sizeHint 兜底
    const int totalH = qMax(iconSize.height() + spacing.verticalGap + titleTextH + spacing.bottomMargin,
                            topRow->sizeHint().height() + spacing.verticalGap + titleTextH + spacing.bottomMargin);
    container->setMinimumSize(100, totalH);

    this->removeToolBar(toolbar);
    toolbar->hide();

    QVBoxLayout* vLayout = new QVBoxLayout(container);
    const int frameInset = 4;
    vLayout->setContentsMargins(spacing.edgeMargin + frameInset, frameInset,
                                spacing.edgeMargin + frameInset, frameInset + 2);
    vLayout->setSpacing(2);
    vLayout->setSizeConstraint(QLayout::SetFixedSize);

    QLabel* titleLabel = nullptr;
    if (!compact) {
        titleLabel = new QLabel(title, container);
        titleLabel->setObjectName("toolbarTitle_" + toolbar->objectName());
        titleLabel->setFont(titleFont);
        titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        titleLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        titleLabel->setStyleSheet(toolbarTitleLabelQss());
        vLayout->addWidget(titleLabel, 0, Qt::AlignLeft);
    }

    vLayout->addStretch(1);
    vLayout->addWidget(topRow, 0, Qt::AlignHCenter);
    vLayout->addStretch(1);

    QFrame* bottomLine = new QFrame(container);
    bottomLine->setObjectName("toolbarAccentLine_" + toolbar->objectName());
    bottomLine->setFixedHeight(2);
    bottomLine->setFrameShape(QFrame::NoFrame);
    bottomLine->setAttribute(Qt::WA_StyledBackground, true);
    bottomLine->setStyleSheet(
            QStringLiteral("QFrame { background-color: %1; }").arg(toolbarAccentColor()));
    vLayout->addWidget(bottomLine, 0);

    QToolBar* wrapper = new QToolBar(this);
    wrapper->setObjectName("wrapper_" + toolbar->objectName());
    wrapper->setWindowTitle(title);
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
    if (m_toolbarRebuilding) return;

    const QStringList orderedNames = {
            "wrapper_toolBar_meshfile",
            "wrapper_toolBar_3",
            "wrapper_toolBar_2",
            "wrapper_toolBar_4"
    };

    auto collectWrappers = [&]() -> QList<QToolBar*> {
        QList<QToolBar*> out;
        for (const QString& name : orderedNames) {
            if (QToolBar* tb = this->findChild<QToolBar*>(name)) {
                out.push_back(tb);
            }
        }
        return out;
    };

    QList<QToolBar*> wrappers = collectWrappers();
    if (wrappers.isEmpty()) return;

    const int availableWidth = qMax(320, this->width() - 12);

    // 从当前已拟合的尺寸继续缩/放，避免每次 resize 都先重置回首选再重新放大（导致反复重建）
    int iconSize = m_currentToolbarIconSize;

    // 2) 宽度拟合：整行放不下时逐档缩小图标并重建，直到放得下或到图标下限
    while (iconSize > kToolbarIconMin && measureToolbarRowWidth() > availableWidth) {
        iconSize = qMax(kToolbarIconMin, iconSize - 4);
        rebuildToolbarRow(iconSize);
        wrappers = collectWrappers();
    }

    // 3) 空间充足时按填充比例放大图标/字号，避免宽屏下右侧太空
    const int targetWidth = qMax(320, static_cast<int>(availableWidth * kToolbarFillRatio));
    while (iconSize < kToolbarIconMax && measureToolbarRowWidth() < targetWidth) {
        iconSize = qMin(kToolbarIconMax, iconSize + 2);
        rebuildToolbarRow(iconSize);
        wrappers = collectWrappers();
    }

    // 每次重排前先清掉舊分行點，避免重複斷行導致排版漂移
    for (const QString& name : orderedNames) {
        if (QToolBar* tb = this->findChild<QToolBar*>(name)) {
            this->removeToolBarBreak(tb);
        }
    }

    const ToolbarSpacingMetrics spacing = metricsForIconSize(m_currentToolbarIconSize);
    const int gap = isModernDenseStyle(m_styleMode)
            ? 2
            : spacing.groupGap;
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
        const bool firstInRow = (usedWidth == 0);
        const QString seam = toolbarSeamColor();
        if (isModernDenseStyle(m_styleMode)) {
            tb->setStyleSheet(QStringLiteral("#%1 { margin-top: %2px; border: none; border-top: 1px solid %3; %4 }")
                                      .arg(tb->objectName())
                                      .arg(startsNewRow ? rowGap : 0)
                                      .arg(seam)
                                      .arg(firstInRow
                                               ? QStringLiteral("")
                                               : QStringLiteral("border-left: 1px solid %1;").arg(seam)));
        } else {
            tb->setStyleSheet(QStringLiteral("#%1 { margin-top: %2px; border: none; border-top: 1px solid %3; }")
                                      .arg(tb->objectName())
                                      .arg(startsNewRow ? rowGap : 0)
                                      .arg(seam));
        }
        usedWidth += needWidth + gap;
    }
}


void igQtMainWindow::rebuildToolbarRow(int iconSize) {
    if (m_toolbarRebuilding) return;
    m_toolbarRebuilding = true;

    const int originalIconSize = iconSize;
    if (m_styleMode == 7 || m_styleMode == 8) {
        iconSize = qMin(iconSize, 28);
    }

    // 1. 先摘掉 toolBar_4 上残留的两行网格 QWidgetAction（此刻旧容器还活着，可安全比对）
    const QList<QAction*> t4Actions = ui->toolBar_4->actions();
    for (QAction* a : t4Actions) {
        if (auto* wa = qobject_cast<QWidgetAction*>(a)) {
            if (wa->defaultWidget() && wa->defaultWidget()->objectName() == QStringLiteral("twoRowViewGrid")) {
                ui->toolBar_4->removeAction(wa);
                wa->deleteLater();
                break;
            }
        }
    }

    // 2. 删除旧 wrapper 工具栏（连带旧容器与旧按钮）
    removeToolbarWrappers();

    // 3. 把新的图标尺寸应用到原始 QToolBar（两行网格与标题容器都按它重建）
    for (QToolBar* tb : this->findChildren<QToolBar*>()) {
        if (tb->objectName().startsWith("wrapper_"))
            continue;
        tb->setIconSize(QSize(iconSize, iconSize));
        tb->setMinimumHeight(iconSize + qMax(6, iconSize / 6));
    }

    // 4. 重建 toolBar_4 的 3×2 轴方向网格
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

    addToolbarTitle(ui->toolBar_meshfile, QStringLiteral("文件与输出"), iconSize);
    addToolbarTitle(ui->toolBar_3, QStringLiteral("可视化"), iconSize);
    addToolbarTitle(ui->toolBar_2, QStringLiteral("选择与编辑"), iconSize);
    if (m_styleMode != 7 && m_styleMode != 8) {
        addToolbarTitle(ui->toolBar_4, QStringLiteral("视图设置"), iconSize);
    } else {
        this->removeToolBar(ui->toolBar_4);
        ui->toolBar_4->hide();
    }

    {
        const QStringList names = {QStringLiteral("toolBar_meshfile"), QStringLiteral("toolBar_3"),
                                   QStringLiteral("toolBar_2"), QStringLiteral("toolBar_4")};
        int maxH = 0;
        QList<QWidget*> containers;
        for (const QString& n : names) {
            if (QWidget* c = this->findChild<QWidget*>(QStringLiteral("toolbarContainer_") + n)) {
                containers.push_back(c);
                maxH = qMax(maxH, c->sizeHint().height());
            }
        }
        // 容器内部 vLayout 带 SetFixedSize 约束，会按自身 sizeHint 反压高度，
        // 必须先解除该约束，统一 setFixedHeight 才能让四组底部分隔线对齐。
        for (QWidget* c : containers) {
            if (maxH > 0) {
                if (QLayout* lay = c->layout()) { lay->setSizeConstraint(QLayout::SetDefaultConstraint); }
                c->setFixedHeight(maxH);
            }
        }
    }

    m_currentToolbarIconSize = originalIconSize;
    m_toolbarRebuilding = false;
}

void igQtMainWindow::removeToolbarWrappers() {
    const QStringList orderedNames = {
            "wrapper_toolBar_meshfile",
            "wrapper_toolBar_3",
            "wrapper_toolBar_2",
            "wrapper_toolBar_4"
    };
    for (const QString& name : orderedNames) {
        if (QToolBar* tb = this->findChild<QToolBar*>(name)) {
            this->removeToolBar(tb);
            delete tb;
        }
    }
}

int igQtMainWindow::measureToolbarRowWidth() const {
    const QStringList orderedNames = {
            "wrapper_toolBar_meshfile",
            "wrapper_toolBar_3",
            "wrapper_toolBar_2",
            "wrapper_toolBar_4"
    };
    const ToolbarSpacingMetrics spacing = metricsForIconSize(m_currentToolbarIconSize);
    const int gap = isModernDenseStyle(m_styleMode)
            ? 2
            : spacing.groupGap;
    int used = 0;
    int count = 0;
    for (const QString& name : orderedNames) {
        if (const QToolBar* tb = this->findChild<QToolBar*>(name)) {
            used += qMax(tb->minimumWidth(), tb->sizeHint().width());
            if (count > 0) used += gap;
            ++count;
        }
    }
    return used;
}

QString igQtMainWindow::wrapToolbarButtonText(const QString& text, int maxWidth, const QFontMetrics& fm) const {
    if (text.isEmpty() || maxWidth <= 0) return text;
    const int n = text.size();
    if (n <= 1 || fm.horizontalAdvance(text) <= maxWidth) return text;

    const auto lineWidth = [&](int from, int count) {
        return fm.horizontalAdvance(text.mid(from, count));
    };

    int bestScore = std::numeric_limits<int>::max();
    int bestImbalance = std::numeric_limits<int>::max();
    int bestB1 = -1;
    int bestB2 = -1;

    for (int b1 = 1; b1 < n; ++b1) {
        const int w1 = lineWidth(0, b1);
        const int w2 = lineWidth(b1, n - b1);
        if (w1 > maxWidth || w2 > maxWidth) continue;
        const int score = qMax(w1, w2);
        const int imbalance = qAbs(w1 - w2);
        if (score < bestScore || (score == bestScore && imbalance < bestImbalance)) {
            bestScore = score;
            bestImbalance = imbalance;
            bestB1 = b1;
            bestB2 = n;
        }
    }

    if (n >= 3) {
        for (int b1 = 1; b1 < n - 1; ++b1) {
            const int w1 = lineWidth(0, b1);
            if (w1 > maxWidth) continue;
            for (int b2 = b1 + 1; b2 < n; ++b2) {
                const int w2 = lineWidth(b1, b2 - b1);
                const int w3 = lineWidth(b2, n - b2);
                if (w2 > maxWidth || w3 > maxWidth) continue;
                const int score = qMax(w1, qMax(w2, w3));
                const int imbalance = qMax(w1, qMax(w2, w3)) - qMin(w1, qMin(w2, w3));
                if (score < bestScore || (score == bestScore && imbalance < bestImbalance)) {
                    bestScore = score;
                    bestImbalance = imbalance;
                    bestB1 = b1;
                    bestB2 = b2;
                }
            }
        }
    }

    if (bestB1 > 0) {
        if (bestB2 >= n) {
            return text.left(bestB1) + QLatin1Char('\n') + text.mid(bestB1);
        }
        return text.left(bestB1) + QLatin1Char('\n') + text.mid(bestB1, bestB2 - bestB1) + QLatin1Char('\n') + text.mid(bestB2);
    }

    QStringList lines;
    int pos = 0;
    while (pos < n) {
        int end = pos + 1;
        while (end < n && lineWidth(pos, end - pos + 1) <= maxWidth) ++end;
        lines.append(text.mid(pos, end - pos));
        pos = end;
        if (lines.size() >= kToolbarButtonTextMaxLines && pos < n) {
            break;
        }
    }
    if (pos < n) lines.append(text.mid(pos));
    return lines.join(QStringLiteral("\n"));
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

void igQtMainWindow::applyResponsiveToolbarLayout() {
    // 重建式拟合：由 relayoutToolbarWrappers 统一决定图标尺寸（缩小/放大/换行）
    relayoutToolbarWrappers();
}

void igQtMainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    // 窗口首次真正显示后，屏幕 handle 才稳定；此时挂 hook + 跑一次布局，
    // 修正启动阶段按"主屏宽度"猜的档位。
    hookResponsiveEvents();
    applyResponsiveToolbarLayout();
    updateViewRailPosition();
}
