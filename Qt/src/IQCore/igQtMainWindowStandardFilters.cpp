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
    static const QStringList ids = {
        "cell_size", "count_cell_vertices", "extract_edges",
        "global_point_and_cell_ids", "point_and_cell_ids", "process_ids", "reflect",
        "axis_aligned_reflection", "extract_component", "merge_vector_components",
        "resample_to_image", "resample_to_line", "triangle_strips",
        "resample_with_dataset", "rename_arrays", "texture_map_to_cylinder", "median"};
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
