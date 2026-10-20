#include <IQWidgets/igQtSearchInfoWidget.h>
#include <IQWidgets/igQtTransformBoxController.h>
#include <AppendArcLength/iGameAppendArcLengthFilter.h>
#include <MeshMetrics/iGameCellMeshMetricsFilter.h>
#include <Connectivity/iGameConnectivityFilter.h>
#include <DataProcessing/iGameDecimatePolylineFilter.h>
#include <IntegrateVariables/iGameIntegrateVariablesFilter.h>
#include <ModelSurface/iGameMultiBlockGeometryFilter.h>
#include <Modeling/iGameRibbonFilter.h>
#include <WarpByVector/iGameWarpByVectorFilter.h>
#include <iGameSceneManager.h>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <limits>
#include <IQWidgets/igQtSmoothWidget.h>
#include <IQWidgets/igQtTensorPrincipalInvariantsWidget.h>
#include <IQWidgets/igQtMedianFilterWidget.h>
#include <IQWidgets/igQtTextureMapToCylinderWidget.h>
#include <IQWidgets/igQtResampleWithDataSetWidget.h>
#include <IQWidgets/igQtRenameArraysWidget.h>
#include "IQCore/igQtMainWindow.h"
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQWidgets/igQtAxisAlignedReflectionWidget.h>
#include <IQWidgets/igQtCountCellVerticesWidget.h>
#include <IQWidgets/igQtExtractComponentWidget.h>
#include <IQWidgets/igQtExtractEdgesWidget.h>
#include <IQWidgets/igQtGenerateProcessIdsWidget.h>
#include <IQWidgets/igQtGlobalIdWidget.h>
#include <IQWidgets/igQtMergeVectorComponentsWidget.h>
#include <IQWidgets/igQtPointAndCellIdsWidget.h>
#include <IQWidgets/igQtResampleToImageWidget.h>
#include <IQWidgets/igQtResampleToLineWidget.h>
#include <IQWidgets/igQtTriangleStripWidget.h>
#include <CellSize/iGameCellSizeFilter.h>
#include <iGameScene.h>
#include <QDockWidget>
#include <QScrollArea>
#include <QStringList>
#include <functional>
#include <cmath>

using namespace iGame;

namespace {
QString standardFilterPanelKey(const QString& id) { return QStringLiteral("standardFilterPanel_") + id; }

// 参数面板复用同一个会话；结果相关的信号只在第一次创建时连接。
// 面板放在左侧工具面板的 Tab 里，与流场、轮廓提取等保持一致。
// setup 的第二个参数是「关闭这个 Tab」，面板自带的关闭/取消按钮连到它即可。
template<class Panel, class Setup, class... Args>
Panel* filterPanel(igQtMainWindow* window, const QString& id, Setup setup, Args... args) {
    const QString key = standardFilterPanelKey(id);
    auto* panel = window->findChild<Panel*>(key);
    if (!panel) {
        panel = new Panel(args..., window);
        panel->setObjectName(key);
        panel->hide();
        std::function<void()> close = [window, key]() { window->closeLeftToolWidget(key); };
        setup(panel, close);
    }
    return panel;
}

// 菜单文字形如 "resample_to_image（重采样到图像）"，Tab 标题只取括号里的中文
QString panelTitle(const QString& actionText) {
    const int left = actionText.indexOf(QStringLiteral("（"));
    const int right = actionText.lastIndexOf(QStringLiteral("）"));
    if (left >= 0 && right > left + 1) return actionText.mid(left + 1, right - left - 1);
    return actionText;
}

void showFilterPanel(igQtMainWindow* window, const QString& id, const QString& actionText) {
    const QString key = standardFilterPanelKey(id);
    auto* panel = window->findChild<QWidget*>(key);
    if (!panel) return;
    window->openLeftToolWidget(key, panel, panelTitle(actionText));
}
} // namespace

