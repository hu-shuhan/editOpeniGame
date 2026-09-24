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
#include <cmath>

using namespace iGame;

namespace {
// Reuse the parameter session; install result connections only on creation.
template<class Panel, class Setup, class... Args>
Panel* filterPanel(igQtMainWindow* window, const QString& id, const QString& title,
                   Setup setup, Args... args) {
    const QString name = QStringLiteral("standardFilterPanel_") + id;
    auto* dock = window->findChild<QDockWidget*>(name);
    if (!dock) {
        dock = new QDockWidget(title, window);
        dock->setObjectName(name);
        dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                          QDockWidget::DockWidgetFloatable);
        auto* panel = new Panel(args..., dock);
        auto* scroll = new QScrollArea(dock);
        scroll->setWidgetResizable(true);
        scroll->setWidget(panel);
        dock->setWidget(scroll);
        window->addDockWidget(Qt::RightDockWidgetArea, dock);
        setup(panel, dock);
    }
    return dock->findChild<Panel*>();
}

void showFilterPanel(igQtMainWindow* window, const QString& id) {
    auto* dock = window->findChild<QDockWidget*>(QStringLiteral("standardFilterPanel_") + id);
    if (!dock) return;
    dock->show();
    dock->raise();
    window->resizeDocks({dock}, {440}, Qt::Horizontal);
}
} // namespace

bool igQtMainWindow::connectImportedFilterAction(QAction* action, const QString& filterId) {
    static const QStringList ids = {
        "cell_size", "count_cell_vertices", "extract_edges",
        "global_point_and_cell_ids", "point_and_cell_ids", "process_ids", "reflect",
        "axis_aligned_reflection", "extract_component", "merge_vector_components",
        "resample_to_image", "resample_to_line", "triangle_strips"};
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
            auto* panel = filterPanel<igQtCountCellVerticesWidget>(this, id, title, [&](auto* p, auto*) {
                connect(p, &igQtCountCellVerticesWidget::DrawCountModel, this, displayResult);
                connect(p, &igQtCountCellVerticesWidget::UpdateCountModel, this, displayResult);
            });
            panel->SetOriginDataObject(input);
        } else if (id == "extract_edges") {
            auto* panel = filterPanel<igQtExtractEdgesWidget>(this, id, title, [&](auto* p, auto*) {
                connect(p, &igQtExtractEdgesWidget::DrawEdgesModel, this, displayResult);
                connect(p, &igQtExtractEdgesWidget::UpdateEdgesModel, this, displayResult);
            });
            panel->SetOriginDataObject(input);
        } else if (id == "extract_component") {
            auto* panel = filterPanel<igQtExtractComponentWidget>(this, id, title, [&](auto* p, auto*) {
                connect(p, &igQtExtractComponentWidget::DrawExtractComponentModel, this, displayResult);
                connect(p, &igQtExtractComponentWidget::UpdateExtractComponentModel, this, displayResult);
                connect(p, &igQtExtractComponentWidget::ApplyFailed, this, failed);
            });
            panel->SetOriginDataObject(input);
        } else if (id == "process_ids") {
            auto* panel = filterPanel<igQtGenerateProcessIdsWidget>(this, id, title, [&](auto* p, auto*) {
                connect(p, &igQtGenerateProcessIdsWidget::DrawProcessIdsModel, this, displayResult);
                connect(p, &igQtGenerateProcessIdsWidget::UpdateProcessIdsModel, this, displayResult);
                connect(p, &igQtGenerateProcessIdsWidget::ApplyFailed, this, failed);
            });
            panel->SetOriginDataObject(input);
        } else if (id == "merge_vector_components") {
            auto* panel = filterPanel<igQtMergeVectorComponentsWidget>(this, id, title, [&](auto* p, auto*) {
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
            auto* panel = filterPanel<igQtPointAndCellIdsWidget>(this, id, title, [&](auto* p, auto* dock) {
                connect(p, &igQtPointAndCellIdsWidget::idsGenerated, this, displayResult);
                connect(p, &igQtPointAndCellIdsWidget::cancelRequested, dock, &QDockWidget::hide);
            });
            panel->setCurrentModel(model);
        } else if (id == "global_point_and_cell_ids") {
            auto* panel = filterPanel<igQtGlobalIdWidget>(this, id, title, [&](auto* p, auto* dock) {
                connect(p, &igQtGlobalIdWidget::resultReady, this, displayResult);
                connect(p, &igQtGlobalIdWidget::cancelRequested, dock, &QDockWidget::hide);
            });
            panel->setCurrentModel(model);
        } else if (id == "resample_to_image") {
            auto* panel = filterPanel<igQtResampleToImageWidget>(this, id, title, [&](auto* p, auto* dock) {
                connect(p, &igQtResampleToImageWidget::resultReady, this, displayResult);
                connect(p, &igQtResampleToImageWidget::closeRequested, dock, &QDockWidget::hide);
            });
            panel->setCurrentModel(model);
        } else if (id == "triangle_strips") {
            auto* panel = filterPanel<igQtTriangleStripWidget>(this, id, title, [&](auto* p, auto*) {
                connect(p, &igQtTriangleStripWidget::resultReady, this,
                        [=](DataObject::Pointer surface, DataObject::Pointer lines) {
                    displayResult(surface);
                    displayResult(lines);
                });
            });
            if (!panel->isOutput(input)) panel->setInput(input);
        } else if (id == "axis_aligned_reflection") {
            auto* panel = filterPanel<igQtAxisAlignedReflectionWidget>(this, id, title, [&](auto* p, auto*) {
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
            auto* panel = filterPanel<igQtResampleToLine>(this, id, title, [&](auto* p, auto* dock) {
                connect(p, &igQtResampleToLine::ResetInteractor, dock, &QDockWidget::hide);
            }, modelTreeWidget);
            panel->BindCurrentModel();
        }
        showFilterPanel(this, id);
    });
    return true;
}
