#include "IQCore/igQtMainWindow.h"
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQComponents/Dialog/igQtChromeFramelessDialog.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <iGameScene.h>
#include <iGameSelection.h>
#include <QDialog>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QFormLayout>
#include <QDockWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <cmath>
#include <vector>
#include <limits>
#include "ValidateCells/iGameValidateCellsFilter.h"
#include "ExtractCellsByType/iGameExtractCellsByTypeFilter.h"
#include "DataProcessing/ExtractLocation/iGameExtractLocationFilter.h"
#include "Outline/iGameOutlineFilter.h"
#include "ReverseSense/iGameReverseSenseFilter.h"
#include "WarpByScalar/iGameWarpByScalarFilter.h"
#include "AppendAttributes/iGameAppendAttributesFilter.h"
#include "Tube/iGameTube.h"
#include "Subdivide/iGameSubdivide.h"
#include "Modeling/iGameLinearExtrusionFilter.h"
#include <IQWidgets/igQtExtractCellsByTypeWidget.h>
#include <IQWidgets/igQtExtractLocationWidget.h>
#include <IQWidgets/igQtLinearExtrusionWidget.h>
#include <map>
#include <QCheckBox>

using namespace iGame;

namespace {
// Shared model information for the input selection and Apply callback.
struct AppendInputInfo {
    QString name;
    DataObject::Pointer object;
    IGsize points{0};
    IGsize cells{0};
};
}