bool igQtMainWindow::connectImportedFilterAction(QAction* action, const QString& filterId) {
    auto currentFilterInput = [this](const QString& title) -> DataObject::Pointer {
        auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto obj = model ? model->GetDataObject() : nullptr;
        if (!obj) {
            showDarkFramelessMessage(title, QStringLiteral("请先加载并选择一个模型。"));
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

        if (filterId == QStringLiteral("append_arc_length")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        auto* scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto data = model ? model->GetDataObject() : nullptr;
        if (data == nullptr) {
            showDarkFramelessMessage(QStringLiteral("No Model Available"),
                QStringLiteral("Please load and select a model first."));
            return;
        }
        AppendArcLengthFilter::Pointer filter = AppendArcLengthFilter::New();
        filter->SetInput(data);
        if (!filter->Execute()) {
            std::string message = filter->GetMessage();
            if (message.empty()) message = "AppendArcLengthFilter execute failed";
            showDarkFramelessMessage(QStringLiteral("Warning"), QString::fromStdString(message));
            return;
        }
        // Register a distinct pipeline result, retaining the original source model.
        auto sourceItem = modelTreeWidget->getItemFromObject(data);
        data = filter->GetOutput();
        static int polylineLengthNumber = 0;
        const QString outputName = QStringLiteral("PolylineLength%1").arg(++polylineLengthNumber);
        data->SetName(outputName.toStdString());
        const int outputModelId = modelTreeWidget->addDataObjectToModelTree(data, ItemSource::Algorithm);
        modelTreeWidget->renameModelRow(data, outputName);
        model = scene->GetModelById(outputModelId);
        if (sourceItem) { sourceItem->changeVisibility(false); }
        auto attributeSet = data->GetAttributeSet();
        int attrIndex = attributeSet ? attributeSet->GetAttributeIndex(AppendArcLengthFilter::GetArrayName()) : -1;
        IGsize valueCount = 0;
        if (attributeSet && attrIndex >= 0) {
            auto& attribute = attributeSet->GetAttribute(attrIndex);
            if (attribute.pointer) { valueCount = attribute.pointer->GetNumberOfValues(); }
        }
        auto item = modelTreeWidget->getItemFromObject(data);
        if (item && attrIndex >= 0) {
            item->setExpanded(true);
            for (int i = 0; i < item->childCount(); ++i) {
                auto child = dynamic_cast<AttribTreeWidgetItem*>(item->child(i));
                if (!child || child->attributeIndex() != attrIndex) { continue; }
                item->setCurrentChild(child);
                item->setSelected(false);
                item->viewAttribute(attrIndex, -1);
                child->setSelected(true);
                modelTreeWidget->setCurrentItem(child);
                break;
            }
        }
        if (ui->dockWidget_SearchInfo && ui->dockWidget_SearchInfo->isVisible()) {
            ui->widget_SearchInfo->setCurrentModel(model);
        }
        showDarkFramelessMessage(QStringLiteral("Success"),
            QStringLiteral("Arc length appended: %1 point(s), %2 polyline(s).")
            .arg(static_cast<qulonglong>(valueCount))
            .arg(static_cast<qulonglong>(filter->GetNumberOfProcessedLines())),
            true);
        });
            return true;
        }
        if (filterId == QStringLiteral("cell_quality")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        // 1. 创建对话框
        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("单元质量评估 (Cell Quality)"));
        dialog->setFilterDescription(
                QStringLiteral("对齐 ParaView Cell Quality：选择一个统一指标作用于模型内所有单元；\n"
                               "不适用的单元填入「不支持值」，执行后汇总报告跳过数量。\n"
                               "适用范围：四面体、六面体、三角形、四边形；\n"
                               "各指标对单元类型的适用性按 ParaView Cell Quality 对齐。"));

        // 2. 统一指标下拉框（与 ParaView 一致的交互，不再区分四面体/六面体专用算法）
        //    20 项按 ParaView Cell Quality 的 Quality Measure 列表原样排列
        std::vector<QString> metricNames = {
                QStringLiteral("面积 (Area)"),
                QStringLiteral("纵横比 (Aspect Ratio)"),
                QStringLiteral("条件数 (Condition)"),
                QStringLiteral("畸变度 (Distortion)"),
                QStringLiteral("边长比 (Edge Ratio)"),
                QStringLiteral("等角歪斜 (Equiangle Skew)"),
                QStringLiteral("雅可比行列式 (Jacobian)"),
                QStringLiteral("最小内角 (Minimum Angle)"),
                QStringLiteral("Oddy"),
                QStringLiteral("半径比 (Radius Ratio)"),
                QStringLiteral("相对尺寸平方 (Relative Size Squared)"),
                QStringLiteral("归一化雅可比 (Scaled Jacobian)"),
                QStringLiteral("形状 (Shape)"),
                QStringLiteral("形状与尺寸 (Shape and Size)"),
                QStringLiteral("剪切 (Shear)"),
                QStringLiteral("剪切与尺寸 (Shear and Size)"),
                QStringLiteral("歪斜度 (Skew)"),
                QStringLiteral("伸展度 (Stretch)"),
                QStringLiteral("锥度 (Taper)"),
                QStringLiteral("翘曲度 (Warpage)")};

        std::vector<iGame::CellMeshMetricsFilter::CellQualityMetric> metricEnums = {
                iGame::CellMeshMetricsFilter::QUALITY_AREA,
                iGame::CellMeshMetricsFilter::QUALITY_ASPECT_RATIO,
                iGame::CellMeshMetricsFilter::QUALITY_CONDITION,
                iGame::CellMeshMetricsFilter::QUALITY_DISTORTION,
                iGame::CellMeshMetricsFilter::QUALITY_EDGE_RATIO,
                iGame::CellMeshMetricsFilter::QUALITY_EQUIANGLE_SKEW,
                iGame::CellMeshMetricsFilter::QUALITY_JACOBIAN,
                iGame::CellMeshMetricsFilter::QUALITY_MIN_ANGLE,
                iGame::CellMeshMetricsFilter::QUALITY_ODDY,
                iGame::CellMeshMetricsFilter::QUALITY_RADIUS_RATIO,
                iGame::CellMeshMetricsFilter::QUALITY_RELATIVE_SIZE_SQUARED,
                iGame::CellMeshMetricsFilter::QUALITY_SCALED_JACOBIAN,
                iGame::CellMeshMetricsFilter::QUALITY_SHAPE,
                iGame::CellMeshMetricsFilter::QUALITY_SHAPE_AND_SIZE,
                iGame::CellMeshMetricsFilter::QUALITY_SHEAR,
                iGame::CellMeshMetricsFilter::QUALITY_SHEAR_AND_SIZE,
                iGame::CellMeshMetricsFilter::QUALITY_SKEW,
                iGame::CellMeshMetricsFilter::QUALITY_STRETCH,
                iGame::CellMeshMetricsFilter::QUALITY_TAPER,
                iGame::CellMeshMetricsFilter::QUALITY_WARPAGE};

        int comboID =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("评估指标"), metricNames);

        // 3. 不支持/无效值（对齐 ParaView 的 UnsupportedGeometry，默认 -1.0）
        int unsupportedId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("不支持/无效值"), "-1.0");

        dialog->show();

        // 4. 确认回调逻辑
        dialog->setApplyFunctor([=, this]() {
            // 4.1 前置校验：必须有选中模型
            auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
            if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                         QStringLiteral("请先在模型树中选中需要评估的体网格模型。"));
                dialog->close();
                return;
            }
            auto obj = scene->GetCurrentModel()->GetDataObject();
            if (obj == nullptr) {
                showDarkFramelessMessage(QStringLiteral("无可用模型"), QStringLiteral("当前模型数据对象为空。"));
                dialog->close();
                return;
            }

            // 4.2 读取指标
            bool ok = false;
            int selectedIndex = dialog->getComboIndex(comboID, ok);
            if (!ok || selectedIndex < 0 || selectedIndex >= (int) metricEnums.size()) {
                showDarkFramelessMessage(QStringLiteral("无效选择"), QStringLiteral("请选择一个有效的评估指标。"));
                dialog->close();
                return;
            }
            auto metric = metricEnums[selectedIndex];

            // 4.3 读取"不支持值"，非法输入则回退到 -1.0
            double unsupportedValue = -1.0;
            bool okValue = false;
            double parsed = dialog->getDouble(unsupportedId, okValue);
            if (okValue) {
                unsupportedValue = parsed;
            } else {
                showDarkFramelessMessage(QStringLiteral("参数提示"),
                                         QStringLiteral("「不支持/无效值」不是合法数字，已回退为默认值 -1.0。"));
            }

            // 4.4 执行 filter
            auto filter = iGame::CellMeshMetricsFilter::New();
            filter->setMetric(metric);
            filter->setUnsupportedValue(unsupportedValue);
            filter->SetInput(obj);
            if (!filter->Execute()) {
                showDarkFramelessMessage(
                        QStringLiteral("计算失败"),
                        QStringLiteral("单元质量评估执行失败，请检查输入是否为可识别的体网格。"));
                dialog->close();
                return;
            }

            // 4.5 输出独立结果节点（不修改原模型）
            auto qualityObj = filter->GetOutput();
            if (qualityObj == nullptr) {
                showDarkFramelessMessage(QStringLiteral("计算失败"), QStringLiteral("未产生有效的输出结果。"));
                dialog->close();
                return;
            }
            qualityObj->SetName(obj->GetName() + "_CellQuality");
            modelTreeWidget->addDataObjectToModelTree(qualityObj, Algorithm);
            rendererWidget->update();

            // 4.6 汇总报告：支持数量 / 跳过数量 / 使用的指标与不支持值
            const int supported = filter->GetSupportedCount();
            const int unsupported = filter->GetUnsupportedCount();
            const int skipped = filter->GetSkippedBlockCount();
            QString report = QStringLiteral("单元质量评估完成。\n\n"
                                            "• 评估指标：%1\n"
                                            "• 成功计算单元：%2 个\n"
                                            "• 跳过不适用单元：%3 个（已标记为 %4）\n"
                                            "• 无法处理的网格块：%5 个\n"
                                            "• 结果已生成为独立节点：%6")
                                     .arg(metricNames[selectedIndex])
                                     .arg(supported)
                                     .arg(unsupported)
                                     .arg(unsupportedValue)
                                     .arg(skipped)
                                     .arg(QString::fromStdString(qualityObj->GetName()));
            if (supported == 0 && unsupported > 0) {
                report += QStringLiteral(
                        "\n\n提示：当前指标对该模型中的所有单元均不适用。\n"
                        "请确认单元类型（四面体/六面体/三角形/四边形），或更换评估指标 ——\n"
                        "不同指标对单元类型的适用范围与 ParaView Cell Quality 一致。");
            }
            showDarkFramelessMessage(QStringLiteral("评估完成"), report, true);
        });
    });
            return true;
        }
        if (filterId == QStringLiteral("connectivity")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("连通区域"), QStringLiteral("请先选择一个模型。"));
                    return;
                }
                auto object = currentModel->GetDataObject();
                if (iGame::DynamicCast<iGame::SurfaceMesh>(object).IsNull()) {
                    showDarkFramelessMessage(QStringLiteral("连通区域"),
                                             QStringLiteral("当前模型不是曲面网格（SurfaceMesh）"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("连通区域"));
                dialog->setFilterDescription(
                        QStringLiteral("按共享点连通性给区域编号或抽取区域"));
                const int modeId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("抽取模式"),
                        std::vector<QString>{QStringLiteral("点种子"),
                                             QStringLiteral("单元种子"),
                                             QStringLiteral("指定区域"),
                                             QStringLiteral("最大区域"),
                                             QStringLiteral("全部区域"),
                                             QStringLiteral("最近点")});
                const int colorId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("按区域编号着色"), "true");
                const int assignId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("编号方式"),
                        std::vector<QString>{QStringLiteral("遍历顺序"), QStringLiteral("单元数升序"),
                                             QStringLiteral("单元数降序")});
                const int seedsId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("种子编号"), "0");
                const int specId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("区域编号"), "0");
                const int closestId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("最近点坐标 (x, y, z)"), "0,0,0");
                dialog->setParameterLayoutVertical();
                dialog->setMinimumWidth(440);
                dialog->resize(qMax(480, dialog->fontMetrics().horizontalAdvance(
                        QStringLiteral("最近点坐标 (x, y, z)")) + 120), 520);
                for (int id : {seedsId, specId}) {
                    auto* edit = qobject_cast<QLineEdit*>(dialog->getWidget(id));
                    edit->setPlaceholderText(QStringLiteral("例如：0, 2, 5（编号从 0 开始）"));
                    edit->setToolTip(QStringLiteral("多个编号用英文逗号或空格分隔，编号从 0 开始。"));
                }
                auto* closestEdit = qobject_cast<QLineEdit*>(dialog->getWidget(closestId));
                closestEdit->setPlaceholderText(QStringLiteral("例如：1.5, 0, -2.0"));
                closestEdit->setToolTip(QStringLiteral("输入模型坐标中的 x、y、z，用英文逗号或空格分隔。"));
                if (auto* scrollArea = dialog->findChild<QScrollArea*>(QStringLiteral("scrollArea"))) {
                    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                }
                // 默认「全部区域」：一次即可看到所有连通区域的编号着色。
                if (auto* modeCombo = qobject_cast<QComboBox*>(dialog->getWidget(modeId))) {
                    modeCombo->setCurrentIndex(4);
                }
                auto* inputError = new QLabel(dialog);
                inputError->setObjectName(QStringLiteral("ConnectivityInputError"));
                inputError->setWordWrap(true);
                inputError->setStyleSheet(QStringLiteral("color: #ffb4ab;"));
                dialog->addRowWidget(inputError);
                inputError->hide();
                for (int id : {seedsId, specId, closestId}) {
                    connect(qobject_cast<QLineEdit*>(dialog->getWidget(id)), &QLineEdit::textChanged,
                            inputError, &QWidget::hide);
                }
                auto* modeCombo = qobject_cast<QComboBox*>(dialog->getWidget(modeId));
                auto updateModeParameters = [=](int mode) {
                    inputError->hide();
                    dialog->setParameterVisible(seedsId, mode == 0 || mode == 1);
                    dialog->setParameterVisible(specId, mode == 2);
                    dialog->setParameterVisible(closestId, mode == 5);
                    const QString descriptions[] = {
                        QStringLiteral("抽取包含指定点的连通区域。填写点编号，多个编号用逗号分隔。"),
                        QStringLiteral("抽取包含指定单元的连通区域。填写单元编号，多个编号用逗号分隔。"),
                        QStringLiteral("抽取指定编号的区域。区域编号从 0 开始，由编号方式决定。"),
                        QStringLiteral("仅保留单元数最多的连通区域，无需额外参数。"),
                        QStringLiteral("保留全部连通区域，可按区域编号着色，无需额外参数。"),
                        QStringLiteral("抽取距离指定坐标最近的连通区域。填写模型坐标 x、y、z。")};
                    dialog->setFilterDescription(descriptions[mode]);
                    if (mode == 0 || mode == 1) {
                        auto* seeds = qobject_cast<QLineEdit*>(dialog->getWidget(seedsId));
                        seeds->setToolTip(mode == 0 ? QStringLiteral("填写点编号，多个编号用逗号或空格分隔。")
                                                   : QStringLiteral("填写单元编号，多个编号用逗号或空格分隔。"));
                    }
                };
                connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                        dialog, updateModeParameters);
                updateModeParameters(modeCombo->currentIndex());
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    const int mode = dialog->getComboIndex(modeId, ok);
                    auto filter = iGame::ConnectivityFilter::New();
                    switch (mode) {
                        case 0: filter->SetExtractionMode(iGame::ConnectivityFilter::POINT_SEEDED_REGIONS); break;
                        case 1: filter->SetExtractionMode(iGame::ConnectivityFilter::CELL_SEEDED_REGIONS); break;
                        case 2: filter->SetExtractionMode(iGame::ConnectivityFilter::SPECIFIED_REGIONS); break;
                        case 3: filter->SetExtractionMode(iGame::ConnectivityFilter::LARGEST_REGION); break;
                        case 4: filter->SetExtractionMode(iGame::ConnectivityFilter::ALL_REGIONS); break;
                        default: filter->SetExtractionMode(iGame::ConnectivityFilter::CLOSEST_POINT_REGION); break;
                    }
                    filter->SetColorRegions(dialog->getChecked(colorId, ok));
                    const int assign = dialog->getComboIndex(assignId, ok);
                    if (assign == 1) {
                        filter->SetRegionIdAssignmentMode(iGame::ConnectivityFilter::CELL_COUNT_ASCENDING);
                    } else if (assign == 2) {
                        filter->SetRegionIdAssignmentMode(iGame::ConnectivityFilter::CELL_COUNT_DESCENDING);
                    } else {
                        filter->SetRegionIdAssignmentMode(iGame::ConnectivityFilter::UNSPECIFIED);
                    }

                    auto parseDoubles = [](const QString& text, std::vector<double>& out) -> bool {
                        out.clear();
                        QString t = text;
                        t.replace(',', ' ');
                        const auto parts = t.simplified().split(' ', Qt::SkipEmptyParts);
                        for (const auto& p : parts) {
                            bool c = false;
                            const double v = p.toDouble(&c);
                            if (!c || !std::isfinite(v)) { return false; }
                            out.push_back(v);
                        }
                        return !out.empty();
                    };

                    auto rejectInput = [=](int id, const QString& message) {
                        inputError->setText(message);
                        inputError->show();
                        if (auto* edit = qobject_cast<QLineEdit*>(dialog->getWidget(id))) {
                            edit->setFocus();
                            edit->selectAll();
                        }
                    };
                    auto parseIds = [&](int id, qlonglong maxId, std::vector<igIndex>& values) {
                        auto* edit = qobject_cast<QLineEdit*>(dialog->getWidget(id));
                        QString text = edit->text();
                        text.replace(',', ' ');
                        const auto parts = text.simplified().split(' ', Qt::SkipEmptyParts);
                        values.clear();
                        for (const auto& part : parts) {
                            bool valid = false;
                            const qlonglong value = part.toLongLong(&valid);
                            if (!valid || value < 0 || value > maxId ||
                                static_cast<unsigned long long>(value) > static_cast<unsigned long long>(std::numeric_limits<igIndex>::max())) {
                                rejectInput(id, QStringLiteral("请输入 0 到 %1 范围内的整数编号，多个编号用逗号或空格分隔。").arg(maxId));
                                return false;
                            }
                            values.push_back(static_cast<igIndex>(value));
                        }
                        if (values.empty()) {
                            rejectInput(id, QStringLiteral("请至少输入一个整数编号。"));
                            return false;
                        }
                        return true;
                    };
                    inputError->hide();
                    if (mode == 0 || mode == 1) {
                        auto mesh = iGame::DynamicCast<iGame::SurfaceMesh>(object);
                        const qlonglong count = static_cast<qlonglong>(mode == 0 ? mesh->GetNumberOfPoints() : mesh->GetNumberOfFaces());
                        std::vector<igIndex> values;
                        if (!parseIds(seedsId, count - 1, values)) return;
                        filter->InitializeSeedList();
                        for (igIndex value : values) filter->AddSeed(value);
                    } else if (mode == 2) {
                        std::vector<igIndex> values;
                        if (!parseIds(specId, std::numeric_limits<int>::max(), values)) return;
                        filter->InitializeSpecifiedRegionList();
                        for (igIndex value : values) filter->AddSpecifiedRegion(static_cast<int>(value));
                    } else if (mode == 5) {
                        auto* edit = qobject_cast<QLineEdit*>(dialog->getWidget(closestId));
                        std::vector<double> values;
                        if (!parseDoubles(edit->text(), values) || values.size() != 3) {
                            rejectInput(closestId, QStringLiteral("请输入三个有限坐标值 x, y, z，例如：1.5, 0, -2.0。"));
                            return;
                        }
                        filter->SetClosestPoint(values[0], values[1], values[2]);
                    }

                    filter->SetInput(object);
                    if (!filter->Execute()) {
                        if (mode == 2 && (filter->GetMessage() == "Specified region ID is outside the valid range" ||
                                          filter->GetMessage() == "Extraction produced an empty mesh")) {
                            rejectInput(specId, QStringLiteral("请填写 0 到 %1 范围内的区域编号（按当前编号方式排序）。")
                                    .arg(filter->GetNumberOfExtractedRegions() - 1));
                            return;
                        }
                        showDarkFramelessMessage(
                                QStringLiteral("连通区域"),
                                QStringLiteral("执行失败：%1").arg(QString::fromStdString(filter->GetMessage())));
                        return;
                    }
                    auto output = filter->GetOutput();
                    if (!output) {
                        showDarkFramelessMessage(QStringLiteral("连通区域"), QStringLiteral("输出对象为空"));
                        return;
                    }
                    output->SetName(object->GetName() + "_Connectivity");

                    auto inputDraw = iGame::DynamicCast<iGame::DrawObject>(object);
                    auto outDraw = iGame::DynamicCast<iGame::DrawObject>(output);
                    if (inputDraw && outDraw) {
                        // 只继承视图样式；不继承颜色映射表——RegionId 是新增属性，需用输出自身的
                        // 颜色映射（range 由 RegionId 数据范围确定），否则会出现只有一种颜色。
                        outDraw->SetViewStyle(static_cast<IGenum>(inputDraw->GetViewStyle()));
                    }

                    dialog->setFilterDescription(
                            QStringLiteral("共 %1 个连通区域").arg(filter->GetNumberOfExtractedRegions()));
                    modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);

                    // 优先按 RegionId（点标量）上色；否则退回第一个标量点/单元属性。
                    if (auto attrSet = output->GetAttributeSet(); attrSet && filter->GetColorRegions()) {
                        int activeIdx = attrSet->GetAttributeIndex(iGame::ConnectivityFilter::RegionIdName);
                        if (activeIdx < 0) {
                            int pointAttrIdx = -1;
                            int cellAttrIdx = -1;
                            for (int i = 0; i < static_cast<int>(attrSet->GetNumberOfAttributes()); ++i) {
                                auto& attr = attrSet->GetAttribute(i);
                                if (attr.IsNone() || attr.isDeleted || attr.type != IG_SCALAR) { continue; }
                                if (attr.attachmentType == IG_POINT && pointAttrIdx < 0) {
                                    pointAttrIdx = i;
                                } else if (attr.attachmentType == IG_CELL && cellAttrIdx < 0) {
                                    cellAttrIdx = i;
                                }
                            }
                            activeIdx = (pointAttrIdx >= 0) ? pointAttrIdx : cellAttrIdx;
                        }
                        if (activeIdx >= 0 && outDraw) {
                            // 显式按该属性的数据范围设置色表 range，保证热力图映射正确
                            // （不依赖 SetAttributeWithPointData 内部的 MTime 条件）。
                            auto& attr = attrSet->GetAttribute(activeIdx);
                            if (auto range = attr.GetDataRange()) {
                                double lo = range->GetValue(2);
                                double hi = range->GetValue(3);
                                if (hi <= lo) { hi = lo + 1.0; } // 单值退化范围，避免所有值落入同一点
                                if (auto cm = outDraw->GetColorMapper()) {
                                    cm->SetRangeStable(false);
                                    cm->SetRange(lo, hi);
                                }
                            }
                            if (auto scene2 = rendererWidget->GetScene()) {
                                outDraw->ViewCloudPicture(scene2, activeIdx, 0);
                            }
                        }
                    }

                    modelTreeWidget->updateCloudPicture();
                    rendererWidget->update();
                    dialog->close();
                });
            });
            return true;
        }
        if (filterId == QStringLiteral("decimate_polyline")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                auto input = currentModel ? currentModel->GetDataObject() : nullptr;
                if (!DecimatePolylineFilter::CanProcessInput(input)) {
                    showDarkFramelessMessage(
                            QStringLiteral("折线简化失败"),
                            QStringLiteral("当前数据不支持该算法。"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("折线简化 (Decimate Polyline)"));
                dialog->setMinimumWidth(520);
                dialog->setFilterDescription(
                        QStringLiteral("与 ParaView 一致，支持角度、自定义点字段和距离三种策略；"
                                       "每条折线独立简化，端点始终保留。"));
                const int reductionId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("目标缩减率 (0..1)"), QStringLiteral("0.9"));
                const int maximumErrorId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("最大误差 (Maximum Error)"),
                        QStringLiteral("1.7976931348623157e+308"));
                const int strategyId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX,
                        QStringLiteral("简化策略 (Decimation Strategy)"),
                        std::vector<QString>{QStringLiteral("角度策略 (Angle)"),
                                             QStringLiteral("自定义字段策略 (Custom Field)"),
                                             QStringLiteral("距离策略 (Distance)")});
                std::vector<QString> customFieldNames;
                auto inputPointSet = DynamicCast<PointSet>(input);
                auto attributes = input->GetAttributeSet();
                auto allAttributes = attributes ? attributes->GetAllAttributes() : nullptr;
                if (allAttributes) {
                    for (IGsize i = 0; i < allAttributes->GetNumberOfElements(); ++i) {
                        const auto& attribute = allAttributes->GetElement(i);
                        if (attribute.isDeleted || attribute.attachmentType != IG_POINT ||
                            !attribute.pointer || attribute.pointer->GetDimension() < 1 ||
                            attribute.pointer->GetName().empty() ||
                            !inputPointSet || attribute.pointer->GetNumberOfElements() <
                                                      inputPointSet->GetNumberOfPoints()) {
                            continue;
                        }
                        switch (attribute.pointer->GetArrayType()) {
                        case IG_FloatArray:
                        case IG_DoubleArray:
                        case IG_IntArray:
                        case IG_UnsignedIntArray:
                        case IG_CharArray:
                        case IG_UnsignedCharArray:
                        case IG_ShortArray:
                        case IG_UnsignedShortArray:
                        case IG_LongLongArray:
                        case IG_UnsignedLongLongArray:
                            customFieldNames.push_back(
                                    QString::fromStdString(attribute.pointer->GetName()));
                            break;
                        default: break;
                        }
                    }
                }
                const int customFieldId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX,
                        QStringLiteral("点字段 (Field Name)"),
                        customFieldNames.empty()
                                ? std::vector<QString>{QStringLiteral("<无可用点字段>")}
                                : customFieldNames);
                auto* strategyCombo = qobject_cast<QComboBox*>(dialog->getWidget(strategyId));
                auto* customFieldCombo = qobject_cast<QComboBox*>(dialog->getWidget(customFieldId));
                if (strategyCombo) strategyCombo->setCurrentIndex(2);

                // ParaView exposes FieldName as a child property of the custom-field
                // strategy. Keep the whole row hidden for Angle/Distance and reveal it
                // only when Custom Field is selected.
                auto findRowLabel = [](QWidget* value) -> QLabel* {
                    if (!value || !value->parentWidget()) return nullptr;
                    const auto grids = value->parentWidget()->findChildren<QGridLayout*>();
                    for (auto* grid : grids) {
                        const int itemIndex = grid->indexOf(value);
                        if (itemIndex < 0) continue;
                        int row = 0, column = 0, rowSpan = 0, columnSpan = 0;
                        grid->getItemPosition(itemIndex, &row, &column, &rowSpan, &columnSpan);
                        for (int c = 0; c < grid->columnCount(); ++c) {
                            auto* item = grid->itemAtPosition(row, c);
                            if (!item || item->widget() == value) continue;
                            if (auto* label = qobject_cast<QLabel*>(item->widget())) return label;
                        }
                    }
                    return nullptr;
                };
                auto* customFieldLabel = findRowLabel(customFieldCombo);
                auto setCustomFieldVisible = [customFieldCombo, customFieldLabel](bool visible) {
                    if (customFieldLabel) customFieldLabel->setVisible(visible);
                    if (customFieldCombo) customFieldCombo->setVisible(visible);
                };
                setCustomFieldVisible(false);
                if (strategyCombo && customFieldCombo) {
                    connect(strategyCombo, qOverload<int>(&QComboBox::currentIndexChanged), dialog,
                            [setCustomFieldVisible](int index) {
                                setCustomFieldVisible(index == 1);
                            });
                }
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool reductionOk = false;
                    bool maximumErrorOk = false;
                    bool strategyOk = false;
                    const double targetReduction = dialog->getDouble(reductionId, reductionOk);
                    const double maximumError = dialog->getDouble(maximumErrorId, maximumErrorOk);
                    const int strategy = dialog->getComboIndex(strategyId, strategyOk);
                    if (!reductionOk || !std::isfinite(targetReduction) ||
                        targetReduction < 0.0 || targetReduction > 1.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("目标缩减率必须是 0 到 1 之间的数值。"));
                        return;
                    }
                    if (!maximumErrorOk || !std::isfinite(maximumError) || maximumError < 0.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("最大误差必须是非负数值。"));
                        return;
                    }
                    if (!strategyOk || strategy < 0 || strategy > 2) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("请选择有效的简化策略。"));
                        return;
                    }
                    if (strategy == 1 && customFieldNames.empty()) {
                        showDarkFramelessMessage(
                                QStringLiteral("参数错误"),
                                QStringLiteral("自定义字段策略需要至少一个数值型点数据数组。"));
                        return;
                    }

                    auto filter = DecimatePolylineFilter::New();
                    filter->SetInput(input);
                    filter->SetTargetReduction(targetReduction);
                    filter->SetMaximumError(maximumError);
                    if (strategy == 0) {
                        filter->SetDecimationStrategy(
                                DecimatePolylineFilter::DecimationStrategy::Angle);
                    } else if (strategy == 1) {
                        filter->SetDecimationStrategy(
                                DecimatePolylineFilter::DecimationStrategy::CustomField);
                        if (customFieldCombo) {
                            filter->SetCustomFieldName(
                                    customFieldCombo->currentText().toStdString());
                        }
                    } else {
                        filter->SetDecimationStrategy(
                                DecimatePolylineFilter::DecimationStrategy::Distance);
                    }
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("执行出错"),
                                QStringLiteral("Poly Data 必须包含显式 LINES；自定义字段策略还要求"
                                               "选择有效的数值型点数据数组。"));
                        return;
                    }

                    auto output = filter->GetOutput();
                    output->SetName(input->GetName() + "_DecimatePolyline");
                    if (auto outputDrawObject = DynamicCast<DrawObject>(output)) {
                        // A line-only result is clearest when both retained vertices and segments are shown.
                        outputDrawObject->SetViewStyle(IG_POINTS | IG_WIREFRAME);
                        outputDrawObject->SetPointSize(7.0f);
                        outputDrawObject->SetLineWidth(3.0f);
                    }
                    // ParaView hides the source when a filter result is applied. The eye icon can
                    // still be used to show it again for an overlay comparison.
                    if (auto* inputItem = modelTreeWidget->getItemFromObject(input)) {
                        inputItem->changeVisibility(false);
                    }
                    modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);
                    rendererWidget->update();
                    dialog->close();
                });
            });
            return true;
        }
        if (filterId == QStringLiteral("integrate_variables")) {
            connect(action, &QAction::triggered, this, [this](bool) {
        auto* scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto data = model ? model->GetDataObject() : nullptr;
        if (data == nullptr) {
            showDarkFramelessMessage(QStringLiteral("No Model Available"),
                                     QStringLiteral("Please load and select a model first."));
            return;
        }
        const auto dtype = data->GetDataObjectType();
        if (dtype != IG_SURFACE_MESH && dtype != IG_VOLUME_MESH &&
            dtype != IG_UNSTRUCTURED_MESH && dtype != IG_STRUCTURED_MESH) {
            showDarkFramelessMessage(
                    QStringLiteral("Unsupported Model"),
                    QStringLiteral("Integrate Variables supports surface, volume, unstructured, and structured meshes."));
            return;
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("积分变量 (Integrate Variables)"));
        dialog->setFilterDescription(QStringLiteral(
                "对最高维度单元的点属性和单元属性进行积分。输出为一个点，并保存积分结果及总长度、总面积或总体积。"));

        dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                             QStringLiteral("积分策略"),
                             std::vector<QString>{QStringLiteral("Linear Strategy")});
        const int divideCellDataId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_CHECK_BOX,
                QStringLiteral("单元数据除以总测度"),
                QStringLiteral("false"));

        dialog->setApplyFunctor([this, dialog, data, divideCellDataId]() {
            bool optionOk = false;
            const bool divideCellData = dialog->getChecked(divideCellDataId, optionOk);
            if (!optionOk) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("无法读取积分选项。"));
                return;
            }

            IntegrateVariablesFilter::Pointer filter = IntegrateVariablesFilter::New();
            filter->SetInput(data);
            filter->SetDivideAllCellDataByMeasure(divideCellData);
            if (!filter->Execute()) {
                std::string message = filter->GetMessage();
                if (message.empty()) message = "IntegrateVariablesFilter execute failed";
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QString::fromStdString(message));
                return;
            }

            auto output = filter->GetOutput();
            if (!output) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("积分算法没有生成有效输出。"));
                return;
            }

            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
            if (auto item = modelTreeWidget->getItemFromObject(output)) item->setExpanded(true);
            rendererWidget->update();
            dialog->hide();
            dialog->deleteLater();

            ui->dockWidget_SearchInfo->show();
            ui->dockWidget_SearchInfo->raise();
            ui->widget_SearchInfo->setCurrentModel(rendererWidget->GetScene()->GetCurrentModel());
        });
        dialog->resize(480, 320);
        dialog->show();
    });
            return true;
        }
        if (filterId == QStringLiteral("multiblock_surface_as_multiblock")) {
            connect(action, &QAction::triggered, this, [this, currentFilterInput, refreshFilterResult]() {
        const QString title = QStringLiteral("多块模型表面提取");
        auto input = currentFilterInput(title);
        if (!input) return;

        auto multiBlockFilter = MultiBlockGeometryFilter::New();
        multiBlockFilter->SetInput(input);
        if (!multiBlockFilter->Execute()) {
            QString errorMsg = QString::fromStdString(multiBlockFilter->GetMessage());
            if (errorMsg.isEmpty()) { errorMsg = QStringLiteral("多块模型表面提取失败。"); }
            showDarkFramelessMessage(title, errorMsg);
            return;
        }

        auto multiBlockObj = multiBlockFilter->GetOutput();
        if (multiBlockObj) {
            multiBlockObj->SetName(input->GetName() + "_MultiBlockSurface");
        }
        refreshFilterResult(input, multiBlockObj, title);

        // 子构件层面的异常信息汇总展示（避免静默丢块）
        const auto& failedBlocks = multiBlockFilter->GetFailedBlocks();
        if (!failedBlocks.empty()) {
            QString warningMsg = QStringLiteral("多块表面提取已完成，但以下 %1 个子构件处理异常（已为您原样保留）：\n")
                                     .arg(failedBlocks.size());
            for (const auto& block : failedBlocks) {
                warningMsg += QStringLiteral("• [%1]: %2\n")
                                  .arg(QString::fromStdString(block.path), QString::fromStdString(block.reason));
            }
            showDarkFramelessMessage(QStringLiteral("部分子构件提示"), warningMsg);
        }
    });
            return true;
        }
        if (filterId == QStringLiteral("ribbon")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
                auto* scene = rendererWidget->GetScene();
                if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                             QStringLiteral("请先加载并选择一个网格模型。"));
                    return;
                }
                auto model = scene->GetCurrentModel();
                auto obj = model->GetDataObject();
                if (obj == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用数据"),
                                             QStringLiteral("当前模型没有可用的网格数据。"));
                    return;
                }

                igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("带状面 (Ribbon)"));
                dialog->setFilterDescription(QStringLiteral(
                    "把线/折线沿“切向 × 线法向”构成的框架扫成有向带状面。<br>"
                    "半宽决定带宽：带宽 = 2 × 半宽；偏转角相对线法向旋转带面（0° 时带面垂直于线法向与切向）。<br>"
                    "输入需要包含线单元（IG_LINE / IG_POLY_LINE），其他单元类型会被忽略并在结果提示中说明。"));

                const int widthId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                         QStringLiteral("半宽 (Width)"), "0.5");
                const int angleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                         QStringLiteral("偏转角/度 (Angle)"), "0");
                const int useDefId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX,
                        QStringLiteral("使用默认线法向 (Use Default Normal)"), "true");
                const int nxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("默认法向 X (Normal X)"), "0");
                const int nyId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("默认法向 Y (Normal Y)"), "0");
                const int nzId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("默认法向 Z (Normal Z)"), "1");

                // 加宽面板，避免说明文字与标签过度换行
                dialog->setFixedWidth(660);

                auto* useDefCheck = qobject_cast<QCheckBox*>(dialog->getWidget(useDefId));
                if (useDefCheck != nullptr) {
                    useDefCheck->setToolTip(QStringLiteral(
                        "勾选时全部点使用下方默认法向；取消勾选则优先取输入的点法向（IG_NORMAL），"
                        "输入没有点法向时回退到默认法向并在结果提示中说明。"));
                }
                const QString normalEditTip = QStringLiteral(
                    "仅在勾选上方“使用默认线法向”后作为带面的定向依据；切线与该法向平行的那条折线会被跳过。");
                for (int id: {nxId, nyId, nzId}) {
                    if (QWidget* editor = dialog->getWidget(id)) { editor->setToolTip(normalEditTip); }
                }
                if (QWidget* angleEdit = dialog->getWidget(angleId)) {
                    angleEdit->setToolTip(QStringLiteral("单位度，取值 0–360；超出范围会被夹到区间内（与 VTK 一致）。"));
                }

                dialog->setApplyFunctor([this, dialog, obj, widthId, angleId, useDefId, nxId, nyId, nzId]() {
                    bool wok = false;
                    bool aok = false;
                    bool uok = false;
                    bool xok = false;
                    bool yok = false;
                    bool zok = false;

                    const double width = dialog->getDouble(widthId, wok);
                    const double angle = dialog->getDouble(angleId, aok);
                    const bool useDefaultNormal = dialog->getChecked(useDefId, uok);
                    const double nx = dialog->getDouble(nxId, xok);
                    const double ny = dialog->getDouble(nyId, yok);
                    const double nz = dialog->getDouble(nzId, zok);

                    if (!wok || width <= 0.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("半宽必须是大于 0 的数字（带宽 = 2 × 半宽）。"));
                        return;
                    }
                    if (!aok) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("偏转角不是有效数字。"));
                        return;
                    }
                    if (xok && yok && zok && nx == 0.0 && ny == 0.0 && nz == 0.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("默认法向不能是零向量。"));
                        return;
                    }

                    RibbonFilter::Pointer filter = RibbonFilter::New();
                    filter->SetInput(obj);
                    filter->SetWidth(width);
                    filter->SetAngle(angle);
                    filter->SetUseDefaultNormal(uok ? useDefaultNormal : true);
                    if (xok && yok && zok) { filter->SetDefaultNormal(Vector3d(nx, ny, nz)); }

                    if (!filter->Execute()) {
                        // 失败原因（无输入 / 无线单元 / 全被跳过 / 零宽度或零法向）由 filter 写在 m_Message 里
                        const std::string message = filter->GetMessage();
                        showDarkFramelessMessage(
                                QStringLiteral("带状面生成失败"),
                                message.empty() ? QStringLiteral("请检查输入是否包含线/折线单元（IG_LINE / IG_POLY_LINE）。")
                                                : QString::fromStdString(message));
                        return;
                    }

                    auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
                    if (!outMesh) {
                        showDarkFramelessMessage(QStringLiteral("带状面生成失败"),
                                                 QStringLiteral("算法未产生有效结果。"));
                        return;
                    }

                    modelTreeWidget->addDataObjectToModelTree(outMesh, ItemSource::Algorithm);
                    rendererWidget->update();
                    // 非致命提示（忽略的非线单元、被跳过的折线）只在确实发生时弹一次，
                    // 与 Tube 一致：先弹模态提示再关闭面板，避免 close dock 触发布局重建后再开模态对话框
                    if (!filter->GetMessage().empty()) {
                        showDarkFramelessMessage(QStringLiteral("带状面生成完成"),
                                                 QString::fromStdString(filter->GetMessage()), true);
                    }
                    dialog->close();
                });
                dialog->show();
            });
            return true;
        }
        if (filterId == QStringLiteral("transform") || filterId == QStringLiteral("axis_aligned_transform")) {
            connect(action, &QAction::triggered, this, [=, this]() {
        const QString title = QStringLiteral("通用几何变换");
        // 面板已打开：只把它提到最前
        if (auto* existing = findChild<igQtTransformBoxController*>(QStringLiteral("standardTransformController"))) {
            existing->FocusPanel();
            return;
        }

        auto input = currentFilterInput(title);
        if (!input) return;

        const IGenum inputType = input->GetDataObjectType();
        if (inputType != IG_SURFACE_MESH && inputType != IG_VOLUME_MESH &&
            inputType != IG_STRUCTURED_MESH && inputType != IG_UNSTRUCTURED_MESH &&
            inputType != IG_POINT_SET) {
            showDarkFramelessMessage(title,
                                     QStringLiteral("请在模型树中选择网格或点云（多帧容器暂不支持交互变换）。"));
            return;
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(title);
        dialog->setFilterDescription(QStringLiteral(
                "三维盒子交互：按住鼠标<b>中键</b>拖动。<br>"
                "拖棱或中心：平移<br>"
                "拖面手柄：缩放<br>"
                "拖面：旋转"));

        std::array<int, igQtTransformBoxController::kParamCount> parameterIds{};
        parameterIds[0] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("平移 X"), "0");
        parameterIds[1] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("平移 Y"), "0");
        parameterIds[2] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("平移 Z"), "0");
        parameterIds[3] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("旋转 X (度)"), "0");
        parameterIds[4] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("旋转 Y (度)"), "0");
        parameterIds[5] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("旋转 Z (度)"), "0");
        parameterIds[6] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("缩放 X"), "1");
        parameterIds[7] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("缩放 Y"), "1");
        parameterIds[8] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("缩放 Z"), "1");

        // 「重置」按钮：参数回到单位变换、盒子回到初始包围盒（模型不变，需要应用才生效）
        auto* resetRow = new QWidget(dialog);
        auto* resetLayout = new QHBoxLayout(resetRow);
        resetLayout->setContentsMargins(0, 0, 0, 0);
        auto* resetButton = new QPushButton(QStringLiteral("重置"), resetRow);
        resetLayout->addStretch();
        resetLayout->addWidget(resetButton);
        dialog->addRowWidget(resetRow);
        dialog->show();

        auto* controller =
                new igQtTransformBoxController(input, rendererWidget, dialog, parameterIds, this);
        controller->setObjectName(QStringLiteral("standardTransformController"));
        connect(resetButton, &QPushButton::clicked, controller, [controller]() {
            if (controller != nullptr) { controller->Reset(); }
        });
        connect(controller, &igQtTransformBoxController::Applied, this,
                [=, this](DataObject::Pointer output) {
                    refreshFilterResult(input, output, title);
                    if (auto* item = modelTreeWidget->getItemFromObject(input)) {
                        item->changeVisibility(false);
                        rendererWidget->update();
                    }
                });
        connect(controller, &igQtTransformBoxController::Failed, this,
                [this, title]() { showDarkFramelessMessage(title, QStringLiteral("几何变换失败。")); });
        connect(controller, &igQtTransformBoxController::Closed, this, [controller]() {
            if (controller == nullptr) { return; }
            controller->deleteLater();
            controller->setObjectName(QString());
        });
        if (!controller->HasBox()) {
            showDarkFramelessMessage(title, QStringLiteral("交互盒子创建失败，仍可手动输入参数后应用。"));
        }

        dialog->setApplyFunctor([controller]() {
            if (controller != nullptr) { controller->Apply(); }
        });
    });
            return true;
        }
        if (filterId == QStringLiteral("warp_by_vector")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!obj) return;

        // 收集可作为形变向量的数组:优先点关联的三维向量
        std::vector<QString> vectorItems;
        std::vector<std::string> vectorNames;
        if (auto attrs = obj->GetAttributeSet()) {
            for (IGsize i = 0; i < static_cast<IGsize>(attrs->GetNumberOfAttributes()); ++i) {
                auto& attr = attrs->GetAttribute(i);
                if (attr.isDeleted || !attr.pointer) continue;
                if (attr.attachmentType != IG_POINT) continue;
                if (attr.type != IG_VECTOR && attr.pointer->GetDimension() < 3) continue;
                const std::string name = attr.pointer->GetName();
                vectorNames.push_back(name);
                vectorItems.push_back(QStringLiteral("%1 (dim=%2)")
                                              .arg(QString::fromStdString(name))
                                              .arg(attr.pointer->GetDimension()));
            }
        }
        if (vectorItems.empty()) {
            showDarkFramelessMessage(QStringLiteral("Warning"),
                                     QStringLiteral("当前模型没有点关联的三维向量数组，无法进行向量形变。"));
            return;
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("按向量变形 (Warp By Vector)"));
        int vectorId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("向量 (Vectors)"),
                                            vectorItems);
        int scaleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                           QStringLiteral("缩放系数 (Scale Factor)"), "1");
        int autoScaleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                               QStringLiteral("自动缩放(按模型尺度)"), "false");
        int limitId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                           QStringLiteral("最大位移比例 (0 = 不限)"), "0");
        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok = false;
            const int choice = dialog->getComboIndex(vectorId, ok);
            if (!ok || choice < 0 || choice >= static_cast<int>(vectorNames.size())) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("请选择有效的向量数组。"));
                return;
            }
            const double scale = dialog->getDouble(scaleId, ok);
            const bool autoScale = dialog->getChecked(autoScaleId, ok);
            const double limit = dialog->getDouble(limitId, ok);

            auto filter = WarpByVectorFilter::New();
            filter->SetInput(obj);
            filter->SetVectorArrayName(vectorNames[static_cast<size_t>(choice)]);
            filter->SetScaleFactor(scale);
            filter->SetAutoScale(autoScale);
            filter->SetMaxDisplacementRatio(limit > 0.0 ? limit : 0.0);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("向量形变失败，请检查向量数组与缩放系数。"));
                return;
            }

            // 输出为独立的新数据对象,原模型保持不变
            auto warpOutput = filter->GetOutput();
            if (!warpOutput) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("向量形变未产生有效结果。"));
                return;
            }
            warpOutput->SetName(obj->GetName() + "_warp");
            modelTreeWidget->addDataObjectToModelTree(warpOutput, Algorithm);
            rendererWidget->update();
            dialog->close();
        });
    });
            return true;
        }
    static const QStringList ids = {
        "cell_size", "count_cell_vertices", "extract_edges",
        "global_point_and_cell_ids", "point_and_cell_ids", "process_ids", "reflect",
        "axis_aligned_reflection", "extract_component", "merge_vector_components",
        "resample_to_image", "resample_to_line", "triangle_strips",
        "resample_with_dataset", "rename_arrays", "texture_map_to_cylinder", "median",
        "smooth", "tensor_principal_invariants"};
    if (!ids.contains(filterId)) return false;

    auto displayResult = [this](DataObject::Pointer output) {
        if (!output) return;
        // Upstream panels may keep the same output object across Apply calls.
        if (modelTreeWidget->getItemFromObject(output)) {
            modelTreeWidget->updateAllAttriubute(output);
        } else {
            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
        }
        if (auto draw = DynamicCast<DrawObject>(output)) draw->ForceReConvertToDrawableData();
        if (auto* item = modelTreeWidget->getItemFromObject(output)) item->setExpanded(true);
        rendererWidget->update();
    };

    connect(action, &QAction::triggered, this, [this, action, filterId, displayResult]() {
        auto* scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto input = model ? model->GetDataObject() : nullptr;
        const QString title = action->text();
        if (!input) {
            showDarkFramelessMessage(title, QStringLiteral("请先加载并选择一个模型。"));
            return;
        }
        const QString id = filterId == "reflect" ? QStringLiteral("axis_aligned_reflection") : filterId;
        auto failed = [this, title](const QString& reason) { showDarkFramelessMessage(title, reason); };

        if (id == "tensor_principal_invariants") {
            auto* panel = filterPanel<igQtTensorPrincipalInvariantsWidget>(this, id, [&](auto* p, auto) {
                connect(p, &igQtTensorPrincipalInvariantsWidget::DrawResultModel, this, displayResult);
                connect(p, &igQtTensorPrincipalInvariantsWidget::UpdateResultModel, this, displayResult);
            });
            panel->SetOriginDataObject(input);
            showFilterPanel(this, id, title);
            return;
        }
        if (id == "smooth") {
            auto* panel = filterPanel<igQtSmoothWidget>(this, id, [&](auto* p, auto close) {
                connect(p, &igQtSmoothWidget::smoothGenerated, this, [displayResult](DataObject::Pointer output) {
                    if (!output) return;
                    output->SetName(output->GetName() + "_Smooth");
                    displayResult(output);
                });
                connect(p, &igQtSmoothWidget::cancelRequested, this, close);
            });
            panel->resetForm();
            panel->setCurrentModel(model);
            showFilterPanel(this, id, title);
            return;
        }
        if (id == "median") {
            auto* panel = filterPanel<igQtMedianFilterWidget>(this, id, [&](auto* p, auto) {
                connect(p, &igQtMedianFilterWidget::DrawMedianModel, this, displayResult);
                connect(p, &igQtMedianFilterWidget::UpdateMedianModel, this, displayResult);
            });
            panel->SetOriginDataObject(input);
            showFilterPanel(this, id, title);
            return;
        }
        if (id == "resample_with_dataset" || id == "rename_arrays") {
            if (id == "resample_with_dataset") {
                auto* panel = filterPanel<igQtResampleWithDataSet>(this, id, [](auto*, auto) {}, modelTreeWidget);
                panel->BindCurrentModel();
            } else {
                auto* panel = filterPanel<igQtRenameArrays>(this, id, [](auto*, auto) {}, modelTreeWidget);
                panel->BindCurrentModel();
            }
            showFilterPanel(this, id, title);
            return;
        }
        if (id == "texture_map_to_cylinder") {
            auto* panel = filterPanel<igQtTextureMapToCylinderWidget>(this, id, [&](auto* p, auto) {
                connect(p, &igQtTextureMapToCylinderWidget::DrawResultModel, this, displayResult);
                connect(p, &igQtTextureMapToCylinderWidget::UpdateResultModel, this, displayResult);
            });
            panel->SetOriginDataObject(input);
            showFilterPanel(this, id, title);
            return;
        }
        if (id == "cell_size") {
            auto filter = CellSizeFilter::New();
            filter->SetInput(input);
            if (!filter->Execute()) {
                failed(QString::fromStdString(filter->GetMessage()));
                return;
            }
            displayResult(filter->GetOutput());
            return;
        }
        if (id == "count_cell_vertices") {
            auto* panel = filterPanel<igQtCountCellVerticesWidget>(this, id, [&](auto* p, auto) {
                connect(p, &igQtCountCellVerticesWidget::DrawCountModel, this, displayResult);
                connect(p, &igQtCountCellVerticesWidget::UpdateCountModel, this, displayResult);
            });
            panel->SetOriginDataObject(input);
        } else if (id == "extract_edges") {
            auto* panel = filterPanel<igQtExtractEdgesWidget>(this, id, [&](auto* p, auto) {
                connect(p, &igQtExtractEdgesWidget::DrawEdgesModel, this, displayResult);
                connect(p, &igQtExtractEdgesWidget::UpdateEdgesModel, this, displayResult);
            });
            panel->SetOriginDataObject(input);
        } else if (id == "extract_component") {
            auto* panel = filterPanel<igQtExtractComponentWidget>(this, id, [&](auto* p, auto) {
                connect(p, &igQtExtractComponentWidget::DrawExtractComponentModel, this, displayResult);
                connect(p, &igQtExtractComponentWidget::UpdateExtractComponentModel, this, displayResult);
                connect(p, &igQtExtractComponentWidget::ApplyFailed, this, failed);
            });
            panel->SetOriginDataObject(input);
        } else if (id == "process_ids") {
            auto* panel = filterPanel<igQtGenerateProcessIdsWidget>(this, id, [&](auto* p, auto) {
                connect(p, &igQtGenerateProcessIdsWidget::DrawProcessIdsModel, this, displayResult);
                connect(p, &igQtGenerateProcessIdsWidget::UpdateProcessIdsModel, this, displayResult);
                connect(p, &igQtGenerateProcessIdsWidget::ApplyFailed, this, failed);
            });
            panel->SetOriginDataObject(input);
        } else if (id == "merge_vector_components") {
            auto* panel = filterPanel<igQtMergeVectorComponentsWidget>(this, id, [&](auto* p, auto) {
                connect(p, &igQtMergeVectorComponentsWidget::MergeCompleted, this,
                        [=, this](DataObject::Pointer output, const std::string& name) {
                    displayResult(output);
                    if (!output) return;
                    auto* attrs = output->GetAttributeSet();
                    const int index = attrs ? attrs->GetAttributeIndex(name) : -1;
                    auto* item = modelTreeWidget->getItemFromObject(output);
                    if (item && index >= 0 && index < item->childCount()) {
                        item->setCurrentChild(item->child(index));
                        item->viewAttribute(index, -1);
                    }
                    rendererWidget->update();
                });
            });
            panel->SetOriginDataObject(input);
        } else if (id == "point_and_cell_ids") {
            auto* panel = filterPanel<igQtPointAndCellIdsWidget>(this, id, [&](auto* p, auto close) {
                connect(p, &igQtPointAndCellIdsWidget::idsGenerated, this, displayResult);
                connect(p, &igQtPointAndCellIdsWidget::cancelRequested, this, close);
            });
            panel->setCurrentModel(model);
        } else if (id == "global_point_and_cell_ids") {
            auto* panel = filterPanel<igQtGlobalIdWidget>(this, id, [&](auto* p, auto close) {
                connect(p, &igQtGlobalIdWidget::resultReady, this, displayResult);
                connect(p, &igQtGlobalIdWidget::cancelRequested, this, close);
            });
            panel->setCurrentModel(model);
        } else if (id == "resample_to_image") {
            auto* panel = filterPanel<igQtResampleToImageWidget>(this, id, [&](auto* p, auto close) {
                connect(p, &igQtResampleToImageWidget::resultReady, this, displayResult);
                connect(p, &igQtResampleToImageWidget::closeRequested, this, close);
            });
            panel->setCurrentModel(model);
        } else if (id == "triangle_strips") {
            auto* panel = filterPanel<igQtTriangleStripWidget>(this, id, [&](auto* p, auto) {
                connect(p, &igQtTriangleStripWidget::resultReady, this,
                        [=](DataObject::Pointer surface, DataObject::Pointer lines) {
                    displayResult(surface);
                    displayResult(lines);
                });
            });
            if (!panel->isOutput(input)) panel->setInput(input);
        } else if (id == "axis_aligned_reflection") {
            auto* panel = filterPanel<igQtAxisAlignedReflectionWidget>(this, id, [&](auto* p, auto) {
                connect(p, &igQtAxisAlignedReflectionWidget::applyRequested, this, [=]() {
                    auto filter = AxisAlignedReflectionFilter::New();
                    filter->SetInput(p->input());
                    filter->SetPlane(p->plane());
                    filter->SetCenter(p->center());
                    filter->SetCopyInput(p->copyInput());
                    filter->SetFlipAllInputArrays(p->flipAllInputArrays());
                    if (!filter->Execute()) {
                        failed(QStringLiteral("反射失败，请检查输入网格和反射参数。"));
                        return;
                    }
                    displayResult(filter->GetOutput());
                });
            });
            panel->setInput(input);
        } else if (id == "resample_to_line") {
            auto* panel = filterPanel<igQtResampleToLine>(this, id, [&](auto* p, auto close) {
                connect(p, &igQtResampleToLine::ResetInteractor, this, close);
            }, modelTreeWidget);
            panel->BindCurrentModel();
        }
        showFilterPanel(this, id, title);
    });
    return true;
}
