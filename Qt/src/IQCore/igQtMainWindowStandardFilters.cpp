#include <iGameBoxStyle.h>
#include <iGameRadialStyle.h>
#include <iGameRegionPlaneStyle.h>
#include <iGameSelectionParameter.h>
#include <iGameUnstructuredMesh.h>
#include <IQComponents/igQtModelTreeWidget.h>
#include <iGameInteractor.h>
#include <iGameVolumeMesh.h>
#include <iGameSurfaceMesh.h>
#include <AppendDatasets/iGameAppendDatasetsFilter.h>
#include <FeatureExtraction/iGameDeflectNormalsFilter.h>
#include <Selection/iGameExtractCellsByRegionFilter.h>
#include <Histogram/iGameHistogramFilter.h>
#include <DataProcessing/OverlappingCellsDetector/iGameOverlappingCellsDetectorFilter.h>
#include <PointPlaneInterpolator/iGamePointPlaneInterpolatorFilter.h>
#include <DataProcessing/iGameRandomAttributesFilter.h>
#include <SurfaceVectors/iGameSurfaceVectorsFilter.h>
#include <TimeSeries/iGameTemporalShiftScaleFilter.h>
#include <VolumeOfRevolution/iGameVolumeOfRevolutionFilter.h>
#include <TensorView/iGameYieldCriteriaFilter.h>
#include <IQWidgets/igQtExtractTimeStepsWidget.h>
#include <IQWidgets/igQtHistogramChartWidget.h>
#include <IQWidgets/igQtConnectedSurfacePropertiesWidget.h>
#include <QListWidget>
#include <QSignalBlocker>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QTimer>
#include <IQWidgets/igQtSearchInfoWidget.h>
#include <IQWidgets/igQtTransformBoxController.h>
#include <AppendArcLength/iGameAppendArcLengthFilter.h>
#include <MeshMetrics/iGameCellMeshMetricsFilter.h>
#include <Connectivity/iGameConnectivityFilter.h>
#include <DataProcessing/iGameDecimatePolylineFilter.h>
#include <IntegrateVariables/iGameIntegrateVariablesFilter.h>
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


    if (filterId == "compute_connected_surface_properties") {
        connect(action, &QAction::triggered, this, [this, action, currentFilterInput]() {
            const auto title = action->text();
            auto input = currentFilterInput(title);
            if (!input) return;
            auto scene = rendererWidget->GetScene();
            auto model = scene ? scene->GetCurrentModel() : nullptr;
            if (DynamicCast<SurfaceMesh>(input).IsNull()) {
                showDarkFramelessMessage(title, QStringLiteral("当前仅支持 SurfaceMesh（对应 ParaView PolyData 的多边形表面）。"
                                      "体网格、结构网格或点集请先提取表面。"));
                return;
            }
            auto* panel = findChild<igQtConnectedSurfacePropertiesWidget*>(
                    QStringLiteral("ConnectedSurfacePropertiesWidget"),
                    Qt::FindDirectChildrenOnly);
            if (!panel) {
                panel = new igQtConnectedSurfacePropertiesWidget(
                        rendererWidget, modelTreeWidget, model, this);
                panel->setOutputReadyCallback([this](DataObject* output) {
                    ui->widget_SearchInfo->setCurrentModelData(output);
                    if (!output) return;
                    ui->widget_SearchInfo->setCurrentDataType(2);
                    ui->dockWidget_SearchInfo->show();
                    ui->dockWidget_SearchInfo->raise();
                });
            }
            panel->setSourceModel(model);
            if (!panel->isReady()) {
                showDarkFramelessMessage(title, QStringLiteral("当前仅支持 SurfaceMesh（对应 ParaView PolyData 的多边形表面）。"
                                      "体网格、结构网格或点集请先提取表面。"));
                return;
            }
            panel->show();
            panel->raise();
            panel->activateWindow();
            return;
        });
        return true;
    }
        if (filterId == QStringLiteral("append_datasets")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        auto scene = rendererWidget->GetScene();
        if (!scene) return;

        // Capture the tree selection before opening the parameter panel so Apply
        // cannot pick up unrelated scene models or previous results.
        const auto inputs = modelTreeWidget->GetSelectedDataObjects();
        if (inputs.empty()) {
            showDarkFramelessMessage(QStringLiteral("提示"),
                                     QStringLiteral("请先在模型树中选中要合并的模型。\n"
                                                    "按住 Ctrl 或 Shift 点击可多选。"));
            return;
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("Append Datasets"));
        dialog->setFilterDescription(QStringLiteral(
                "仅将打开面板时选中的网格合并为一个 UnstructuredMesh（保留原始单元类型）。\n"
                "按住 Ctrl 或 Shift 点击模型树可多选；单选时生成相同几何和属性的新模型，不合并重复点。\n"
                "默认不合并重复点（与 ParaView 一致）；需要消除接缝冗余顶点时勾选「合并重复顶点」，"
                "容差为 0 时仅合并完全重合的点。\n"
                "属性按交集规则保留：仅保留所有输入网格都具备的属性。"));
        const int mergeId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_CHECK_BOX,
                QStringLiteral("合并重复顶点 (Merge Points)"), "false");
        const int tolId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("合并容差"), "0");
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok = false;
            const bool mergePoints = dialog->getChecked(mergeId, ok);
            float tolerance = (float) dialog->getDouble(tolId, ok);
            if (!ok) tolerance = 0.0f; // 解析失败时回退到 ParaView 默认值 0（精确合并）

            auto filter = AppendDatasetsFilter::New();
            filter->SetMergePoints(inputs.size() > 1 && mergePoints);
            filter->SetTolerance(tolerance);

            for (const auto& input : inputs) {
                filter->AddInput(input);
            }
            const int count = static_cast<int>(inputs.size());

            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("执行失败"),
                                         QString::fromStdString(filter->GetMessage()));
                return;
            }

            auto outMesh = DynamicCast<UnstructuredMesh>(filter->GetOutput());
            if (!outMesh) {
                showDarkFramelessMessage(QStringLiteral("执行失败"),
                                         QStringLiteral("合并未生成有效的非结构化网格。"));
                return;
            }
            outMesh->SetName("append_datasets_result");

            modelTreeWidget->addDataObjectToModelTree(outMesh, Algorithm);

            // 自动激活第一个标量属性，便于立即看到颜色映射
            auto attrSet = outMesh->GetAttributeSet();
            if (attrSet) {
                int activeIdx = -1;
                for (int i = 0; i < (int) attrSet->GetNumberOfAttributes(); i++) {
                    auto& attr = attrSet->GetAttribute(i);
                    if (attr.IsNone() || attr.isDeleted || attr.type != IG_SCALAR) continue;
                    activeIdx = i;
                    break;
                }
                if (activeIdx >= 0) {
                    outMesh->ViewCloudPicture(rendererWidget->GetScene(), activeIdx, 0);
                }
            }
            rendererWidget->update();
            showDarkFramelessMessage(QStringLiteral("合并完成"),
                                     QStringLiteral("已合并 %1 个网格为单一 UnstructuredMesh。").arg(count),
                                     true);
            dialog->close();
        });
    });
            return true;
        }

        if (filterId == QStringLiteral("deflect_normals")) {
            connect(action, &QAction::triggered, this, [=, this]() {
        const QString title = QStringLiteral("法向偏转");
        auto input = currentFilterInput(title);
        if (!input) return;
        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(title);
        dialog->setFixedWidth(640);

        // 向量场所在位置（自动 / 点 / 单元）：显式指定，避免同名属性取错
        const int vfAttachId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_COMBO_BOX,
                QStringLiteral("向量场所在位置 (Vector Field Location)"),
                std::vector<QString>{QStringLiteral("自动 Auto (Point -> Cell 兜底)"),
                        QStringLiteral("点数据 Point Data"),
                        QStringLiteral("单元/面数据 Cell Data")});
        // 向量场下拉框（按位置联动筛选，显示带位置标记，userData 存纯名称）
        const int vecFieldId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_COMBO_BOX,
                QStringLiteral("向量场 (Vector Field)"), std::vector<QString>{QStringLiteral("(请选择)")});

        auto attrSet = input->GetAttributeSet();
        auto* vecCombo = dynamic_cast<QComboBox*>(dialog->getWidget(vecFieldId));
        auto refreshVectorFields = [vecCombo, attrSet](int locationFilter) {
            if (!vecCombo) return;
            vecCombo->clear();
            if (!attrSet) { vecCombo->addItem(QStringLiteral("(无可用向量属性)")); return; }
            bool found = false;
            for (size_t i = 0; i < attrSet->GetNumberOfAttributes(); ++i) {
                auto& attr = attrSet->GetAttribute(i);
                if (!attr.pointer || attr.isDeleted || attr.pointer->GetDimension() != 3) continue;
                if (locationFilter == 1 && attr.attachmentType != IG_POINT) continue;
                if (locationFilter == 2 && attr.attachmentType != IG_CELL) continue;
                QString name = QString::fromStdString(attr.pointer->GetName());
                QString tag = (attr.attachmentType == IG_POINT) ? QStringLiteral(" (Point)")
                                                               : QStringLiteral(" (Cell)");
                vecCombo->addItem(name + tag, name);
                found = true;
            }
            if (!found) vecCombo->addItem(QStringLiteral("(无可用向量属性)"));
        };
        refreshVectorFields(0);
        if (auto* vfCombo = dynamic_cast<QComboBox*>(dialog->getWidget(vfAttachId))) {
            connect(vfCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    refreshVectorFields);
        }

        const int strengthId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("偏转强度 (Scale Factor)"), "1.0");
        const int useUserNormalId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                QStringLiteral("使用自定义法向 (Use User Normal)"), "false");
        const int normalXId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("法向 X (Normal X)"), "0");
        const int normalYId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("法向 Y (Normal Y)"), "0");
        const int normalZId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("法向 Z (Normal Z)"), "1");

        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok{};
            auto combo = dynamic_cast<QComboBox*>(dialog->getWidget(vecFieldId));
            if (!combo || combo->currentText() == QStringLiteral("(无可用向量属性)") ||
                combo->currentText() == QStringLiteral("(请选择)")) {
                showDarkFramelessMessage(QStringLiteral("缺少向量场"),
                                         QStringLiteral("请先选择一个 3 分量的向量属性。"));
                return;
            }
            QString fieldName = combo->currentData().toString();
            if (fieldName.isEmpty()) fieldName = combo->currentText();
            const int attachIdx = dialog->getComboIndex(vfAttachId, ok);
            IGenum vfAttach = IG_NONE;
            if (attachIdx == 1) vfAttach = IG_POINT;
            else if (attachIdx == 2) vfAttach = IG_CELL;
            const double strength = dialog->getDouble(strengthId, ok);
            const bool useUserNormal = dialog->getChecked(useUserNormalId, ok);
            const double nx = dialog->getDouble(normalXId, ok);
            const double ny = dialog->getDouble(normalYId, ok);
            const double nz = dialog->getDouble(normalZId, ok);

            DeflectNormalsFilter::Pointer filter = DeflectNormalsFilter::New();
            filter->SetInput(input);
            filter->SetAttributeByName(fieldName.toStdString());
            filter->SetVectorFieldAttachment(vfAttach);
            filter->SetDeflectStrength(static_cast<float>(strength));
            filter->SetUseUserNormal(useUserNormal);
            filter->SetUserNormal(nx, ny, nz);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("执行出错"),
                                         QString::fromStdString(filter->GetMessage()));
                dialog->close();
                return;
            }
            // 界面明确显示基准法向来源
            QString baseSrc = QString::fromStdString(filter->GetBaseNormalSource());
            showDarkFramelessMessage(QStringLiteral("基准法向来源"),
                                     baseSrc.isEmpty() ? QStringLiteral("未知") : baseSrc);
            // 核心已生成独立输出，直接使用
            auto rawOut = filter->GetOutput();
            if (!rawOut) { dialog->close(); return; }
            // 模型节点名兜底：无名 → DeflectResult；有名 → 原名 + 后缀
            rawOut->SetName(rawOut->GetName().empty() ? std::string("DeflectResult")
                                                      : rawOut->GetName() + " (Deflect)");
            modelTreeWidget->addDataObjectToModelTree(rawOut, Algorithm);

            // 执行后提供“法向显示模式”切换框（偏转法向 / 几何法向）
            QDialog* displayDlg = new QDialog(this);
            displayDlg->setWindowTitle(QStringLiteral("法向显示模式 (Display Normal)"));
            displayDlg->setFixedWidth(440);
            auto* dlay = new QVBoxLayout(displayDlg);
            dlay->addWidget(new QLabel(QStringLiteral("显示法向 (Display Normal):"), displayDlg));
            auto* modeCombo = new QComboBox(displayDlg);
            modeCombo->addItems({QStringLiteral("使用偏转法向 (Deflected)"),
                                 QStringLiteral("使用几何法向 (Geometric)")});
            dlay->addWidget(modeCombo);

            auto applyDisplayNormal = [this, rawOut](int mode) {
                auto drawObj = DynamicCast<DrawObject>(rawOut);
                if (!drawObj) return;
                auto outAttrSet = rawOut->GetAttributeSet();
                int idx = -1;
                if (mode == 0) {
                    // 偏转法向：把 DeflectedNormals 作为渲染器实际使用的光照法向，并按它着色
                    if (outAttrSet) idx = outAttrSet->GetAttributeIndex("DeflectedNormals");
                    if (idx >= 0) {
                        auto nArr = DynamicCast<FloatArray>(outAttrSet->GetAttribute(idx).pointer);
                        if (nArr) drawObj->SetNormalSource("DeflectedNormals");
                    }
                } else {
                    // 几何法向：回退到几何法向，并按其着色
                    drawObj->SetNormalSource("");
                    if (outAttrSet) idx = outAttrSet->GetAttributeIndex("Normals");
                    if (idx < 0) idx = -1;
                }
                drawObj->ViewCloudPicture(rendererWidget->GetScene(), idx, 0);
                drawObj->ForceReConvertToDrawableData();
                rendererWidget->update();
            };
            connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    applyDisplayNormal);
            applyDisplayNormal(modeCombo->currentIndex());
            displayDlg->show();
            rendererWidget->update();
            dialog->close();
        });
    });
            return true;
        }

        if (filterId == QStringLiteral("extract_cells_by_region")) {
            connect(action, &QAction::triggered, this, [=, this]() {
        const QString title = QStringLiteral("按区域提取单元");
        auto input = currentFilterInput(title);
        if (!input) return;
        auto mesh = DynamicCast<UnstructuredMesh>(input);
        if (!mesh) {
            showDarkFramelessMessage(title, QStringLiteral("请选择非结构化网格。"));
            return;
        }
        const auto bounds = mesh->GetBoundingBox();
        const Vector3d center = (bounds.min + bounds.max) * 0.5;
        const Vector3d size = bounds.max - bounds.min;
        const double scale = std::max({size[0], size[1], size[2], 1.e-6});
        auto scene = rendererWidget->GetScene();
        auto interactor = scene ? scene->GetInteractor() : nullptr;
        if (!interactor) return;
        auto previousBox = interactor->GetSpecialInteractor("SelectBox");
        const bool previousHaveBox = iGame::SelectionParameter::Instance().GetHaveBox();
        const bool previousInSelection = iGame::SelectionParameter::Instance().GetInSelection();
        if (auto oldBox = DynamicCast<iGame::BoxStyle>(previousBox)) oldBox->ClearDraw();
        auto number = [](double value) { return QString::number(value, 'g', 12); };

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(title);
        dialog->setFilterDescription(QStringLiteral(
            "悬停高亮后按住<b>左键</b>拖动。<br>点击执行后提取单元。"));
        dialog->resize(340, 520);
        dialog->enablePreview(false);
        const int shape = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
            QStringLiteral("区域"), std::vector<QString>{QStringLiteral("Box"), QStringLiteral("Sphere"), QStringLiteral("Plane")});
        std::array<int, 3> position{}, lengths{}, normal{};
        const QString axes[] = {QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")};
        for (int i = 0; i < 3; ++i)
            position[i] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("中心/原点 ") + axes[i], number(center[i]));
        for (int i = 0; i < 3; ++i)
            lengths[i] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("尺寸 ") + axes[i], number(std::max(size[i] * .5, scale * .16)));
        const int radius = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
            QStringLiteral("球半径"), number(scale * .25));
        for (int i = 0; i < 3; ++i)
            normal[i] = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("平面法向 ") + axes[i], i == 0 ? "1" : "0");
        const int strict = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
            QStringLiteral("全部顶点在内"), "true");
        auto* countLabel = new QLabel(QStringLiteral("调整区域后点击执行"), dialog);
        dialog->addRowWidget(countLabel);
        auto* planeButtons = new QWidget(dialog);
        auto* planeLayout = new QHBoxLayout(planeButtons);
        planeLayout->setContentsMargins(0, 0, 0, 0);
        std::array<QPushButton*, 3> axisButtons{};
        for (int i = 0; i < 3; ++i) {
            axisButtons[i] = new QPushButton(axes[i], planeButtons);
            axisButtons[i]->setToolTip(QStringLiteral("法向 ") + axes[i]);
            planeLayout->addWidget(axisButtons[i]);
        }
        auto* flipButton = new QPushButton(QStringLiteral("翻转法向"), planeButtons);
        planeLayout->addWidget(flipButton);
        dialog->addRowWidget(planeButtons);

        struct RegionSession {
            iGame::BoxStyle::Pointer box;
            iGame::RadialStyle::Pointer sphere;
            iGame::RegionPlaneStyle::Pointer plane;
            DataObject::Pointer output;
            bool closed = false;
            int activeMode = -1;
        };
        auto session = std::make_shared<RegionSession>();
        session->box = iGame::BoxStyle::New();
        session->box->Initialize(interactor);
        session->box->SetLeftButtonOnly(true);
        session->sphere = iGame::RadialStyle::New();
        session->sphere->Initialize(interactor);
        session->sphere->SetMoveEndWithStart(true);
        session->sphere->SetShowSphere(true);
        session->sphere->SetCenterTranslationMode(true);
        session->sphere->SetAllowBoundaryDrag(true);
        session->sphere->SetParaViewInteraction(true);
        session->sphere->SetLeftButtonOnly(true);
        session->sphere->SetAllowRegionResize(true, std::max(scale * 1.e-6, 1.e-7));
        session->plane = iGame::RegionPlaneStyle::New();
        session->plane->Initialize(interactor);
        session->plane->SetBounds(bounds);
        QPointer<igQtFilterDialogDockWidget> safeDialog(dialog);
        auto line = [dialog](int id) { return qobject_cast<QLineEdit*>(dialog->getWidget(id)); };
        auto dirty = [=]() {
            if (!session->closed && safeDialog) countLabel->setText(QStringLiteral("参数已修改，点击执行更新结果"));
        };
        auto readVector = [=](const std::array<int, 3>& ids, Vector3d& value) {
            for (int i = 0; i < 3; ++i) {
                bool ok = false;
                value[i] = dialog->getDouble(ids[i], ok);
                if (!ok || !std::isfinite(value[i])) return false;
            }
            return true;
        };
        auto writeVector = [=](const std::array<int, 3>& ids, const iGame::Point& value) {
            for (int i = 0; i < 3; ++i) line(ids[i])->setText(number(value[i]));
        };
        auto updateWidget = [=]() -> bool {
            if (session->closed || !safeDialog) return false;
            bool ok = false;
            const int mode = dialog->getComboIndex(shape, ok);
            Vector3d p, v;
            if (!ok || !readVector(position, p)) return false;
            if (mode == 0) {
                if (!readVector(lengths, v) || v[0] <= 0. || v[1] <= 0. || v[2] <= 0.) return false;
                const auto rotation = session->box->GetBox() ? session->box->GetBox()->GetRotationMatrix() : igm::mat4(1.f);
                session->box->InitBox(iGame::Point(p[0] - v[0] * .5, p[1] - v[1] * .5, p[2] - v[2] * .5),
                                      iGame::Point(p[0] + v[0] * .5, p[1] + v[1] * .5, p[2] + v[2] * .5));
                session->box->GetBox()->SetRotation(rotation);
                session->box->ClearDraw();
                session->box->ToDraw();
            } else if (mode == 1) {
                const double r = dialog->getDouble(radius, ok);
                if (!ok || !std::isfinite(r) || r <= 0.) return false;
                session->sphere->SetPoint(iGame::Point(p[0], p[1], p[2]), iGame::Point(p[0] + r, p[1], p[2]));
            } else if (mode == 2) {
                if (!readVector(normal, v) || !std::isfinite(v.squaredNorm()) || v.squaredNorm() < 1.e-24) return false;
                session->plane->SetPlane(iGame::Point(p[0], p[1], p[2]), iGame::Point(v[0], v[1], v[2]));
            } else return false;
            rendererWidget->update();
            return true;
        };
        auto switchShape = [=]() {
            if (session->closed || !safeDialog) return;
            bool ok = false;
            const int mode = dialog->getComboIndex(shape, ok);
            if (!ok) return;
            const QString gestures[] = {
                QStringLiteral("拖边平移；拖红点缩放；拖面旋转。"),
                QStringLiteral("拖中心或线框平移；拖红点改半径。"),
                QStringLiteral("拖中心平移；拖端点或圆环旋转。<br>拖平面沿法向移动，保留法向负侧。")};
            dialog->setFilterDescription(QStringLiteral("悬停高亮后按住<b>左键</b>拖动。<br>") + gestures[mode] +
                QStringLiteral("<br>空白处旋转模型；点击执行后提取。"));
            session->box->ClearDraw();
            session->sphere->SetShow(false);
            session->plane->SetShow(false);
            interactor->RemoveSepcialInteractor("SelectBox");
            interactor->RemoveSepcialInteractor("ExtractCellsByRegionSphere");
            interactor->RemoveSepcialInteractor("ExtractCellsByRegionPlane");
            iGame::SelectionParameter::Instance().SetHaveBox(mode == 0);
            for (int id : lengths) dialog->setParameterVisible(id, mode == 0);
            dialog->setParameterVisible(radius, mode == 1);
            for (int id : normal) dialog->setParameterVisible(id, mode == 2);
            planeButtons->setVisible(mode == 2);
            if (mode == 0) interactor->_SetSpecialInteractor("SelectBox", session->box);
            else if (mode == 1) interactor->_SetSpecialInteractor("ExtractCellsByRegionSphere", session->sphere);
            else interactor->_SetSpecialInteractor("ExtractCellsByRegionPlane", session->plane);
            session->activeMode = mode;
            updateWidget();
            if (mode == 1) session->sphere->SetShow(true);
            if (mode == 2) session->plane->SetShow(true);
            dirty();
            rendererWidget->update();
        };
        session->box->_SetPointMoveCallBack("ExtractCellsByRegion", [=]() {
            if (session->closed || !safeDialog || !session->box->GetBox()) return;
            writeVector(position, session->box->GetBox()->GetMidPoint());
            writeVector(lengths, session->box->GetBox()->GetLength());
            dirty();
            rendererWidget->update();
        });
        session->sphere->_SetPointMoveCallBack("ExtractCellsByRegion", [=]() {
            if (session->closed || !safeDialog) return;
            writeVector(position, session->sphere->GetStartPoint());
            line(radius)->setText(number((session->sphere->GetEndPoint() - session->sphere->GetStartPoint()).length()));
            dirty();
            rendererWidget->update();
        });
        session->plane->SetChangedCallback([=]() {
            if (session->closed || !safeDialog) return;
            writeVector(position, session->plane->GetOrigin());
            writeVector(normal, session->plane->GetNormal());
            dirty();
            rendererWidget->update();
        });
        dialog->setApplyFunctor([=, this]() {
            if (!updateWidget()) {
                showDarkFramelessMessage(title, QStringLiteral("请填写有效的区域参数：尺寸和半径须大于零，法向须非零。"));
                return;
            }
            auto filter = ExtractCellsByRegionFilter::New();
            filter->SetInput(input);
            if (session->activeMode == 0) {
                const auto box = session->box->GetBox();
                const auto& p = box->GetMidPoint();
                const auto& length = box->GetLength();
                const auto& rotation = box->GetRotationMatrix();
                std::array<Vector3d, 3> boxAxes;
                for (int i = 0; i < 3; ++i) boxAxes[i] = Vector3d(rotation[i][0], rotation[i][1], rotation[i][2]);
                filter->SetOrientedBox(Vector3d(p[0], p[1], p[2]),
                    Vector3d(length[0] * .5, length[1] * .5, length[2] * .5), boxAxes);
            } else if (session->activeMode == 1) {
                const auto& p = session->sphere->GetStartPoint();
                filter->SetSphere(Vector3d(p[0], p[1], p[2]), (session->sphere->GetEndPoint() - p).length());
            } else {
                const auto& p = session->plane->GetOrigin();
                const auto& n = session->plane->GetNormal();
                filter->SetPlane(Vector3d(p[0], p[1], p[2]), Vector3d(n[0], n[1], n[2]));
            }
            bool ok = false;
            filter->SetRequireAllPoints(dialog->getChecked(strict, ok));
            if (!filter->Execute()) { showDarkFramelessMessage(title, QStringLiteral("区域提取失败。")); return; }
            if (filter->GetSelectedCellIds().empty()) {
                countLabel->setText(QStringLiteral("当前区域没有单元；未更新结果。"));
                return;
            }
            auto output = filter->GetOutput();
            output->SetName(input->GetName() + "_ExtractCellsByRegion");
            auto* resultItem = session->output ? modelTreeWidget->getItemFromObject(session->output) : nullptr;
            if (resultItem) {
                auto* resultModel = resultItem->getModel();
                resultModel->SetDataObject(output);
                scene->SetCurrentModel(resultModel);
                modelTreeWidget->updateAllAttriubute(output);
                modelTreeWidget->setCurrentItem(resultItem);
                modelTreeWidget->updateCurrentModelInfo();
                scene->Update();
                rendererWidget->update();
            } else modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
            session->output = output;
            countLabel->setText(QStringLiteral("已提取：%1 个单元").arg(filter->GetSelectedCellIds().size()));
        });
        dialog->setCloseFunctor([=]() {
            session->closed = true;
            session->box->RemovePointMoveCallBack("ExtractCellsByRegion");
            session->sphere->RemovePointMoveCallBack("ExtractCellsByRegion");
            session->plane->SetChangedCallback({});
            session->box->ClearDraw();
            session->sphere->SetShow(false);
            session->plane->SetShow(false);
            interactor->RemoveSepcialInteractor("ExtractCellsByRegionSphere");
            interactor->RemoveSepcialInteractor("ExtractCellsByRegionPlane");
            interactor->RemoveSepcialInteractor("SelectBox");
            if (previousBox) interactor->_SetSpecialInteractor("SelectBox", previousBox);
            iGame::SelectionParameter::Instance().SetHaveBox(previousHaveBox);
            iGame::SelectionParameter::Instance().SetInSelection(previousInSelection);
            if (auto oldBox = DynamicCast<iGame::BoxStyle>(previousBox)) if (oldBox->GetBox()) oldBox->ToDraw();
            rendererWidget->update();
        });
        iGame::SelectionParameter::Instance().SetInSelection(false);
        for (const auto& ids : {position, lengths, normal}) for (int id : ids)
            connect(line(id), &QLineEdit::editingFinished, dialog, [=]() { updateWidget(); dirty(); });
        connect(line(radius), &QLineEdit::editingFinished, dialog, [=]() { updateWidget(); dirty(); });
        connect(qobject_cast<QComboBox*>(dialog->getWidget(shape)), QOverload<int>::of(&QComboBox::currentIndexChanged),
            dialog, [=](int) { switchShape(); });
        connect(qobject_cast<QCheckBox*>(dialog->getWidget(strict)), &QCheckBox::toggled, dialog, [=](bool) { dirty(); });
        for (int axis = 0; axis < 3; ++axis) connect(axisButtons[axis], &QPushButton::clicked, dialog, [=]() {
            for (int i = 0; i < 3; ++i) line(normal[i])->setText(i == axis ? "1" : "0");
            updateWidget(); dirty();
        });
        connect(flipButton, &QPushButton::clicked, dialog, [=]() {
            Vector3d n;
            if (!readVector(normal, n)) return;
            for (int i = 0; i < 3; ++i) line(normal[i])->setText(number(-n[i]));
            updateWidget(); dirty();
        });
        switchShape();
        dialog->show();
        QTimer::singleShot(0, dialog, [this, dialog]() {
            const int x = std::max(8, rendererWidget->width() - dialog->width() - 16);
            dialog->move(rendererWidget->mapToGlobal(QPoint(x, 16)));
        });
        dialog->raise();
        dialog->activateWindow();
    });
            return true;
        }

        if (filterId == QStringLiteral("extract_time_steps")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                if (!m_extractTimeStepsWidget) m_extractTimeStepsWidget = new igQtExtractTimeStepsWidget(this);
                // Own the result connection here as well, so this entry works without
                // development-menu setup. Replace its equivalent connection to avoid duplicates.
                disconnect(m_extractTimeStepsWidget,
                        &igQtExtractTimeStepsWidget::ExtractTimeStepsApplied, this, nullptr);
                connect(m_extractTimeStepsWidget,
                        &igQtExtractTimeStepsWidget::ExtractTimeStepsApplied, this,
                        [this](iGame::DataObject::Pointer output) {
                    if (!output) return;
                    modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);
                    rendererWidget->update();
                });


                auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("保留指定时间步"), QStringLiteral("当前没有模型"));
                    return;
                }

                auto object = currentModel->GetDataObject();
                auto frames = object ? object->PeekTimeFrames() : nullptr;
                if (!frames || frames->GetTimeNum() == 0) {
                    showDarkFramelessMessage(
                            QStringLiteral("保留指定时间步"),
                            QStringLiteral("当前模型没有时间序列。请选择带时间序列的模型（例如打开的 .pvd 序列）。"));
                    return;
                }

                openLeftToolWidget("extractTimeSteps", m_extractTimeStepsWidget, QStringLiteral("提取时间步"));
                m_extractTimeStepsWidget->SetOriginDataObject(object);
            });
            return true;
        }

        if (filterId == QStringLiteral("histogram")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
                if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("提示"), QStringLiteral("请先加载一个模型"));
                    return;
                }
                auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
                if (!obj) return;
                auto attributeSet = obj->GetAttributeSet();
                auto attributes = attributeSet ? attributeSet->GetAllAttributes() : nullptr;
                if (!attributes || attributes->GetNumberOfElements() == 0) {
                    showDarkFramelessMessage(QStringLiteral("提示"),
                                             QStringLiteral("当前模型没有属性数据，无法统计取值分布"));
                    return;
                }

                closeHistogramParameters();
                auto* dialog = new igQtFilterDialogDockWidget(this);
                m_histogramParametersDock = dialog;
                m_histogramParameterContents = dialog->widget();
                dialog->setObjectName(QStringLiteral("HistogramParametersDock"));
                dialog->setAllowedAreas(Qt::LeftDockWidgetArea);
                dialog->setFloating(false);
                dialog->setFeatures(QDockWidget::DockWidgetClosable);
                // Restore the content to its shell before deleting the parameter dialog.
                auto* closeButton = dialog->findChild<QPushButton*>(QStringLiteral("closeButton"));
                disconnect(closeButton, &QPushButton::clicked, dialog, &igQtFilterDialogDockWidget::close);
                connect(closeButton, &QPushButton::clicked, this, &igQtMainWindow::closeHistogramParameters);
                dialog->setFilterTitle(QStringLiteral("属性直方图 (Histogram)"));
                dialog->setFilterDescription(
                        QStringLiteral("统计所选属性数组的取值分布；参数与 ParaView 的 Histogram 过滤器一一对应"));

                // 下拉框条目顺序与属性下标可能因空槽而错位，这里显式保存"条目序号 -> 属性下标"的映射
                std::vector<QString> arrNames;
                std::vector<int> arrIndexes;
                for (igIndex a = 0; a < attributes->GetNumberOfElements(); ++a) {
                    auto& at = attributes->GetElement(a);
                    if (at.isDeleted || at.pointer == nullptr) continue;
                    arrNames.push_back(QStringLiteral("%1: %2")
                                               .arg(at.attachmentType == IG_POINT ? QStringLiteral("点")
                                                                                  : QStringLiteral("单元"))
                                               .arg(QString::fromStdString(at.pointer->GetName())));
                    arrIndexes.push_back(static_cast<int>(a));
                }
                int arrayId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                                                   QStringLiteral("数组选择"), arrNames);
                // 与 ParaView 一致：面板默认选中「当前正在显示的数组」（DataObject::GetAttributeIndex()），
                // 而不是属性集里的第一个数组。否则属性集首项是单元数组时（例如 c 在 v 前面），
                // 默认统计/绘图的就是那个数组。
                if (auto* arrayCombo = dynamic_cast<QComboBox*>(dialog->getWidget(arrayId)); arrayCombo != nullptr) {
                    const int currentAttribute = obj->GetAttributeIndex();
                    for (size_t row = 0; row < arrIndexes.size(); ++row) {
                        if (arrIndexes[row] == currentAttribute) {
                            arrayCombo->setCurrentIndex(static_cast<int>(row));
                            break;
                        }
                    }
                }
                int binsId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                  QStringLiteral("分箱数"), "10");
                int componentId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                       QStringLiteral("分量"), "0");
                int centerId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                    QStringLiteral("居中分箱"), "false");
                int useRangeId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                      QStringLiteral("自定义范围"), "false");
                int rangeMinId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("范围下限"), "0");
                int rangeMaxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                      QStringLiteral("范围上限"), "1");
                int normalizeId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                       QStringLiteral("归一化"), "false");
                int averagesId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                                      QStringLiteral("计算均值"), "false");
                dialog->setWidget(nullptr);
                dialog->hide();
                openLeftToolWidget("histogramParameters", m_histogramParameterContents, QStringLiteral("直方图参数"));
                resizeDocks({m_leftFieldDock}, {420}, Qt::Horizontal);

                dialog->setApplyFunctor([=, this, previousHistogram = HistogramData::Pointer{}]() mutable {
                    bool okArray = false;
                    int arrayIdx = dialog->getComboIndex(arrayId, okArray);
                    bool okBins = false;
                    int bins = dialog->getInt(binsId, okBins);
                    bool okComponent = false;
                    int component = dialog->getInt(componentId, okComponent);
                    bool okCenter = false, okUseRange = false, okRangeMin = false, okRangeMax = false;
                    bool okNormalize = false, okAverages = false;
                    const bool center = dialog->getChecked(centerId, okCenter);
                    const bool useRange = dialog->getChecked(useRangeId, okUseRange);
                    const double rangeMin = dialog->getDouble(rangeMinId, okRangeMin);
                    const double rangeMax = dialog->getDouble(rangeMaxId, okRangeMax);
                    const bool normalize = dialog->getChecked(normalizeId, okNormalize);
                    const bool calculateAverages = dialog->getChecked(averagesId, okAverages);
                    if (!okArray || !okBins || arrayIdx < 0 || arrayIdx >= static_cast<int>(arrIndexes.size())) {
                        showDarkFramelessMessage(QStringLiteral("提示"), QStringLiteral("请选择数组并填写分箱数"));
                        return;
                    }
                    if (bins < 1 || bins > 10000) {
                        showDarkFramelessMessage(QStringLiteral("提示"),
                                                 QStringLiteral("分箱数请填 1 ~ 10000 之间的整数"));
                        return;
                    }
                    if (!okComponent || component < 0) {
                        showDarkFramelessMessage(QStringLiteral("提示"),
                                                 QStringLiteral("分量请填不小于 0 的整数（等于数组维数时按模长统计）"));
                        return;
                    }
                    if (useRange && (!okRangeMin || !okRangeMax)) {
                        showDarkFramelessMessage(QStringLiteral("提示"),
                                                 QStringLiteral("自定义范围请填写范围下限与范围上限"));
                        return;
                    }
                    auto filter = HistogramFilter::New();
                    filter->SetInput(obj);
                    filter->SetAttributeIndex(arrIndexes[arrayIdx]);
                    filter->SetNumberOfBins(bins);
                    filter->SetComponent(component);
                    filter->SetCenterBinsAroundMinAndMax(center);
                    if (useRange) {
                        filter->SetUseCustomBinRanges(true);
                        filter->SetCustomBinRanges(rangeMin, rangeMax);
                    }
                    filter->SetNormalize(normalize);
                    filter->SetCalculateAverages(calculateAverages);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(QStringLiteral("警告"), QString::fromStdString(filter->GetMessage()));
                        return;
                    }
                    auto histogram = filter->GetHistogramData();
                    // Input and Histogram outputs share one chart with independent representations.
                    if (m_histogramDock == nullptr) {
                        m_histogramChartWidget = new igQtHistogramChartWidget(this);
                        m_histogramDock = new QDockWidget(this);
                        m_histogramDock->setObjectName(QStringLiteral("dockWidget_Histogram"));
                        m_histogramDock->setWidget(m_histogramChartWidget);
                        m_histogramDock->setAllowedAreas(Qt::RightDockWidgetArea);
                        m_histogramDock->setFeatures(QDockWidget::DockWidgetClosable |
                                                     QDockWidget::DockWidgetMovable |
                                                     QDockWidget::DockWidgetFloatable);
                        this->addDockWidget(Qt::RightDockWidgetArea, m_histogramDock);
                        resizeDocks({m_histogramDock}, {640}, Qt::Horizontal);
                        m_histogramSeriesDock = new QDockWidget(QStringLiteral("直方图系列"), this);
                        m_histogramSeriesDock->setObjectName(QStringLiteral("dockWidget_HistogramSeries"));
                        m_histogramSeriesDock->setAllowedAreas(Qt::LeftDockWidgetArea);
                        m_histogramSeriesDock->setFeatures(QDockWidget::DockWidgetClosable);
                        m_histogramSeriesDock->setWidget(m_histogramChartWidget->settingsWidget());
                        connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged,
                                m_histogramChartWidget, [this]() {
                                    auto model = rendererWidget->GetScene()->GetCurrentModel();
                                    if (model) m_histogramChartWidget->SelectSource(model->GetDataObject().get());
                                });
                    }
                    m_histogramDock->setWindowTitle(
                            QStringLiteral("属性直方图 — %1").arg(QString::fromStdString(obj->GetName())));
                    m_histogramChartWidget->SetHistogram(obj, histogram, arrIndexes[arrayIdx], component,
                                                         previousHistogram.get());
                    previousHistogram = histogram;
                    if (m_histogramDock->isHidden()) { m_histogramDock->show(); }
                    m_histogramDock->raise();
                    openLeftToolWidget("histogramSeries", m_histogramChartWidget->settingsWidget(), QStringLiteral("直方图系列"));
                    closeHistogramParameters();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("overlapping_cells_detector")) {
            connect(action, &QAction::triggered, this, [this]() {
        auto scene = rendererWidget->GetScene();
        if (scene == nullptr || scene->GetCurrentModel() == nullptr) return;

        const auto model = scene->GetCurrentModel();
        const auto sourceModelId = scene->GetCurrentModelID();
        const auto dataObject = model == nullptr ? nullptr : model->GetDataObject();
        if (model == nullptr || dataObject == nullptr ||
            (DynamicCast<UnstructuredMesh>(dataObject).IsNull() && DynamicCast<VolumeMesh>(dataObject).IsNull() && DynamicCast<SurfaceMesh>(dataObject).IsNull())) {
            showDarkFramelessMessage(
                    QStringLiteral("检测重叠单元"),
                    QStringLiteral("请选择三角形或平面凸四边形表面网格，或包含线性体单元的网格。"));
            return;
        }

        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("检测重叠单元"));
        dialog->setFilterDescription(QStringLiteral(
                "检测三角形、平面凸四边形的内部相交，或四面体、六面体、三棱柱、金字塔的共同体积。<br>"
                "执行后生成单元标量 <b>NumberOfOverlapsPerCell</b>；值大于 0 的单元以选中边线高亮显示。<br>"
                "正常邻接不计为重叠；凹或扭曲四边形、高阶、多面体、复合数据及二维/三维混合网格暂不支持。"));
        const int toleranceId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("公差 (Tolerance)"), "0.0");
        dialog->show();
        dialog->setApplyFunctor([this, dialog, model, scene, sourceModelId, toleranceId]() {
            bool toleranceOk = false;
            const double tolerance = dialog->getDouble(toleranceId, toleranceOk);
            if (!toleranceOk || tolerance < 0.0) {
                showDarkFramelessMessage(QStringLiteral("检测重叠单元"), QStringLiteral("公差必须是非负数。"));
                return;
            }
            if (model == nullptr) return;

            auto filter = OverlappingCellsDetectorFilter::New();
            filter->SetInput(model->GetDataObject());
            filter->SetTolerance(tolerance);
            if (!filter->Execute()) {
                const auto& error = filter->GetLastError();
                showDarkFramelessMessage(
                        QStringLiteral("检测重叠单元"),
                        QStringLiteral("执行失败：%1")
                                .arg(error.empty() ? QStringLiteral("未知错误。") : QString::fromStdString(error)));
                return;
            }

            // Filter 返回独立网格；原模型及其属性保持不变。
            auto data = filter->GetOutput();
            if (data == nullptr) {
                showDarkFramelessMessage(QStringLiteral("检测重叠单元"),
                                         QStringLiteral("执行失败：未生成有效输出。"));
                return;
            }
            const auto outputModelId = modelTreeWidget->addDataObjectToModelTree(
                    data, ItemSource::Algorithm);
            auto outputModel = scene->GetModelById(outputModelId);
            const int attributeIndex = data->GetAttributeSet()->GetAttributeIndex(
                    OverlappingCellsDetectorFilter::NumberOfOverlapsPerCellArrayName());
            auto item = modelTreeWidget->getItemFromObject(data);
            if (item != nullptr && attributeIndex >= 0 && attributeIndex < item->childCount()) {
                item->setExpanded(true);
                auto* child = item->child(attributeIndex);
                item->setCurrentChild(child);
                item->setSelected(false);
                item->viewAttribute(attributeIndex, 0);
                child->setSelected(true);
                modelTreeWidget->setCurrentItem(child);
            }

            std::vector<igIndex> overlappingCellIds;
            const auto& overlapCounts = filter->GetNumberOfOverlapsPerCell();
            for (igIndex cellId = 0; cellId < overlapCounts.size(); ++cellId) {
                if (overlapCounts[cellId] > 0) overlappingCellIds.push_back(cellId);
            }
            QStringList overlapCountPreview;
            constexpr int kPreviewCount = 20;
            for (int cellId = 0; cellId < static_cast<int>(overlapCounts.size()) && cellId < kPreviewCount; ++cellId) {
                overlapCountPreview.push_back(QString::number(overlapCounts[cellId]));
            }
            const QString countText =
                    overlapCounts.size() <= kPreviewCount
                            ? QStringLiteral("[%1]").arg(overlapCountPreview.join(QStringLiteral(", ")))
                            : QStringLiteral("[%1, ...]").arg(overlapCountPreview.join(QStringLiteral(", ")));
            auto selection = outputModel == nullptr ? nullptr : outputModel->GetSelection();
            if (selection != nullptr) {
                selection->Reset();
                if (!overlappingCellIds.empty()) {
                    selection->SelectionCallBackEvent(IG_CELL, overlappingCellIds, Selection::Operate::Add);
                    selection->SetSelectItemVisable(true);
                }
            }
            if (outputModel != nullptr) {
                model->SetVisibility(false);
                outputModel->SetVisibility(true);
                scene->SetCurrentModel(outputModel);

                // 该结果与输入节点采用互斥显示：选择输入时恢复原始外观，
                // 选择结果时显示标量和重叠高亮，避免结果轮廓覆盖原模型。
                const bool hasOverlaps = !overlappingCellIds.empty();
                connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this,
                        [this, scene, sourceModelId, outputModelId, hasOverlaps]() {
                            auto sourceModel = scene->GetModelById(sourceModelId);
                            auto resultModel = scene->GetModelById(outputModelId);
                            if (resultModel == nullptr) return;

                            const auto currentModelId = scene->GetCurrentModelID();
                            const bool resultSelected = currentModelId == outputModelId;
                            resultModel->SetVisibility(resultSelected);
                            if (auto resultSelection = resultModel->GetSelection())
                                resultSelection->SetSelectItemVisable(resultSelected && hasOverlaps);

                            if (sourceModel != nullptr &&
                                (currentModelId == sourceModelId || resultSelected))
                                sourceModel->SetVisibility(!resultSelected);
                            rendererWidget->update();
                        });
            }
            rendererWidget->update();

            showDarkFramelessMessage(QStringLiteral("检测重叠单元"),
                                     QStringLiteral("检测完成：发现 %1 对重叠单元；高亮 %2 个单元。<br>"
                                                    "NumberOfOverlapsPerCell = %3")
                                             .arg(static_cast<qulonglong>(filter->GetOverlappingCellPairs().size()))
                                             .arg(static_cast<qulonglong>(overlappingCellIds.size()))
                                             .arg(countText));
            dialog->close();
        });
    });
            return true;
        }

        if (filterId == QStringLiteral("point_plane_interpolator")) {
            connect(action, &QAction::triggered, this, [this](bool) {
        auto* scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto input = model ? model->GetDataObject() : nullptr;
        if (!input || !DynamicCast<PointSet>(input)) {
            showDarkFramelessMessage(QStringLiteral("点平面插值"), QStringLiteral("请先加载并选择一个点集或网格。"));
            return;
        }

        // 如果已经存在，彻底销毁旧的，防止底层手柄重叠和状态污染
        for (int i = 0; i < m_leftFieldTabs->count(); ++i) {
            if (m_leftFieldTabs->tabText(i) == QStringLiteral("点平面插值")) {
                QWidget* oldPage = m_leftFieldTabs->widget(i);
                m_leftFieldTabs->removeTab(i);
                delete oldPage;
            }
        }

        // 1. 创建 Tab 容器及滚动区域
        QWidget* page = new QWidget();
        QVBoxLayout* pageLayout = new QVBoxLayout(page);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->setSpacing(0);

        QScrollArea* scroll = new QScrollArea(page);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet("QScrollArea { background: transparent; } QScrollArea > QWidget > QWidget { background: "
                              "transparent; }");

        QWidget* body = new QWidget(scroll);

        // 统一使用基础 QSS 并挂载主题自适应
        QString baseQss =
                "QWidget { background-color: transparent; color: #EAEAEA; font-size: 13px; }"
                "QLabel { color: #D8D8D8; background: transparent; font-size: 13px; padding-top: 4px; }"
                "QLineEdit, QComboBox, QDoubleSpinBox, QSpinBox { background-color: #2A2A2A; color: #EAEAEA; "
                "border: 1px solid #3A3A3A; padding: 3px 6px; border-radius: 3px; font-size: 13px; min-height: 20px; }"
                "QComboBox::drop-down { border: none; background: transparent; width: 20px; }"
                "QComboBox QAbstractItemView { background-color: #2A2A2A; color: #EAEAEA; selection-background-color: "
                "#3A3A3A; }"
                "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width: 16px; border: none; background: "
                "transparent; }"
                "QPushButton { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; padding: 6px "
                "12px; border-radius: 4px; font-size: 13px; }"
                "QPushButton:hover { background-color: #3A3A3A; }"
                "QGroupBox { border: 1px solid #3A3A3A; border-radius: 4px; margin-top: 8px; padding-top: 12px; }"
                "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 5px; color: "
                "#D8D8D8; }";

        body->setStyleSheet(baseQss);
        igQtPanelTheme::attach(body);

        QVBoxLayout* mainLayout = new QVBoxLayout(body);
        mainLayout->setContentsMargins(10, 10, 10, 10);
        mainLayout->setSpacing(10);

        QLabel* inputLabel = new QLabel(QStringLiteral("输入：") + QString::fromStdString(input->GetName()));
        inputLabel->setStyleSheet("color: #FFFFFF; font-weight: bold;");
        inputLabel->setProperty("igPanelBaseQss", inputLabel->styleSheet());
        igQtPanelTheme::refresh(inputLabel);
        mainLayout->addWidget(inputLabel);

        auto bbox = input->GetBoundingBox();
        double cx = (bbox.min[0] + bbox.max[0]) * 0.5;
        double cy = (bbox.min[1] + bbox.max[1]) * 0.5;
        double cz = (bbox.min[2] + bbox.max[2]) * 0.5;

        // ================= 2. 平面参数分组 =================
        QGroupBox* groupPlane = new QGroupBox(QStringLiteral("平面参数"), body);
        QFormLayout* formPlane = new QFormLayout(groupPlane);
        formPlane->setSpacing(6);

        QDoubleSpinBox* originX = new QDoubleSpinBox(groupPlane);
        originX->setRange(-1e9, 1e9);
        originX->setValue(cx);
        originX->setDecimals(4);
        QDoubleSpinBox* originY = new QDoubleSpinBox(groupPlane);
        originY->setRange(-1e9, 1e9);
        originY->setValue(cy);
        originY->setDecimals(4);
        QDoubleSpinBox* originZ = new QDoubleSpinBox(groupPlane);
        originZ->setRange(-1e9, 1e9);
        originZ->setValue(cz);
        originZ->setDecimals(4);

        QDoubleSpinBox* normalX = new QDoubleSpinBox(groupPlane);
        normalX->setRange(-1, 1);
        normalX->setValue(1.0);
        normalX->setDecimals(4);
        QDoubleSpinBox* normalY = new QDoubleSpinBox(groupPlane);
        normalY->setRange(-1, 1);
        normalY->setValue(0.0);
        normalY->setDecimals(4);
        QDoubleSpinBox* normalZ = new QDoubleSpinBox(groupPlane);
        normalZ->setRange(-1, 1);
        normalZ->setValue(0.0);
        normalZ->setDecimals(4);

        QSignalBlocker b1(originX), b2(originY), b3(originZ);
        QSignalBlocker b4(normalX), b5(normalY), b6(normalZ);

        formPlane->addRow(QStringLiteral("原点 X："), originX);
        formPlane->addRow(QStringLiteral("原点 Y："), originY);
        formPlane->addRow(QStringLiteral("原点 Z："), originZ);
        formPlane->addRow(QStringLiteral("法向 X："), normalX);
        formPlane->addRow(QStringLiteral("法向 Y："), normalY);
        formPlane->addRow(QStringLiteral("法向 Z："), normalZ);
        mainLayout->addWidget(groupPlane);

        // ================= 3. 插值设置分组 =================
        QGroupBox* groupInterp = new QGroupBox(QStringLiteral("插值设置"), body);
        QFormLayout* formInterp = new QFormLayout(groupInterp);
        formInterp->setSpacing(6);

        QSpinBox* res = new QSpinBox(groupInterp);
        res->setRange(2, 1024);
        res->setValue(100);
        formInterp->addRow(QStringLiteral("分辨率："), res);

        QComboBox* kernel = new QComboBox(groupInterp);
        kernel->addItems({QStringLiteral("Voronoi"), QStringLiteral("Gaussian"), QStringLiteral("Shepard")});
        formInterp->addRow(QStringLiteral("插值核："), kernel);

        QComboBox* footprint = new QComboBox(groupInterp);
        footprint->addItems({QStringLiteral("Radius"), QStringLiteral("N Closest")});
        formInterp->addRow(QStringLiteral("邻域搜索："), footprint);

        QDoubleSpinBox* radius = new QDoubleSpinBox(groupInterp);
        radius->setRange(0.0001, 1e6);
        radius->setValue(std::max(0.001, static_cast<double>(bbox.diag()) / 10));
        formInterp->addRow(QStringLiteral("半径："), radius);

        QSpinBox* nClosest = new QSpinBox(groupInterp);
        nClosest->setRange(1, 1000);
        nClosest->setValue(8);
        formInterp->addRow(QStringLiteral("最近邻点数："), nClosest);

        QDoubleSpinBox* sharpness = new QDoubleSpinBox(groupInterp);
        sharpness->setRange(0.0001, 100);
        sharpness->setValue(2.0);
        formInterp->addRow(QStringLiteral("高斯锐度："), sharpness);

        QDoubleSpinBox* power = new QDoubleSpinBox(groupInterp);
        power->setRange(0.0001, 100);
        power->setValue(2.0);
        formInterp->addRow(QStringLiteral("Shepard幂次："), power);

        QComboBox* nullStrategy = new QComboBox(groupInterp);
        nullStrategy->addItems({QStringLiteral("掩码"), QStringLiteral("空值"), QStringLiteral("最近点")});
        nullStrategy->setCurrentIndex(2);
        formInterp->addRow(QStringLiteral("空邻域："), nullStrategy);
        mainLayout->addWidget(groupInterp);

        // ================= 4. 动态显隐逻辑 =================
        auto updateUIState = [=]() {
            int k = kernel->currentIndex();
            int f = footprint->currentIndex();
            bool isVoronoi = (k == 0), isGaussian = (k == 1), isShepard = (k == 2);

            if (auto* l = formInterp->labelForField(footprint)) l->setVisible(!isVoronoi);
            footprint->setVisible(!isVoronoi);

            bool showRadius = !isVoronoi && (f == 0), showNClosest = !isVoronoi && (f == 1);
            if (auto* l = formInterp->labelForField(radius)) l->setVisible(showRadius);
            radius->setVisible(showRadius);
            if (auto* l = formInterp->labelForField(nClosest)) l->setVisible(showNClosest);
            nClosest->setVisible(showNClosest);

            if (auto* l = formInterp->labelForField(sharpness)) l->setVisible(isGaussian);
            sharpness->setVisible(isGaussian);
            if (auto* l = formInterp->labelForField(power)) l->setVisible(isShepard);
            power->setVisible(isShepard);
        };
        connect(kernel, qOverload<int>(&QComboBox::currentIndexChanged), page,
                [updateUIState](int) { updateUIState(); });
        connect(footprint, qOverload<int>(&QComboBox::currentIndexChanged), page,
                [updateUIState](int) { updateUIState(); });
        kernel->setCurrentIndex(0);
        updateUIState();

        QLabel* tip = new QLabel(
                QStringLiteral("悬停高亮后左键拖动：中心点平移、平面沿法向移动、法向端点或圆环旋转。\n未命中组件时左键旋转模型；点击应用后执行插值。"), body);
        tip->setStyleSheet("color: #888888; font-size: 12px; margin-top: 10px;");
        mainLayout->addWidget(tip);

        // ================= 5. 状态与按钮 =================
        QLabel* status = new QLabel(body);
        status->setStyleSheet("color: #A0A0A0; font-weight: bold; margin-top: 5px;");
        mainLayout->addWidget(status);

        QPushButton* applyBtn = new QPushButton(QStringLiteral("应用"), body);
        applyBtn->setStyleSheet("QPushButton { background-color: #2A2A2A; color: #EAEAEA; border: 1px solid #3A3A3A; "
                                "padding: 8px; border-radius: 4px; font-weight: bold; margin-top: 5px; } "
                                "QPushButton:hover { background-color: #3A3A3A; border-color: #4A4A4A; }");
        applyBtn->setProperty("igPanelBaseQss", applyBtn->styleSheet());
        igQtPanelTheme::refresh(applyBtn);

        mainLayout->addWidget(applyBtn);
        mainLayout->addStretch();

        scroll->setWidget(body);
        pageLayout->addWidget(scroll);

        // ================= 6. 核心逻辑初始化 =================
        auto selection = ClipSelection::New();
        selection->PlanePoint = Vector3d(cx, cy, cz);
        selection->PlaneNormal = Vector3d(1.0, 0.0, 0.0);

        auto interactor = scene->GetInteractor();
        auto previousBox = interactor->GetSpecialInteractor("SelectBox");
        auto previousTransform = interactor->GetSpecialInteractor("TransformBox");
        const bool previousHaveBox = SelectionParameter::Instance().GetHaveBox();
        const bool previousInSelection = SelectionParameter::Instance().GetInSelection();
        for (auto previous : {previousBox, previousTransform})
            if (auto box = DynamicCast<BoxStyle>(previous)) box->ClearDraw();
        interactor->RemoveSepcialInteractor("SelectBox");
        interactor->RemoveSepcialInteractor("TransformBox");
        auto planeStyle = RegionPlaneStyle::New();
        planeStyle->Initialize(interactor);
        planeStyle->SetBounds(bbox);
        planeStyle->SetPlane(Point(cx, cy, cz), Point(1., 0., 0.));
        // 7. 直接应用 按钮绑定
        connect(applyBtn, &QPushButton::clicked, page, [=]() mutable {
            status->setText(QStringLiteral("计算中..."));
            auto filter = PointPlaneInterpolatorFilter::New();
            filter->SetInput(input);
            filter->SetOrigin(Point(selection->PlanePoint[0], selection->PlanePoint[1], selection->PlanePoint[2]));

            Vector3d n(selection->PlaneNormal[0], selection->PlaneNormal[1], selection->PlaneNormal[2]);
            if (n.norm() > 1e-6) n = n.normalized();
            filter->SetNormal(Point(n[0], n[1], n[2]));
            filter->SetResolution(res->value());
            filter->SetKernelType(static_cast<PointPlaneInterpolatorFilter::KernelType>(kernel->currentIndex()));
            filter->SetKernelFootprint(
                    static_cast<PointPlaneInterpolatorFilter::KernelFootprint>(footprint->currentIndex()));
            filter->SetRadius(radius->value());
            filter->SetNumberOfPoints(nClosest->value());
            filter->SetSharpness(sharpness->value());
            filter->SetPowerParameter(power->value());
            filter->SetNullPointsStrategy(
                    static_cast<PointPlaneInterpolatorFilter::NullPointsStrategy>(nullStrategy->currentIndex()));

            if (filter->Execute()) {
                auto output = filter->GetOutput();
                if (!output) return;

                std::string targetName = input->GetName() + "_PlaneInterpolated";
                output->SetName(targetName);

                Model::Pointer existingModel = nullptr;
                auto modelList = rendererWidget->GetScene()->GetModelList();
                for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
                    if (it->second->GetDataObject()->GetName() == targetName) {
                        existingModel = it->second;
                        break;
                    }
                }

                if (!existingModel) {
                    int id = modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
                    existingModel = rendererWidget->GetScene()->GetModelById(id);
                } else {
                    existingModel->SetDataObject(output);
                    modelTreeWidget->updateItemName(output);
                    modelTreeWidget->updateAllAttriubute(output);
                }

                if (existingModel) rendererWidget->GetScene()->SetCurrentModel(existingModel);

                if (auto draw = DynamicCast<DrawObject>(output)) {
                    draw->ForceReConvertToDrawableData();
                    auto attrSet = output->GetAttributeSet();
                    if (attrSet && attrSet->GetNumberOfAttributes() > 0) {
                        for (int i = 0; i < attrSet->GetNumberOfAttributes(); ++i) {
                            auto& attr = attrSet->GetAttribute(i);
                            if (attr.pointer && attr.attachmentType == IG_POINT &&
                                attr.pointer->GetName() != "vtkValidPointMask") {
                                draw->ViewCloudPicture(rendererWidget->GetScene(), i, -1);
                                break;
                            }
                        }
                    }
                }
                rendererWidget->update();
                status->setText(QStringLiteral("更新成功！"));
            } else {
                status->setText(QStringLiteral("执行失败，请检查参数。"));
            }
        });

        // Reuse the region extractor's Plane widget without changing interpolation.
        QPointer<QWidget> pageGuard(page);
        auto syncUIto3D = [=]() {
            Vector3d n(normalX->value(), normalY->value(), normalZ->value());
            if (n.norm() < 1.e-6) return;
            n = n.normalized();
            selection->PlanePoint = Vector3d(originX->value(), originY->value(), originZ->value());
            selection->PlaneNormal = n;
            planeStyle->SetPlane(Point(originX->value(), originY->value(), originZ->value()),
                                 Point(n[0], n[1], n[2]));
            rendererWidget->update();
        };
        planeStyle->SetChangedCallback([=]() {
            if (!pageGuard) return;
            const auto& p = planeStyle->GetOrigin();
            const auto& n = planeStyle->GetNormal();
            selection->PlanePoint = Vector3d(p[0], p[1], p[2]);
            selection->PlaneNormal = Vector3d(n[0], n[1], n[2]);
            QSignalBlocker b1(originX), b2(originY), b3(originZ);
            QSignalBlocker b4(normalX), b5(normalY), b6(normalZ);
            originX->setValue(p[0]); originY->setValue(p[1]); originZ->setValue(p[2]);
            normalX->setValue(n[0]); normalY->setValue(n[1]); normalZ->setValue(n[2]);
            rendererWidget->update();
        });
        for (auto* p : {originX, originY, originZ, normalX, normalY, normalZ})
            connect(p, qOverload<double>(&QDoubleSpinBox::valueChanged), page, [=](double) { syncUIto3D(); });

        QPointer<igQtRenderWidget> renderGuard(rendererWidget);
        auto activate = [=]() {
            if (!renderGuard) return;
            interactor->RequestBasicStyle();
            interactor->_SetSpecialInteractor("PointPlaneInterpolatorPlane", planeStyle);
            SelectionParameter::Instance().SetHaveBox(false);
            SelectionParameter::Instance().SetInSelection(false);
            planeStyle->SetShow(true);
            renderGuard->update();
        };
        auto deactivate = [=]() {
            planeStyle->SetChangedCallback({});
            if (!renderGuard) return;
            planeStyle->SetShow(false);
            interactor->RemoveSepcialInteractor("PointPlaneInterpolatorPlane");
            if (previousBox) interactor->_SetSpecialInteractor("SelectBox", previousBox);
            if (previousTransform) interactor->_SetSpecialInteractor("TransformBox", previousTransform);
            SelectionParameter::Instance().SetHaveBox(previousHaveBox);
            SelectionParameter::Instance().SetInSelection(previousInSelection);
            for (auto previous : {previousBox, previousTransform})
                if (auto box = DynamicCast<BoxStyle>(previous)) if (box->GetBox()) box->ToDraw();
            renderGuard->update();
        };
        // 10. 嵌入左侧工具面板
        const int idx = m_leftFieldTabs->addTab(page, QStringLiteral("点平面插值"));
        m_leftFieldDock->show();
        m_leftFieldDock->raise();
        m_leftFieldTabs->setCurrentIndex(idx);

        // 11. 页面销毁时移除平面组件，并恢复之前的交互盒子。
        connect(page, &QObject::destroyed, this, [=]() {
            deactivate();
        });

        // 12. 安全处理 Tab 关闭事件
        QMetaObject::Connection* connPtr = new QMetaObject::Connection();
        *connPtr = connect(m_leftFieldTabs, &QTabWidget::tabCloseRequested, this, [=](int index) {
            if (index == m_leftFieldTabs->indexOf(pageGuard)) {
                m_leftFieldTabs->removeTab(index);
                pageGuard->deleteLater();
            }
        });
        connect(page, &QObject::destroyed, this, [this, connPtr]() {
            disconnect(*connPtr);
            delete connPtr;
        });

        // 13. 激活操作杆
        activate();
    });
            return true;
        }

        if (filterId == QStringLiteral("random_attributes")) {
            connect(action, &QAction::triggered, this, [=, this]() {
        const QString title = QStringLiteral("随机属性生成");
        auto input = currentFilterInput(title);
        if (!input) return;
        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(title);
        dialog->setFixedWidth(580);

        // 数据类型下拉框（10 种，与核心 Filter 支持一致）
        const int dataTypeId = dialog->addParameter(
                igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("数据类型 (Data Type)"),
                std::vector<QString>{QStringLiteral("Float"), QStringLiteral("Double"), QStringLiteral("Int"),
                        QStringLiteral("Unsigned Int"), QStringLiteral("Char"), QStringLiteral("Unsigned Char"),
                        QStringLiteral("Short"), QStringLiteral("Unsigned Short"), QStringLiteral("Long Long"),
                        QStringLiteral("Unsigned Long Long")});
        const int minId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("最小值 (Minimum)"), "0");
        const int maxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("最大值 (Maximum)"), "255");

        // 挂载位置下拉框（Point / Cell）
        const int attachId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                QStringLiteral("挂载位置 (Attach To)"),
                std::vector<QString>{QStringLiteral("点 (Point)"), QStringLiteral("面/单元 (Cell)")});
        const int seedId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("随机种子 (Seed)"), "0");

        // 数组名称 + 同名冲突处理
        const int attrNameId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                QStringLiteral("数组名称 (Attribute Name)"),
                QStringLiteral("RandomPointScalars"));
        const int conflictId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                QStringLiteral("同名处理 (Name Conflict)"),
                std::vector<QString>{QStringLiteral("追加 (Append)"),
                        QStringLiteral("替换 (Replace)"),
                        QStringLiteral("自动改名 (Auto Rename)")});
        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            bool ok{};
            static const int kTypes[] = {
                    IG_FLOAT, IG_DOUBLE, IG_INT, IG_UNSIGNED_INT, IG_CHAR, IG_UNSIGNED_CHAR,
                    IG_SHORT, IG_UNSIGNED_SHORT, IG_LONG_LONG, IG_UNSIGNED_LONG_LONG};
            const int typeIdx = dialog->getComboIndex(dataTypeId, ok);
            const double lo = dialog->getDouble(minId, ok);
            const double hi = dialog->getDouble(maxId, ok);
            const int attachIdx = dialog->getComboIndex(attachId, ok);
            const unsigned int seed = static_cast<unsigned int>(dialog->getInt(seedId, ok));
            const int conflictIdx = dialog->getComboIndex(conflictId, ok);
            if (!ok || typeIdx < 0 || typeIdx >= 10 || lo > hi || seed < 0) {
                showDarkFramelessMessage(title, QStringLiteral("随机属性参数无效。"));
                dialog->close();
                return;
            }
            std::string attrName;
            if (auto nameEdit = dynamic_cast<QLineEdit*>(dialog->getWidget(attrNameId))) {
                attrName = nameEdit->text().trimmed().toStdString();
            }
            RandomAttributesFilter::Pointer filter = RandomAttributesFilter::New();
            filter->SetInput(input);
            filter->SetRange(lo, hi);
            filter->SetDataType(kTypes[typeIdx]);
            filter->SetAttachmentType(attachIdx == 0 ? IG_POINT : IG_CELL);
            filter->SetSeed(seed);
            if (!attrName.empty()) filter->SetAttributeName(attrName);
            switch (conflictIdx) {
            case 1: filter->SetNameConflictMode(RandomAttributesFilter::NameConflictMode::Replace); break;
            case 2: filter->SetNameConflictMode(RandomAttributesFilter::NameConflictMode::AutoRename); break;
            default: filter->SetNameConflictMode(RandomAttributesFilter::NameConflictMode::Append); break;
            }
            if (!filter->Execute()) {
                showDarkFramelessMessage(title, QString::fromStdString(filter->GetMessage()));
                dialog->close();
                return;
            }
            // 核心已生成独立输出，直接使用
            auto rawOut = filter->GetOutput();
            if (!rawOut) { dialog->close(); return; }
            // 模型节点名兜底：无名 → RandomAttrResult；有名 → 原名 + 后缀
            rawOut->SetName(rawOut->GetName().empty() ? std::string("RandomAttrResult")
                                                      : rawOut->GetName() + " (RandomAttr)");
            modelTreeWidget->addDataObjectToModelTree(rawOut, Algorithm);
            // 执行后选中结果，默认按新随机数组着色
            if (auto drawObj = DynamicCast<DrawObject>(rawOut)) {
                auto outAttrSet = rawOut->GetAttributeSet();
                int idx = outAttrSet ? outAttrSet->GetAttributeIndex(
                        attrName.empty() ? "RandomPointScalars" : attrName) : -1;
                if (idx < 0 && outAttrSet) idx = (int)outAttrSet->GetNumberOfAttributes() - 1;
                if (idx >= 0) drawObj->ViewCloudPicture(rendererWidget->GetScene(), idx, 0);
            }
            rendererWidget->update();
            dialog->close();
        });
    });
            return true;
        }

        if (filterId == QStringLiteral("surface_vectors")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
                auto currentModel = rendererWidget->GetScene()->GetCurrentModel();
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                             QStringLiteral("请先加载并选择一个网格模型。"));
                    return;
                }
                auto obj = currentModel->GetDataObject();
                if (!obj) {
                    showDarkFramelessMessage(QStringLiteral("无可用数据"),
                                             QStringLiteral("当前模型没有可用的网格数据。"));
                    return;
                }

                auto attrs = obj->GetAttributeSet();
                std::vector<IGsize> attrIndices;
                std::vector<QString> attrNames;
                if (attrs) {
                    for (IGsize i = 0; i < static_cast<IGsize>(attrs->GetNumberOfAttributes()); ++i) {
                        auto& attr = attrs->GetAttribute(i);
                        if (attr.isDeleted || !attr.pointer) continue;
                        if (attr.pointer->GetDimension() < 3) continue; // 只显示向量(>=3分量)
                        attrIndices.push_back(i);
                        const QString attach =
                                attr.attachmentType == IG_CELL ? QStringLiteral("Cell") : QStringLiteral("Point");
                        attrNames.push_back(
                                QStringLiteral("%1 (%2)").arg(QString::fromStdString(attr.pointer->GetName()), attach));
                    }
                }
                if (attrNames.empty()) {
                    showDarkFramelessMessage(QStringLiteral("Warning"),
                                             QStringLiteral("当前模型没有可用于投影的向量属性（需要3分量及以上）。"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("Surface Vectors"));
                dialog->setFilterDescription(
                        QStringLiteral("将指定向量投影到表面，结果写回同名数组（与 ParaView 一致）。"
                                       "支持表面网格与非结构化网格。"));

                const int vecAttrId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("向量属性"), attrNames);
                const int modeId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_COMBO_BOX, QStringLiteral("约束模式"),
                        std::vector<QString>{
                                QStringLiteral("Parallel (切向: v-(v·n)·n)"),
                                QStringLiteral("Perpendicular (法向: (v·n)·n)"),
                                QStringLiteral("PerpendicularScale (标量: v·n)"),
                        });
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    const int choice = dialog->getComboIndex(vecAttrId, ok);
                    if (!ok || choice < 0 || choice >= static_cast<int>(attrIndices.size())) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("请选择有效的向量属性。"));
                        return;
                    }
                    const int mode = dialog->getComboIndex(modeId, ok);
                    if (!ok || mode < 0 || mode > 2) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("请选择有效的约束模式。"));
                        return;
                    }

                    const auto& attr = obj->GetAttributeSet()->GetAttribute(
                            static_cast<IGsize>(attrIndices[static_cast<size_t>(choice)]));
                    // 输出副本中替换同名数组，输入模型保持不变。
                    const QString attrName = QString::fromStdString(attr.pointer->GetName());

                    SurfaceVectorsFilter::Pointer filter = SurfaceVectorsFilter::New();
                    filter->SetVectorAttributeByIndex(static_cast<int>(attrIndices[static_cast<size_t>(choice)]));
                    filter->SetConstraintMode(
                            static_cast<SurfaceVectorsFilter::ConstraintMode>(mode));
                    filter->SetInput(obj);

                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("执行失败"),
                                QStringLiteral("向量投影失败，仅支持表面网格和非结构化网格。"));
                        return;
                    }

                    auto outObj = filter->GetOutput();
                    if (!outObj) {
                        showDarkFramelessMessage(QStringLiteral("执行失败"),
                                                 QStringLiteral("向量投影没有生成有效输出。"));
                        return;
                    }

                    modelTreeWidget->addDataObjectToModelTree(outObj, Algorithm);
                    rendererWidget->update();
                    showDarkFramelessMessage(
                            QStringLiteral("向量投影完成"),
                            QStringLiteral("已将向量按选定模式投影，结果写回属性 \"%1\"。")
                                    .arg(attrName),
                            true);
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("temporal_shift_scale")) {
            connect(action, &QAction::triggered, this, [this](bool) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("当前没有打开模型。"));
            return;
        }
        auto data = model->GetDataObject();
        if (data == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("当前模型没有数据。"));
            return;
        }
        auto frames = data->PeekTimeFrames();
        if (frames == nullptr || frames->GetTimeNum() == 0) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("当前模型没有时间序列。"));
            return;
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("TemporalShiftScale - 时间平移/缩放"));
        dialog->setFilterDescription(QStringLiteral("t' = (t + Pre Shift) × Scale + Post Shift"));
        const int preShiftId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                    QStringLiteral("Pre Shift"), "0.0");
        const int scaleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                 QStringLiteral("Scale"), "1.0");
        const int postShiftId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                     QStringLiteral("Post Shift"), "0.0");
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok = false;
            const float preShift = static_cast<float>(dialog->getDouble(preShiftId, ok));
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("Pre Shift 不是有效数字。"));
                return;
            }
            const float scale = static_cast<float>(dialog->getDouble(scaleId, ok));
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("Scale 不是有效数字。"));
                return;
            }
            const float postShift = static_cast<float>(dialog->getDouble(postShiftId, ok));
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("参数错误"), QStringLiteral("Post Shift 不是有效数字。"));
                return;
            }

            auto filter = iGame::TemporalShiftScaleFilter::New();
            filter->SetPreShift(preShift);
            filter->SetScale(scale);
            filter->SetPostShift(postShift);
            filter->SetInput(data);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("Warning"),
                                         QStringLiteral("执行失败：Scale 不能为 0，或输入没有时间序列。"));
                return;
            }
            auto outObj = filter->GetOutput();
            if (!outObj) {
                showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("输出对象为空。"));
                return;
            }
            modelTreeWidget->addDataObjectToModelTree(outObj, Algorithm);
            rendererWidget->update();
        });
    });
            return true;
        }

        if (filterId == QStringLiteral("volume_of_revolution")) {
            connect(action, &QAction::triggered, this, [=, this]() {
        const QString title = QStringLiteral("旋转体生成");
        auto input = currentFilterInput(title);
        if (!input) return;
        auto* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(title);
        const int ax = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("轴方向 X"), "0");
        const int ay = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("轴方向 Y"), "0");
        const int az = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("轴方向 Z"), "1");
        const int px = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("轴上一点 X"), "0");
        const int py = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("轴上一点 Y"), "0");
        const int pz = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("轴上一点 Z"), "0");
        const int resolution = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("分段数"), "12");
        const int angle = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, QStringLiteral("角度 (度)"), "360");
        dialog->show();
        dialog->setApplyFunctor([=, this]() {
            const int ids[] = {ax, ay, az, px, py, pz, resolution, angle};
            double values[8]{};
            for (int i = 0; i < 8; ++i) {
                bool ok = false;
                values[i] = dialog->getDouble(ids[i], ok);
                if (!ok) { showDarkFramelessMessage(title, QStringLiteral("旋转参数必须是数字。")); return; }
            }
            if (values[0] * values[0] + values[1] * values[1] + values[2] * values[2] == 0 ||
                values[6] < 3 || values[7] <= 0 || values[7] > 360) {
                showDarkFramelessMessage(title, QStringLiteral("轴、分段数或角度无效。")); return;
            }
            auto filter = VolumeOfRevolutionFilter::New();
            filter->SetInput(input);
            filter->SetAxis(Vector3d(values[0], values[1], values[2]),
                            Vector3d(values[3], values[4], values[5]));
            filter->SetResolution(static_cast<int>(values[6]));
            filter->SetAngle(values[7] * 3.141592653589793 / 180.0);
            if (!filter->Execute()) {
                const std::string message = filter->GetMessage();
                showDarkFramelessMessage(title, message.empty()
                                                    ? QStringLiteral("旋转体生成失败。")
                                                    : QString::fromStdString(message));
                return;
            }
            auto output=filter->GetOutput();
            output->SetName(input->GetName() + "_revolution");
            refreshFilterResult(input, output, title);
            dialog->close();
        });
    });
            return true;
        }

        if (filterId == QStringLiteral("yield_criteria")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("屈服准则"), QStringLiteral("请先选择一个模型。"));
                    return;
                }
                auto object = currentModel->GetDataObject();
                if (object == nullptr || iGame::DynamicCast<iGame::PointSet>(object).IsNull()) {
                    showDarkFramelessMessage(QStringLiteral("屈服准则"),
                                             QStringLiteral("当前对象不支持屈服准则（需要网格 / 点集）"));
                    return;
                }

                // 候选张量数组：6 / 9 分量的数组（不强制要求被标记为张量类型），按附着位置分为点 / 单元两组
                std::vector<QString> pointTensors;
                std::vector<QString> cellTensors;
                if (auto attrSet = object->GetAttributeSet()) {
                    const size_t numAttrs = attrSet->GetNumberOfAttributes();
                    for (size_t i = 0; i < numAttrs; ++i) {
                        auto& attr = attrSet->GetAttribute(static_cast<IGsize>(i));
                        if (attr.isDeleted || attr.pointer == nullptr) continue;
                        const int dim = attr.pointer->GetDimension();
                        if (dim != 6 && dim != 9) continue;
                        const QString name = QString::fromStdString(attr.pointer->GetName());
                        if (attr.attachmentType == IG_POINT) {
                            pointTensors.push_back(name);
                        } else if (attr.attachmentType == IG_CELL) {
                            cellTensors.push_back(name);
                        }
                    }
                }

                // ParaView-style checkable groups with select-all/partial state.
                auto makeTensorList = [](const QString& title,
                                         const std::vector<QString>& names) -> QListWidget* {
                    auto* row = new QWidget;
                    auto* layout = new QVBoxLayout(row);
                    layout->setContentsMargins(0, 0, 0, 0);
                    layout->setSpacing(2);
                    auto* label = new QCheckBox(title, row);
                    label->setTristate(true);
                    layout->addWidget(label);
                    auto* list = new QListWidget(row);
                    list->setSelectionMode(QAbstractItemView::NoSelection);
                    list->setAlternatingRowColors(true);
                    for (const auto& name : names) {
                        auto* item = new QListWidgetItem(name, list);
                        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                        item->setCheckState(Qt::Unchecked);
                    }
                    if (list->count() > 0) { list->item(0)->setCheckState(Qt::Checked); }
                    auto syncGroup = [list, label]() {
                        int checked = 0;
                        for (int i = 0; i < list->count(); ++i)
                            checked += list->item(i)->checkState() == Qt::Checked;
                        QSignalBlocker blocker(label);
                        label->setCheckState(checked == 0 ? Qt::Unchecked :
                            checked == list->count() ? Qt::Checked : Qt::PartiallyChecked);
                    };
                    QObject::connect(list, &QListWidget::itemChanged, row,
                                     [syncGroup](QListWidgetItem*) { syncGroup(); });
                    QObject::connect(label, &QCheckBox::clicked, row, [list, syncGroup](bool checked) {
                        {
                            QSignalBlocker blocker(list);
                            for (int i = 0; i < list->count(); ++i)
                                list->item(i)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
                        }
                        syncGroup();
                    });
                    syncGroup();
                    // 高度自适应：AdjustToContents 会让 sizeHint 反映全部条目的真实高度
                    // （注意 sizeHintForRow 往往略小于实际行高，按它估算会差几像素而出现滚动条），
                    // 这里直接把 sizeHint 作为最小高度，条目很多时再用 maximumHeight 限制。
                    list->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
                    list->setMinimumHeight(list->sizeHint().height());
                    const int maxRows = 8;
                    const int rowHeight = std::max(20, list->sizeHintForRow(0));
                    list->setMaximumHeight(maxRows * rowHeight + 2 * list->frameWidth() + 8);
                    layout->addWidget(list);
                    return list;
                };

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("屈服准则"));
                dialog->setFilterDescription(
                        QStringLiteral("勾选需要计算的结果，可同时选择多个；全部不勾选时输出原模型的数据副本。"
                                       "各分组标题可全选或取消全选。"));
                QListWidget* pointList = nullptr;
                QListWidget* cellList = nullptr;
                if (!pointTensors.empty()) {
                    pointList = makeTensorList(QStringLiteral("点张量 (Point Tensors)"), pointTensors);
                    dialog->addRowWidget(pointList->parentWidget());
                }
                if (!cellTensors.empty()) {
                    cellList = makeTensorList(QStringLiteral("单元张量 (Cell Tensors)"), cellTensors);
                    dialog->addRowWidget(cellList->parentWidget());
                }
                auto* criterionList = makeTensorList(QStringLiteral("屈服准则 (Yield Criteria)"),
                        {QStringLiteral("Principal Stress"), QStringLiteral("Tresca Criterion"),
                         QStringLiteral("Von Mises Criterion")});
                dialog->addRowWidget(criterionList->parentWidget());
                dialog->show();
                dialog->setApplyFunctor([=, this]() {
                    // 收集两个多选列表里选中的张量数组
                    std::vector<std::string> selectedTensors;
                    auto collect = [&selectedTensors](QListWidget* list) {
                        if (list == nullptr) { return; }
                        for (int i = 0; i < list->count(); ++i) {
                            auto* item = list->item(i);
                            if (item->checkState() == Qt::Checked) selectedTensors.push_back(item->text().toStdString());
                        }
                    };
                    collect(pointList);
                    collect(cellList);
                    std::vector<YieldCriteriaFilter::Criterion> selectedCriteria;
                    for (int i = 0; i < criterionList->count(); ++i) {
                        if (criterionList->item(i)->checkState() == Qt::Checked)
                            selectedCriteria.push_back(static_cast<YieldCriteriaFilter::Criterion>(i));
                    }
                    if (!selectedCriteria.empty() && selectedTensors.empty()) {
                        showDarkFramelessMessage(QStringLiteral("错误"),
                                                 QStringLiteral("请至少选择一个 6 或 9 分量的张量数组。"));
                        return;
                    }

                    auto filter = YieldCriteriaFilter::New();
                    filter->SetTensorArrayNames(selectedTensors);
                    filter->SetCriteria(selectedCriteria);
                    filter->SetInput(object);
                    if (filter->Execute()) {
                        modelTreeWidget->addDataObjectToModelTree(filter->GetOutput(), ItemSource::Algorithm);
                        modelTreeWidget->updateCloudPicture();
                        rendererWidget->update();
                        dialog->close();
                    } else {
                        showDarkFramelessMessage(QStringLiteral("错误"),
                                                 QString::fromStdString(filter->GetStatusMessage()));
                    }
                });
            });
            return true;
        }

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

void igQtMainWindow::closeHistogramParameters() {
    if (!m_histogramParametersDock) return;
    closeLeftToolWidget("histogramParameters");
    auto* dialog = m_histogramParametersDock.data();
    if (m_histogramParameterContents) dialog->setWidget(m_histogramParameterContents);
    m_histogramParametersDock = nullptr;
    m_histogramParameterContents = nullptr;
    dialog->close();
}