// Keep migrated standard-menu implementations together while preserving the
// current branch's existing menu dispatch and parameter-panel behavior.
bool igQtMainWindow::connectMigratedFilterAction(QAction* action, const QString& filterId) {
    auto currentFilterInput = [this](const QString& title) -> DataObject::Pointer {
        auto scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto input = model ? model->GetDataObject() : nullptr;
        if (!input) showDarkFramelessMessage(title, QStringLiteral("请先加载并选择一个模型。"));
        return input;
    };
        if (filterId == QStringLiteral("validate_cells")) {
            connect(action, &QAction::triggered, this, [this]() {
                auto scene = rendererWidget->GetScene();
                if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                             QStringLiteral("请先加载并选择一个网格模型。"));
                    return;
                }
                const auto model = scene->GetCurrentModel();
                const auto sourceModelId = scene->GetCurrentModelID();
                const auto dataObject = model == nullptr ? nullptr : model->GetDataObject();
                if (model == nullptr || dataObject == nullptr ||
                    (DynamicCast<UnstructuredMesh>(dataObject).IsNull() &&
                     DynamicCast<SurfaceMesh>(dataObject).IsNull())) {
                    showDarkFramelessMessage(
                            QStringLiteral("单元几何校验"),
                            QStringLiteral("请先在模型树中选择非结构网格或表面网格。"));
                    return;
                }

                auto filter = ValidateCellsFilter::New();
                filter->SetInput(dataObject);
                if (!filter->Execute()) {
                    const auto& error = filter->GetLastError();
                    showDarkFramelessMessage(
                            QStringLiteral("单元几何校验"),
                            QStringLiteral("执行失败：%1")
                                    .arg(error.empty() ? QStringLiteral("未知错误。")
                                                       : QString::fromStdString(error)));
                    return;
                }

                // Filter 返回独立网格（含 ValidityState 单元标量）；原模型保持不变。
                auto data = filter->GetOutput();
                if (data == nullptr) {
                    showDarkFramelessMessage(QStringLiteral("单元几何校验"),
                                             QStringLiteral("执行失败：未生成有效输出。"));
                    return;
                }

                const auto outputModelId = modelTreeWidget->addDataObjectToModelTree(
                        data, ItemSource::Algorithm);
                auto outputModel = scene->GetModelById(outputModelId);
                const int attributeIndex = data->GetAttributeSet()->GetAttributeIndex(
                        ValidateCellsFilter::ValidityStateArrayName());
                auto item = modelTreeWidget->getItemFromObject(data);
                if (item != nullptr && attributeIndex >= 0 &&
                    attributeIndex < item->childCount()) {
                    item->setExpanded(true);
                    auto* child = item->child(attributeIndex);
                    item->setCurrentChild(child);
                    item->setSelected(false);
                    item->viewAttribute(attributeIndex, 0);
                    child->setSelected(true);
                    modelTreeWidget->setCurrentItem(child);
                }

                const auto& invalidIds = filter->GetInvalidCellIds();
                auto selection = outputModel == nullptr ? nullptr
                                                        : outputModel->GetSelection();
                if (selection != nullptr) {
                    selection->Reset();
                    if (!invalidIds.empty()) {
                        selection->SelectionCallBackEvent(
                                IG_CELL, invalidIds, Selection::Operate::Add);
                        selection->SetSelectItemVisable(true);
                    }
                }

                if (outputModel != nullptr) {
                    model->SetVisibility(false);
                    outputModel->SetVisibility(true);
                    scene->SetCurrentModel(outputModel);

                    // 结果与输入节点互斥显示：选择输入时恢复原始外观，
                    // 选择结果时显示 ValidityState 标量和无效单元高亮。
                    const bool hasInvalid = !invalidIds.empty();
                    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this,
                            [this, scene, sourceModelId, outputModelId, hasInvalid]() {
                                auto sourceModel = scene->GetModelById(sourceModelId);
                                auto resultModel = scene->GetModelById(outputModelId);
                                if (resultModel == nullptr) return;

                                const auto currentModelId = scene->GetCurrentModelID();
                                const bool resultSelected = currentModelId == outputModelId;
                                resultModel->SetVisibility(resultSelected);
                                if (auto resultSelection = resultModel->GetSelection())
                                    resultSelection->SetSelectItemVisable(
                                            resultSelected && hasInvalid);

                                if (sourceModel != nullptr &&
                                    (currentModelId == sourceModelId || resultSelected))
                                    sourceModel->SetVisibility(!resultSelected);
                                rendererWidget->update();
                            });
                }
                rendererWidget->update();

                // ValidityState 取值分布，例如 “0:7400, 16:50, 48:22”。
                std::map<short, int> stateDistribution;
                for (short state : filter->GetValidityStates()) {
                    stateDistribution[state]++;
                }
                QStringList distributionText;
                for (const auto& entry : stateDistribution) {
                    distributionText.push_back(
                            QStringLiteral("%1:%2").arg(entry.first).arg(entry.second));
                }

                showDarkFramelessMessage(
                        QStringLiteral("单元几何校验"),
                        QStringLiteral("校验完成：共 %1 个单元，有效 %2，无效 %3。<br>"
                                       "ValidityState 分布：%4")
                                .arg(filter->GetCheckedCellCount())
                                .arg(filter->GetValidCellCount())
                                .arg(filter->GetInvalidCellCount())
                                .arg(distributionText.join(QStringLiteral(", "))));
    });
            return true;
        }

        if (filterId == QStringLiteral("extract_cells_by_type")) {
            connect(action, &QAction::triggered, this, [=, this]() {
        const QString title = QStringLiteral("按单元类型提取");
        auto input = currentFilterInput(title);
        if (!input) return;
        auto filter = ExtractCellsByTypeFilter::New();
        filter->SetInput(input);
        const auto types = filter->GetAvailableCellTypes();
        if (types.empty()) { showDarkFramelessMessage(title, QStringLiteral("当前模型没有可提取的单元类型。")); return; }
        auto* dock = new QDockWidget(title, this);
        dock->setAttribute(Qt::WA_DeleteOnClose);
        dock->setObjectName(QStringLiteral("dockWidget_ExtractCellsByType"));
        auto* panel = new igQtExtractCellsByTypeWidget(dock);
        panel->SetDataObject(input);
        dock->setWidget(panel);
        addDockWidget(Qt::LeftDockWidgetArea, dock);
        panel->onApply = [=, this, resultModel = Model::Pointer{}]() mutable {
            auto selected = panel->GetSelectedCellTypes();
            if (selected.empty()) { showDarkFramelessMessage(title, QStringLiteral("请至少选择一种单元类型。")); return; }
            filter->SetExtractCellTypes(selected);
            if (!filter->Execute()) { showDarkFramelessMessage(title, QStringLiteral("按类型提取失败。")); return; }
            auto output = filter->GetOutput();
            if (!resultModel) {
                const int id = modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
                resultModel = rendererWidget->GetScene()->GetModelById(id);
            } else {
                resultModel->SetDataObject(output);
                modelTreeWidget->updateItemName(output);
                modelTreeWidget->updateAllAttriubute(output);
            }
            rendererWidget->update();
        };
        dock->show();
        panel->onApply();
    });
            return true;
        }

        if (filterId == QStringLiteral("extract_location")) {
            connect(action, &QAction::triggered, this, [this]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (!model) {
            showDarkFramelessMessage(QStringLiteral("提取指定位置数据"),
                                     QStringLiteral("请先在模型树中选择一个网格模型。"));
            return;
        }
        auto* panel = new igQtExtractLocationWidget(rendererWidget, modelTreeWidget, model, this);
        if (!panel->isReady()) {
            showDarkFramelessMessage(QStringLiteral("提取指定位置数据"),
                                     QStringLiteral("当前仅支持非结构网格（UnstructuredMesh）；操作已取消。"));
            panel->deleteLater();
            return;
        }
        panel->show();
        panel->raise();
        panel->activateWindow();
    });
            return true;
        }

        if (filterId == QStringLiteral("outline")) {
            connect(action, &QAction::triggered, this, [this](bool) {
        auto scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        if (!model) {
            showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                     QStringLiteral("请先加载并在模型树中选择一个模型。"));
            return;
        }

        auto input = model->GetDataObject();
        if (!input) {
            showDarkFramelessMessage(QStringLiteral("无可用模型"),
                                     QStringLiteral("当前模型没有可用数据。"));
            return;
        }

        auto filter = OutlineFilter::New();
        filter->SetInput(input);
        if (!filter->Execute()) {
            showDarkFramelessMessage(QStringLiteral("生成包围盒失败"),
                                     QString::fromStdString(filter->GetMessage()));
            return;
        }

        auto output = DynamicCast<DrawObject>(filter->GetOutput());
        if (!output) {
            showDarkFramelessMessage(QStringLiteral("生成包围盒失败"),
                                     QStringLiteral("包围盒输出为空或无法显示。"));
            return;
        }

        // 将白色包围盒作为独立结果加入模型树
        output->SetName(QStringLiteral("Outline_%1").arg(++m_outlineCount).toStdString());
        output->SetViewStyle(IG_WIREFRAME);
        output->SetLineColor(igm::vec3(1.0f, 1.0f, 1.0f));
        output->SetLineWidth(2.0f);
        modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
        rendererWidget->update();
    });
            return true;
        }

        if (filterId == QStringLiteral("reverse_sense")) {
            connect(action, &QAction::triggered, this, [this](bool) {
                auto scene = rendererWidget ? rendererWidget->GetScene() : nullptr;
                auto currentModel = scene ? scene->GetCurrentModel() : nullptr;
                if (!currentModel) {
                    showDarkFramelessMessage(QStringLiteral("反转面朝向"), QStringLiteral("请先选择一个模型。"));
                    return;
                }
                auto object = currentModel->GetDataObject();
                if (iGame::DynamicCast<iGame::SurfaceMesh>(object).IsNull()) {
                    showDarkFramelessMessage(QStringLiteral("反转面朝向"),
                                             QStringLiteral("当前模型不是曲面网格（SurfaceMesh）"));
                    return;
                }

                auto* dialog = new igQtFilterDialogDockWidget(this, true);
                dialog->setFilterTitle(QStringLiteral("反转面朝向"));
                dialog->setFilterDescription(QStringLiteral("反转面的顶点环序，并可取反法向"));
                const int cellsId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("反转面顶点顺序"), "true");
                const int normalsId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX, QStringLiteral("反转法向"), "true");
                dialog->setParameterLayoutVertical();
                auto* help = new QLabel(QStringLiteral(
                        "反转面顶点顺序：改变面的朝向。\n"
                        "反转法向：取反已有的点法向和面法向，不生成新法向。"), dialog);
                help->setWordWrap(true);
                help->setContentsMargins(0, 8, 0, 0);
                dialog->addRowWidget(help);
                dialog->setMinimumWidth(420);
                dialog->resize(qMax(460, help->fontMetrics().horizontalAdvance(
                        QStringLiteral("反转法向：取反已有的点法向和面法向")) + 80), 330);
                if (auto* scrollArea = dialog->findChild<QScrollArea*>(QStringLiteral("scrollArea"))) {
                    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                }
                dialog->show();

                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    auto filter = iGame::ReverseSenseFilter::New();
                    filter->SetReverseCells(dialog->getChecked(cellsId, ok));
                    filter->SetReverseNormals(dialog->getChecked(normalsId, ok));
                    filter->SetInput(object);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("反转面朝向"),
                                QStringLiteral("执行失败：%1").arg(QString::fromStdString(filter->GetMessage())));
                        return;
                    }
                    auto output = filter->GetOutput();
                    if (!output) {
                        showDarkFramelessMessage(QStringLiteral("反转面朝向"), QStringLiteral("输出对象为空"));
                        return;
                    }
                    output->SetName(object->GetName() + "_Reversed");

                    auto inputDraw = iGame::DynamicCast<iGame::DrawObject>(object);
                    auto outDraw = iGame::DynamicCast<iGame::DrawObject>(output);
                    if (inputDraw && outDraw) {
                        outDraw->SetViewStyle(static_cast<IGenum>(inputDraw->GetViewStyle()));
                    }

                    modelTreeWidget->addDataObjectToModelTree(output, ItemSource::Algorithm);
                    modelTreeWidget->updateCloudPicture();
                    rendererWidget->update();
                    dialog->close();
                });
            });
            return true;
        }

        if (filterId == QStringLiteral("warp_by_scalar")) {
            connect(action, &QAction::triggered, this, [this](bool) {
        // 变形过滤器只改写点坐标，因此只接受点集派生类型
        auto resolveWarpInput = [this](DataObject::Pointer& obj, const QString& title) -> bool {
            obj = nullptr;
            auto scene = rendererWidget->GetScene();
            if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
                showDarkFramelessMessage(title, QStringLiteral("请先在模型树中选择一个模型。"));
                return false;
            }
            obj = scene->GetCurrentModel()->GetDataObject();
            if (!obj) {
                showDarkFramelessMessage(title, QStringLiteral("当前模型没有可用的数据对象。"));
                return false;
            }
            const IGenum type = obj->GetDataObjectType();
            if (type != IG_POINT_SET && type != IG_SURFACE_MESH && type != IG_VOLUME_MESH &&
                type != IG_UNSTRUCTURED_MESH && type != IG_STRUCTURED_MESH) {
                showDarkFramelessMessage(
                        title, QStringLiteral("该算法只支持点集 / 表面网格 / 体网格 / 非结构网格 / 结构网格。"));
                return false;
            }
            return true;
        };

        struct WarpArrayInfo {
            QString name;
            int dimension{1};
            QString typeName;
        };
        auto collectPointArrays = [](DataObject::Pointer obj, std::vector<WarpArrayInfo>& out) {
            auto attrSet = obj->GetAttributeSet();
            if (!attrSet) { return; }
            auto attrs = attrSet->GetAllPointAttributes();
            if (!attrs) { return; }
            for (IGsize i = 0; i < attrs->GetNumberOfElements(); ++i) {
                auto& attr = attrs->GetElement(i);
                if (attr.IsNone() || !attr.pointer) { continue; }
                // Warp By Scalar accepts only single-component point arrays.
                if (attr.pointer->GetDimension() != 1) { continue; }
                WarpArrayInfo info;
                info.name = QString::fromStdString(attr.pointer->GetName());
                info.dimension = attr.pointer->GetDimension();
                switch (attr.type) {
                    case IG_SCALAR:
                        info.typeName = QStringLiteral("标量");
                        break;
                    case IG_VECTOR:
                        info.typeName = QStringLiteral("向量");
                        break;
                    case IG_NORMAL:
                        info.typeName = QStringLiteral("法向");
                        break;
                    case IG_TCOORD:
                        info.typeName = QStringLiteral("纹理坐标");
                        break;
                    case IG_TENSOR:
                        info.typeName = QStringLiteral("张量");
                        break;
                    default:
                        info.typeName = QStringLiteral("属性");
                        break;
                }
                out.push_back(info);
            }
        };
        DataObject::Pointer obj = nullptr;
        if (!resolveWarpInput(obj, QStringLiteral("按标量变形"))) { return; }

        std::vector<WarpArrayInfo> arrays;
        collectPointArrays(obj, arrays);
        if (arrays.empty()) {
            showDarkFramelessMessage(QStringLiteral("按标量变形"),
                                     QStringLiteral("当前模型没有单分量点属性数组，无法按标量变形。\n"
                                                    "可以先用「转换为点数据」把单元属性转到点上。"));
            return;
        }
        std::vector<QString> items;
        for (const auto& a: arrays) {
            items.push_back(QStringLiteral("%1 (%2, %3 分量)").arg(a.name, a.typeName).arg(a.dimension));
        }

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this, true);
        dialog->setFilterTitle(QStringLiteral("按标量变形 (Warp By Scalar)"));
        dialog->setFilterDescription(QStringLiteral("沿法向按标量值移动点：x' = x + n * s * 缩放系数。\n"
                                                    "未勾选「使用指定法向」时优先使用输入的点法向数组，"
                                                    "缺失时退回下面的法向。"));
        int arrayId = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX,
                                           QStringLiteral("标量数组 (Scalars)"), items);
        int scaleId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                           QStringLiteral("缩放系数 (Scale Factor)"), "1.0");
        int useNormalId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                               QStringLiteral("使用指定法向 (Use Normal)"), "false");
        int nxId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                        QStringLiteral("法向 X (Normal X)"), "0.0");
        int nyId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                        QStringLiteral("法向 Y (Normal Y)"), "0.0");
        int nzId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                        QStringLiteral("法向 Z (Normal Z)"), "1.0");
        int xyPlaneId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
                                             QStringLiteral("XY 平面模式 (XY Plane)"), "false");

        dialog->setFixedWidth(360);
        if (auto* sa = dialog->findChild<QScrollArea*>(QStringLiteral("scrollArea"))) {
            sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        dialog->show();

        dialog->setApplyFunctor([=, this]() {
            bool ok = false;
            const int choice = dialog->getComboIndex(arrayId, ok);
            if (!ok || choice < 0 || choice >= static_cast<int>(arrays.size())) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("请选择有效的标量数组。"));
                return;
            }
            const double scale = dialog->getDouble(scaleId, ok);
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("缩放系数必须是数值。"));
                return;
            }
            const double nx = dialog->getDouble(nxId, ok);
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("法向 X 必须是数值。"));
                return;
            }
            const double ny = dialog->getDouble(nyId, ok);
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("法向 Y 必须是数值。"));
                return;
            }
            const double nz = dialog->getDouble(nzId, ok);
            if (!ok) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("法向 Z 必须是数值。"));
                return;
            }
            const bool useNormal = dialog->getChecked(useNormalId, ok);
            const bool xyPlane = dialog->getChecked(xyPlaneId, ok);

            auto filter = WarpByScalar::New();
            filter->SetScalarsArrayName(arrays[static_cast<size_t>(choice)].name.toStdString());
            filter->SetScaleFactor(scale);
            filter->SetUseNormal(useNormal);
            filter->SetNormal(nx, ny, nz);
            filter->SetXYPlane(xyPlane);
            filter->SetInput(obj);
            if (!filter->Execute()) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"),
                                         QStringLiteral("变形失败：请检查标量数组与法向设置。"));
                return;
            }
            auto output = filter->GetOutput();
            if (!output) {
                showDarkFramelessMessage(QStringLiteral("按标量变形"), QStringLiteral("算法未产生有效结果。"));
                return;
            }
            output->SetName(obj->GetName() + "_warp_scalar");
            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
            rendererWidget->GetScene()->Modified();
            rendererWidget->GetScene()->Update();
            rendererWidget->update();
            dialog->close();
        });
    });
            return true;
        }

        if (filterId == QStringLiteral("append_attributes")) {
            connect(action, &QAction::triggered, this, &igQtMainWindow::showAppendAttributesFilter);
            return true;
        }
        if (filterId == QStringLiteral("tube")) {
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
                dialog->setFilterTitle(QStringLiteral("管道生成 (Tube)"));
                dialog->setFilterDescription(QStringLiteral(
                    "将线段（如流线、折线）沿路径扫掠成圆管。<br>"
                    "在每个路径点上确定垂直于前进方向的平面，画一个由半径和边数决定的正多边形截面，"
                    "再沿路径对齐、缝合。<br>"
                    "截面朝向通过平行传递延续，不会翻转；首尾可用端面封闭。"));

                const int radiusId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("半径 (Radius)"), "0.1");
                const int sidesId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("边数 (Number Of Sides)"), "6");
                const int capId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX,
                        QStringLiteral("端面封端 (Capping)"), "true");
                const int useDefId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_CHECK_BOX,
                        QStringLiteral("手动指定初始法向 (Use Default Normal)"), "false");
                const int nxId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("初始法向 X (Normal X)"), "0");
                const int nyId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("初始法向 Y (Normal Y)"), "0");
                const int nzId = dialog->addParameter(
                        igQtFilterDialogDockWidget::QT_LINE_EDIT,
                        QStringLiteral("初始法向 Z (Normal Z)"), "1");

                // 加宽面板，避免说明文字与标签过度换行
                dialog->setFixedWidth(660);

                // “手动指定初始法向”勾选后，下方三个初始法向输入框才可用（默认置灰）
                auto* useDefCheck = qobject_cast<QCheckBox*>(dialog->getWidget(useDefId));
                QWidget* nxEdit = dialog->getWidget(nxId);
                QWidget* nyEdit = dialog->getWidget(nyId);
                QWidget* nzEdit = dialog->getWidget(nzId);
                auto syncNormalEditors = [nxEdit, nyEdit, nzEdit](bool on) {
                    nxEdit->setEnabled(on);
                    nyEdit->setEnabled(on);
                    nzEdit->setEnabled(on);
                };
                connect(useDefCheck, &QCheckBox::toggled, dialog, syncNormalEditors);
                syncNormalEditors(false);
                useDefCheck->setToolTip(QStringLiteral(
                    "勾选后，使用下方“初始法向 X/Y/Z”指定的方向确定第一个截面的朝向；"
                    "不勾选（默认）则由程序自动选择（管子起始竖直时也会自动换轴）。"));
                const QString normalEditTip = QStringLiteral(
                    "仅在勾选上方“手动指定初始法向”后生效，用于确定第一个截面的初始朝向。");
                nxEdit->setToolTip(normalEditTip);
                nyEdit->setToolTip(normalEditTip);
                nzEdit->setToolTip(normalEditTip);

                dialog->setApplyFunctor([this, dialog, obj, radiusId, sidesId,
                                        capId, useDefId, nxId, nyId, nzId]() {
                    bool rok = false, sok = false,
                         cok = false, uok = false,
                         xok = false, yok = false, zok = false;

                    double radius = dialog->getDouble(radiusId, rok);
                    int sides = dialog->getInt(sidesId, sok);
                    bool capping = dialog->getChecked(capId, cok);
                    bool useDef = dialog->getChecked(useDefId, uok);
                    double nx = dialog->getDouble(nxId, xok);
                    double ny = dialog->getDouble(nyId, yok);
                    double nz = dialog->getDouble(nzId, zok);

                    if (!rok || radius <= 0.0) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("半径必须是大于 0 的数字。"));
                        return;
                    }
                    if (!sok || sides < 3) {
                        showDarkFramelessMessage(QStringLiteral("参数错误"),
                                                 QStringLiteral("边数必须是不小于 3 的整数。"));
                        return;
                    }

                    TubeFilter::Pointer filter = TubeFilter::New();
                    filter->SetInput(obj);
                    filter->SetRadius(radius);
                    filter->SetNumberOfSides(sides);
                    filter->SetCapping(cok ? capping : true);
                    filter->SetUseDefaultNormal(uok ? useDef : false);
                    if (xok && yok && zok)
                        filter->SetDefaultNormal(Vector3d(nx, ny, nz));

                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("数据类型不匹配"),
                                QStringLiteral("管道生成仅支持含线段（IG_LINE / IG_POLY_LINE）的"
                                               "非结构网格或表面网格，请检查输入数据类型。"));
                        return;
                    }

                    auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
                    modelTreeWidget->addDataObjectToModelTree(outMesh, Algorithm);
                    rendererWidget->update();
                    // 先弹模态提示（此时参数面板仍在、主窗口布局稳定），
                    // 用户点“确定”返回后再关闭面板，避免先 close dock 触发布局
                    // 重建、紧接着开模态对话框而导致 Qt 访问违例。
                    showDarkFramelessMessage(QStringLiteral("管道生成完成"),
                                             QStringLiteral("已将线段转换为圆管，可在模型树中查看输出结果。"),
                                             true);
                    dialog->close();
                });
                dialog->show();
            });
            return true;
        }

        if (filterId == QStringLiteral("subdivide")) {
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
                dialog->setFilterTitle(QStringLiteral("三角面细分 (Subdivide)"));
                int subId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
                                                 QStringLiteral("细分次数 (1~8，每次三角形数量 ×4)"), "1");
                dialog->setApplyFunctor([=, this]() {
                    bool ok = false;
                    int tms = dialog->getInt(subId, ok);
                    if (!ok || tms < 1 || tms > 8) {
                        showDarkFramelessMessage(QStringLiteral("Warning"),
                                                 QStringLiteral("请输入 1 ~ 8 之间的整数细分次数。"));
                        return;
                    }

                    auto filter = SubdivideFilter::New();
                    filter->SetNumberOfSubdivisions(tms);
                    filter->SetInput(obj);
                    if (!filter->Execute()) {
                        showDarkFramelessMessage(
                                QStringLiteral("数据类型不匹配"),
                                QStringLiteral("三角面细分仅支持多边形表面网格（Poly Data），请检查输入数据类型。"));
                        return;
                    }

                    auto outMesh = DynamicCast<SurfaceMesh>(filter->GetOutput());
                    modelTreeWidget->addDataObjectToModelTree(outMesh, Algorithm);
                    rendererWidget->update();
                    // 先弹模态提示（面板仍在、布局稳定），用户点“确定”后再关面板
                    showDarkFramelessMessage(QStringLiteral("三角面细分完成"),
                                             QStringLiteral("已对三角面做线性细分，旧顶点保持不变、边中点为线性插值；"
                                                            "可将显示模式改为 Surface With Edges 查看加密后的网格。"),
                                             true);
                    dialog->close();
                });
                dialog->show();
            });
            return true;
        }

        if (filterId == QStringLiteral("linear_extrusion")) {
            connect(action, &QAction::triggered, this, [this](bool checked) {
        auto scene = rendererWidget->GetScene();
        if (scene == nullptr || scene->GetCurrentModel() == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("请先选择一个模型。"));
            return;
        }
        auto data = scene->GetCurrentModel()->GetDataObject();
        if (data == nullptr) {
            showDarkFramelessMessage(QStringLiteral("Warning"), QStringLiteral("当前模型没有数据。"));
            return;
        }
        if (m_linearExtrusionDialog == nullptr) {
            // 首次打开时创建独立弹窗：非模态，用户点 X 才关闭；
            // 面板内容仍是 igQtLinearExtrusionWidget，只是宿主从左侧工具面板换成独立窗口
            m_linearExtrusionDialog = new QDialog(this);
            m_linearExtrusionDialog->setWindowTitle(QStringLiteral("线性拉伸"));
            // 面板配色只作用于本弹窗：黑底白字、边框可见、字号比全局 12pt 更小。
            // 复选框必须连 ::indicator 一起显式定义：一旦给 QCheckBox 设了样式表，Qt 就不再画系统
            // 原生指示器；而全局样式里的对勾图片路径写错了（qrc 前缀是 /Ticon，样式里却写成 :/icons/…，
            // 大小写也不对），所以 :checked 只剩底色，看上去是个纯蓝色方块、分不出选中与否。
            // image 必须给「文件路径 / :/资源路径」，Qt 样式表不接受 data: URI（内联 SVG 试过，不生效）；
            // 也不能复用 Icons/check.png：它描边是 #1296DB，压在本面板蓝底 #0E639C 上对比度只有约 1.95，
            // 14px 下看不出对勾，因此新增白色版 Icons/check_white.svg（对比度约 6.4）。
            const QString panelStyle = QString::fromUtf8(
                    "QDialog { background-color: #1E1E1E; }"
                    "QLabel { color: #FFFFFF; font-size: 11px; }"
                    "QLabel:disabled { color: #6A6A6A; }"
                    "QGroupBox { color: #FFFFFF; border: 1px solid #3C3C3C; border-radius: 4px; padding: 4px; }"
                    "QGroupBox:disabled { color: #6A6A6A; }"
                    "QCheckBox { color: #FFFFFF; font-size: 11px; spacing: 6px; }"
                    "QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #6A6A6A;"
                    " border-radius: 2px; background-color: #2A2A2A; }"
                    "QCheckBox::indicator:unchecked { background-color: #2A2A2A; }"
                    "QCheckBox::indicator:checked { background-color: #0E639C; border: 1px solid #1F8AD2;"
                    " image: url(:/Ticon/Icons/check_white.svg); }"
                    "QCheckBox::indicator:disabled { border-color: #3C3C3C; background-color: #232323; }"
                    "QComboBox, QLineEdit { background-color: #2A2A2A; color: #FFFFFF; border: 1px solid #4A4A4A;"
                    " border-radius: 3px; padding: 2px 6px; min-height: 18px; font-size: 11px; }"
                    "QComboBox:disabled, QLineEdit:disabled { color: #6A6A6A; }"
                    "QComboBox QAbstractItemView { background-color: #2A2A2A; color: #FFFFFF;"
                    " selection-background-color: #007399; }"
                    "QPushButton { background-color: #2D2D30; color: #FFFFFF; border: 1px solid #4A4A4A;"
                    " border-radius: 3px; padding: 3px 12px; min-height: 18px; font-size: 11px; }"
                    "QPushButton:hover { background-color: #3A3A3D; }");
            m_linearExtrusionDialog->setStyleSheet(panelStyle);
            auto* layout = new QVBoxLayout(m_linearExtrusionDialog);
            layout->setContentsMargins(0, 0, 0, 0);
            // 不锁定弹窗尺寸：宽度与高度都交给用户自由拖动
            layout->setSizeConstraint(QLayout::SetDefaultConstraint);
            m_linearExtrusionWidget = new igQtLinearExtrusionWidget(m_linearExtrusionDialog);
            m_linearExtrusionWidget->setMinimumSize(0, 0);
            layout->addWidget(m_linearExtrusionWidget);
            m_linearExtrusionDialog->setMinimumSize(0, 0);
            m_linearExtrusionDialog->resize(360, 380);

            connect(m_linearExtrusionWidget, &igQtLinearExtrusionWidget::DrawLinearExtrusionModel, this,
                    [this](iGame::DataObject::Pointer res) {
                        modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
                    });
            connect(m_linearExtrusionWidget, &igQtLinearExtrusionWidget::UpdateLinearExtrusionModel, this,
                    [this](iGame::DataObject::Pointer res) {
                        modelTreeWidget->updateCurrentModelInfo();
                        rendererWidget->update();
                    });
            connect(m_linearExtrusionWidget, &igQtLinearExtrusionWidget::ApplyFailed, this,
                    [this](const QString& message) { showDarkFramelessMessage(QStringLiteral("Warning"), message); });
        }
        // 每次打开都按当前模型刷新输入数据
        m_linearExtrusionWidget->SetOriginDataObject(data);
        m_linearExtrusionDialog->show();
        m_linearExtrusionDialog->raise();
        m_linearExtrusionDialog->activateWindow();
    });
            return true;
        }
    return false;
}

void igQtMainWindow::showAppendAttributesFilter() {
        auto scene = rendererWidget->GetScene();
        if (scene == nullptr) { return; }
        auto modelList = scene->GetModelList();
        if (!modelList) { return; }

        // 计算单元数，用于在界面上提示各输入是否逐项对应
        auto cellCountOf = [](DataObject::Pointer obj) -> IGsize {
            switch (obj->GetDataObjectType()) {
                case IG_POINT_SET:
                    return 0;
                case IG_SURFACE_MESH: {
                    auto mesh = DynamicCast<SurfaceMesh>(obj);
                    return mesh ? mesh->GetNumberOfFaces() : IGsize(0);
                }
                case IG_VOLUME_MESH: {
                    auto mesh = DynamicCast<VolumeMesh>(obj);
                    return mesh ? mesh->GetNumberOfVolumes() : IGsize(0);
                }
                case IG_UNSTRUCTURED_MESH: {
                    auto mesh = DynamicCast<UnstructuredMesh>(obj);
                    return mesh ? mesh->GetNumberOfCells() : IGsize(0);
                }
                case IG_STRUCTURED_MESH: {
                    auto mesh = DynamicCast<StructuredMesh>(obj);
                    return mesh ? mesh->GetNumberOfCells() : IGsize(0);
                }
                default: {
                    auto cells = obj->GetCellArray();
                    return cells ? cells->GetNumberOfCells() : IGsize(0);
                }
            }
        };

        std::vector<AppendInputInfo> infos;
        for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
            auto model = it->second;
            if (!model) { continue; }
            auto dataObj = model->GetDataObject();
            if (!dataObj) { continue; }
            const IGenum type = dataObj->GetDataObjectType();
            if (type != IG_POINT_SET && type != IG_SURFACE_MESH && type != IG_VOLUME_MESH &&
                type != IG_UNSTRUCTURED_MESH && type != IG_STRUCTURED_MESH) {
                continue;
            }
            AppendInputInfo info;
            info.object = dataObj;
            info.name = QString::fromStdString(dataObj->GetName());
            auto pts = dataObj->GetPoints();
            info.points = pts ? pts->GetNumberOfPoints() : IGsize(0);
            info.cells = cellCountOf(dataObj);
            infos.push_back(info);
        }
        if (infos.empty()) {
            showDarkFramelessMessage(QStringLiteral("追加属性"),
                                        QStringLiteral("场景中没有可用于追加属性的数据对象。"));
            return;
        }

        igQtChromeFramelessDialog* dlg = new igQtChromeFramelessDialog(this);
        dlg->setDialogTitle(QStringLiteral("追加属性 (Append Attributes)"));
        dlg->setMaximizeEnabled(false);
        dlg->setAttribute(Qt::WA_DeleteOnClose);

        QWidget* content = new QWidget(dlg->contentHost());
        content->setStyleSheet(
                "QWidget { background-color: #252526; color: #D4D4D4; }"
                "QCheckBox { color: #D4D4D4; spacing: 6px; }"
                "QCheckBox::indicator { width: 16px; height: 16px; }"
                "QCheckBox::indicator:unchecked { border: 1px solid #6A6A6A; background-color: #3A3A3A; }"
                "QCheckBox::indicator:checked { border: 1px solid #0E639C; background-color: #0E639C; }"
                "QCheckBox::indicator:unchecked:hover { border: 1px solid #9A9A9A; }"
                "QCheckBox::indicator:checked:hover { background-color: #1177BB; }"
                "QPushButton { background-color: #3A3A3A; color: #D4D4D4; border: 1px solid #4A4A4A; "
                "              padding: 6px 12px; border-radius: 4px; }"
                "QPushButton:hover { background-color: #4A4A4A; border-color: #5A5A5A; }"
                "QPushButton:pressed { background-color: #2A2A2A; }"
                "QScrollArea { background-color: #1E1E1E; border: none; }"
                "QScrollBar:vertical { background: #1E1E1E; width: 10px; margin: 0; }"
                "QScrollBar::handle:vertical { background: #4A4A4A; border-radius: 5px; min-height: 20px; }"
                "QScrollBar::handle:vertical:hover { background: #5A5A5A; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");
        QVBoxLayout* mainLayout = new QVBoxLayout(content);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);

        QLabel* tip = new QLabel(
                QStringLiteral("选择要合并属性的数据对象（列表顺序即输入顺序，第一个提供输出几何与数据类型）。\n"
                                "要求所选对象的点数和单元数逐项对应；同名的属性会全部保留，"),
                content);
        tip->setWordWrap(true);
        mainLayout->addWidget(tip);

        QHBoxLayout* btnLayout = new QHBoxLayout();
        QPushButton* selectAllBtn = new QPushButton(QStringLiteral("全选"), content);
        QPushButton* deselectAllBtn = new QPushButton(QStringLiteral("全不选"), content);
        btnLayout->addWidget(selectAllBtn);
        btnLayout->addWidget(deselectAllBtn);
        btnLayout->addStretch();
        mainLayout->addLayout(btnLayout);

        QScrollArea* scrollArea = new QScrollArea(content);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        QWidget* listWidget = new QWidget(scrollArea);
        QVBoxLayout* listLayout = new QVBoxLayout(listWidget);
        listLayout->setSpacing(4);
        listLayout->setContentsMargins(0, 0, 0, 0);
        QList<QCheckBox*> checkBoxes;
        for (const AppendInputInfo& info: infos) {
            QCheckBox* cb = new QCheckBox(QStringLiteral("%1  (点: %2, 单元: %3)")
                                                    .arg(info.name)
                                                    .arg(info.points)
                                                    .arg(info.cells),
                                            listWidget);
            cb->setChecked(true);
            checkBoxes.append(cb);
            listLayout->addWidget(cb);
        }
        listWidget->setLayout(listLayout);
        scrollArea->setWidget(listWidget);
        mainLayout->addWidget(scrollArea);

        QCheckBox* pointCb = new QCheckBox(QStringLiteral("追加点属性 (Point Data)"), content);
        pointCb->setChecked(true);
        QCheckBox* cellCb = new QCheckBox(QStringLiteral("追加单元属性 (Cell Data)"), content);
        cellCb->setChecked(true);
        mainLayout->addWidget(pointCb);
        mainLayout->addWidget(cellCb);

        QDialogButtonBox* buttonBox =
                new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, content);
        mainLayout->addWidget(buttonBox);

        dlg->setContentWidget(content);
        dlg->resize(440, 540);

        connect(selectAllBtn, &QPushButton::clicked, [checkBoxes]() {
            for (QCheckBox* cb: checkBoxes) { cb->setChecked(true); }
        });
        connect(deselectAllBtn, &QPushButton::clicked, [checkBoxes]() {
            for (QCheckBox* cb: checkBoxes) { cb->setChecked(false); }
        });
        connect(buttonBox, &QDialogButtonBox::accepted, this, [this, dlg, checkBoxes, infos, pointCb, cellCb]() {
            std::vector<DataObject::Pointer> selected;
            for (int i = 0; i < checkBoxes.size(); ++i) {
                if (checkBoxes[i]->isChecked()) { selected.push_back(infos[i].object); }
            }
            if (selected.empty()) {
                showDarkFramelessMessage(QStringLiteral("追加属性"), QStringLiteral("请至少选择一个数据对象。"));
                return;
            }
            if (!pointCb->isChecked() && !cellCb->isChecked()) {
                showDarkFramelessMessage(QStringLiteral("追加属性"),
                                            QStringLiteral("请至少勾选「点属性」或「单元属性」之一。"));
                return;
            }

            auto filter = AppendAttributes::New();
            for (const auto& obj: selected) { filter->AddInput(obj); }
            filter->SetAppendPointData(pointCb->isChecked());
            filter->SetAppendCellData(cellCb->isChecked());

            if (!filter->Execute()) {
                showDarkFramelessMessage(
                        QStringLiteral("追加属性"),
                        QStringLiteral("执行失败：请确认所选数据对象的点数与单元数逐项对应。"));
                return;
            }
            auto output = filter->GetOutput();
            if (!output) {
                showDarkFramelessMessage(QStringLiteral("追加属性"), QStringLiteral("算法未产生有效结果。"));
                return;
            }
            output->SetName(selected[0]->GetName() + "_appended");
            modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
            rendererWidget->GetScene()->Modified();
            rendererWidget->GetScene()->Update();
            rendererWidget->update();
            dlg->accept();
        });
        connect(buttonBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

        dlg->show();
}
