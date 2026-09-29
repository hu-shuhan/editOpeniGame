#include <IQCore/igQtMainWindow.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <AppendLocationAttribute/iGameAppendLocationAttribute.h>
#include <BoundaryMeshQuality/iGameBoundaryMeshQualityFilter.h>
#include <Convert/iGameConvertToSurfaceMeshFilter.h>
#include <Convert/iGamePointSetToOctreeFilter.h>
#include <DataProcessing/iGameMeshTetrahedralize.h>
#include <DataProcessing/iGameVolumeMeshSimplification.h>
#include <FeatureExtraction/iGameCountCellFacesFilter.h>
#include <FeatureExtraction/iGameFeatureEdgeRegionFilter.h>
#include <GenerateIds/iGameGenerateIdsFilter.h>
#include <Interpolation/iGamePointVolumeInterpolatorFilter.h>
#include <MeshQuality/iGameMeshQualityFilter.h>
#include <MyFilter/iGameCleanToGridFilter.h>
#include <Periodic/iGameAngularPeriodicFilter.h>
#include <PointLineInterpolator/iGamePointLineInterpolatorFilter.h>
#include <Shrink/iGameShrinkFilter.h>
#include <SurfaceNormals/iGameSurfaceNormalsFilter.h>
#include <Threshold/iGameThresholdFilter.h>
#include <iGameScene.h>
#include <IQWidgets/igQtRenderWidget.h>
#include <IQWidgets/igQtCharts.h>
#include <iGameSelection.h>
#include <QHideEvent>
#include <QShowEvent>
#include <QPointer>
#include <QSignalBlocker>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>

using namespace iGame;

namespace {
// A panel keeps a smart reference to its selected input. Applying a filter may
// change the current model, but subsequent Apply clicks still use this input.
class Batch3Panel : public QDockWidget {
public:
    std::function<void()> activatePreview;
    std::function<void()> deactivatePreview;
    ~Batch3Panel() override { if (deactivatePreview) deactivatePreview(); }
    void showEvent(QShowEvent* event) override {
        QDockWidget::showEvent(event);
        if (activatePreview) activatePreview();
    }
    void hideEvent(QHideEvent* event) override {
        if (deactivatePreview) deactivatePreview();
        QDockWidget::hideEvent(event);
    }
    QFormLayout* form;
    QLabel* status;
    QPushButton* apply;
    Batch3Panel(QWidget* parent, const QString& title, const QString& id) : QDockWidget(title, parent) {
        setObjectName("thirdBatchFilterPanel_" + id);
        setAttribute(Qt::WA_DeleteOnClose);
        auto* scroll = new QScrollArea(this);
        auto* body = new QWidget(scroll);
        form = new QFormLayout(body);
        scroll->setWidget(body);
        scroll->setWidgetResizable(true);
        setWidget(scroll);
        status = new QLabel(body);
        status->setObjectName("filterStatus");
        status->setWordWrap(true);
        apply = new QPushButton(QStringLiteral("应用"), body);
        apply->setObjectName("applyFilter");
    }
    template<class T> T* row(const char* id, const QString& title, T* widget) {
        widget->setObjectName(id);
        form->addRow(title, widget);
        return widget;
    }
    QDoubleSpinBox* number(const char* id, const QString& title, double value,
                           double low = -1e12, double high = 1e12) {
        auto* w = new QDoubleSpinBox;
        w->setDecimals(8); w->setRange(low, high); w->setValue(value);
        return row(id, title, w);
    }
    QSpinBox* integer(const char* id, const QString& title, int value, int low, int high) {
        auto* w = new QSpinBox; w->setRange(low, high); w->setValue(value);
        return row(id, title, w);
    }
    QCheckBox* check(const char* id, const QString& title, bool value) {
        auto* w = new QCheckBox; w->setChecked(value); return row(id, title, w);
    }
    QComboBox* choice(const char* id, const QString& title, const QStringList& items) {
        auto* w = new QComboBox; w->addItems(items); return row(id, title, w);
    }
    QLineEdit* text(const char* id, const QString& title, const QString& value) {
        return row(id, title, new QLineEdit(value));
    }
};

DataObject::Pointer surfaceInput(DataObject::Pointer input) {
    if (DynamicCast<SurfaceMesh>(input)) return input;
    auto conversion = ConvertToSurfaceMeshFilter::New();
    conversion->SetConvertMethod(ConvertToSurfaceMeshFilter::IG_EXTRACT_SURFACE_MESH);
    conversion->SetInput(input);
    if (!conversion->Execute() || !conversion->GetOutput())
        throw std::runtime_error("当前模型无法提取表面网格。");
    return conversion->GetOutput();
}

struct ArrayChoice {
    ArrayObject::Pointer array;
    IGenum association;
};

std::vector<ArrayChoice> arrayChoices(DataObject::Pointer input, QComboBox* combo, bool pointsOnly) {
    std::vector<ArrayChoice> arrays;
    auto* attributes = input->GetAttributeSet();
    if (!attributes) return arrays;
    auto all = attributes->GetAllAttributes();
    for (int i = 0; i < all->GetNumberOfElements(); ++i) {
        auto a = all->GetElement(i);
        if (a.isDeleted || !a.pointer || (a.attachmentType != IG_POINT && a.attachmentType != IG_CELL) ||
            (pointsOnly && a.attachmentType != IG_POINT)) continue;
        combo->addItem(QString::fromStdString(a.pointer->GetName()) +
                       (a.attachmentType == IG_POINT ? QStringLiteral("（点）") : QStringLiteral("（单元）")));
        arrays.push_back({a.pointer, a.attachmentType});
    }
    return arrays;
}
} // namespace

bool igQtMainWindow::connectThirdBatchFilterAction(QAction* action, const QString& id) {
    const QStringList ids = {"angular_periodic", "append_location_attributes", "boundary_mesh_quality",
        "clean_to_grid", "clean_poly_data", "clean_cells_to_grid", "count_cell_faces",
        "feature_edges_region_ids", "generate_ids", "mesh_quality", "point_line_interpolator",
        "point_set_to_octree_image", "point_volume_interpolator", "shrink", "surface_normals",
        "threshold", "volume_mesh_simplification", "mesh_tetrahedralize"};
    // Probe and probe_location share the interactive probe panel in the main window.
    if (!ids.contains(id)) return false;
    connect(action, &QAction::triggered, this, [this, action, id]() {
        auto* scene = rendererWidget->GetScene();
        auto model = scene ? scene->GetCurrentModel() : nullptr;
        auto input = model ? model->GetDataObject() : nullptr;
        if (!input || !DynamicCast<PointSet>(input)) {
            showDarkFramelessMessage(action->text(), QStringLiteral("请先加载并选择一个点集或网格。"));
            return;
        }
        if (auto* old = findChild<QDockWidget*>("thirdBatchFilterPanel_" + id)) delete old;
        auto* panel = new Batch3Panel(this, action->text(), id);
        panel->form->addRow(QStringLiteral("输入"), new QLabel(QString::fromStdString(input->GetName())));
        std::function<DataObject::Pointer()> run;
        std::function<void(DataObject::Pointer)> presentResult;
        auto execute = [input](auto filter) -> DataObject::Pointer {
            filter->SetInput(input);
            if (!filter->Execute() || !filter->GetOutput())
                throw std::runtime_error("执行失败，请检查模型类型、属性及参数。");
            return filter->GetOutput();
        };

        if (id == "append_location_attributes") {
            run = [=]() { return execute(AppendLocationAttribute::New()); };
        } else if (id == "count_cell_faces") {
            run = [=]() { return execute(CountCellFacesFilter::New()); };
        } else if (id == "angular_periodic") {
            std::array<QDoubleSpinBox*, 3> origin, axis;
            const char* axes[] = {"X", "Y", "Z"};
            for (int i = 0; i < 3; ++i) {
                origin[i] = panel->number(qPrintable(QString("origin%1").arg(i)), QStringLiteral("轴心 ") + axes[i], 0);
                axis[i] = panel->number(qPrintable(QString("axis%1").arg(i)), QStringLiteral("方向 ") + axes[i], i == 2 ? 1 : 0);
            }
            auto* angle = panel->number("angle", QStringLiteral("周期角度（度）"), 90, 0.000001, 360);
            auto* copies = panel->integer("copies", QStringLiteral("总份数（含原模型）"), 4, 1, 10000);
            auto* mode = panel->choice("mode", QStringLiteral("份数模式"), {QStringLiteral("指定份数"), QStringLiteral("一周内最大份数")});
            auto* full = panel->check("fullPeriod", QStringLiteral("要求整周闭合"), false);
            run = [=]() {
                auto f = AngularPeriodicFilter::New();
                f->SetRotationAxis(Point(origin[0]->value(), origin[1]->value(), origin[2]->value()),
                                   Vector3d(axis[0]->value(), axis[1]->value(), axis[2]->value()));
                f->SetAngle(angle->value()); f->SetNumberOfCopies(copies->value());
                f->SetIterationMode(mode->currentIndex()); f->SetRequireFullPeriod(full->isChecked());
                f->SetInput(input);
                if (!f->Execute()) throw std::runtime_error(f->GetMessage());
                return f->GetOutput();
            };
        } else if (id.startsWith("clean_")) {
            auto* absolute = panel->check("absolute", QStringLiteral("使用绝对容差"), true);
            auto* tolerance = panel->number("tolerance", QStringLiteral("容差（相对时为包围盒比例）"), 0, 0);
            auto* merge = panel->check("mergePoints", QStringLiteral("合并重合点"), id != "clean_cells_to_grid");
            auto* unused = panel->check("removeUnused", QStringLiteral("移除未使用的点"), true);
            auto* degenerate = panel->check("removeDegenerate", QStringLiteral("移除退化单元"), true);
            run = [=]() {
                auto f = CleanToGridFilter::New();
                f->SetToleranceIsAbsolute(absolute->isChecked());
                f->SetAbsoluteTolerance(tolerance->value()); f->SetToleranceFraction(tolerance->value());
                f->SetMergePoints(merge->isChecked()); f->SetRemoveUnusedPoints(unused->isChecked());
                f->SetRemoveDegenerateCells(degenerate->isChecked());
                f->SetInput(id == "clean_poly_data" ? surfaceInput(input) : input);
                if (!f->Execute()) throw std::runtime_error("网格清理失败。");
                return id == "clean_poly_data" ? surfaceInput(f->GetOutput()) : f->GetOutput();
            };
        } else if (id == "boundary_mesh_quality") {
            auto* metric = panel->choice("metric", QStringLiteral("质量指标"), {
                QStringLiteral("单元中心到边界面中心距离"), QStringLiteral("单元中心到边界平面距离"),
                QStringLiteral("面法向与中心连线夹角")});
            run = [=]() {
                auto f = BoundaryMeshQualityFilter::New();
                f->SetBoundaryMetric(static_cast<BoundaryMeshQualityFilter::BoundaryMetric>(metric->currentIndex()));
                return execute(f);
            };
        } else if (id == "feature_edges_region_ids" || id == "surface_normals") {
            auto* angle = panel->number("featureAngle", QStringLiteral("特征角（度）"), 30, 0, 180);
            if (id == "feature_edges_region_ids") {
                run = [=]() {
                    auto f = FeatureEdgeRegionFilter::New(); f->SetInput(surfaceInput(input));
                    f->SetFeatureAngle(angle->value());
                    if (!f->Execute()) throw std::runtime_error("特征区域编号失败。");
                    return f->GetOutput();
                };
            } else {
                auto* points = panel->check("pointNormals", QStringLiteral("计算点法向量"), true);
                auto* cells = panel->check("cellNormals", QStringLiteral("计算面法向量"), true);
                auto* split = panel->check("splitting", QStringLiteral("拆分锐边顶点"), true);
                auto* flip = panel->check("flip", QStringLiteral("翻转法向量"), false);
                auto* consistent = panel->check("consistency", QStringLiteral("统一面绕序"), true);
                run = [=]() {
                    auto f = SurfaceNormalsFilter::New(); f->SetInput(surfaceInput(input));
                    f->SetFeatureAngle(angle->value()); f->SetComputePointNormals(points->isChecked());
                    f->SetComputeCellNormals(cells->isChecked()); f->SetSplitting(split->isChecked());
                    f->SetFlipNormals(flip->isChecked()); f->SetConsistency(consistent->isChecked());
                    if (!f->Execute()) throw std::runtime_error("法向量计算失败。");
                    return f->GetOutput();
                };
            }
        } else if (id == "generate_ids") {
            auto* association = panel->choice("association", QStringLiteral("属性位置"), {QStringLiteral("点"), QStringLiteral("单元")});
            auto* name = panel->text("arrayName", QStringLiteral("数组名称"), "Ids");
            auto* start = panel->text("startId", QStringLiteral("起始 ID（64 位整数）"), "0");
            run = [=]() {
                bool ok; auto value = start->text().toLongLong(&ok);
                if (!ok || name->text().trimmed().isEmpty()) throw std::runtime_error("请输入数组名称和有效的整数 ID。");
                auto f = iGameGenerateIdsFilter::New(association->currentIndex() == 0 ? IG_POINT : IG_CELL);
                f->SetArrayName(name->text().trimmed().toStdString()); f->SetStartId(value);
                return execute(f);
            };
        } else if (id == "mesh_quality") {
            auto* triangles = panel->choice("triangles", QStringLiteral("三角形指标"), {"Area", "Max angle", "Min angle", "Jacobian", "Aspect ratio", "Edge ratio"});
            auto* quads = panel->choice("quads", QStringLiteral("四边形指标"), {"Area", "Max angle", "Min angle", "Jacobian", "Aspect ratio", "Edge ratio", "Warpage", "Taper", "Skew"});
            auto* tets = panel->choice("tetrahedra", QStringLiteral("四面体指标"), {"Edge ratio", "Volume", "Aspect ratio", "Jacobian", "Collapse ratio", "Volume skew", "Min angle", "Equiangle skewness", "Inradius", "Circumradius", "Volume aspect ratio"});
            auto* hexes = panel->choice("hexahedra", QStringLiteral("六面体指标"), {"Volume", "Taper", "Jacobian", "Edge ratio", "Max edge ratio", "Skew", "Stretch", "Diagonal", "Relative size squared"});
            hexes->setCurrentIndex(3);
            run = [=]() {
                auto f = MeshQualityFilter::New();
                f->SetTriangleMetric(static_cast<SurfaceMeshMetricsFilter::SurfaceMetric>(triangles->currentIndex()));
                f->SetQuadMetric(static_cast<SurfaceMeshMetricsFilter::SurfaceMetric>(quads->currentIndex()));
                f->SetTetMetric(static_cast<VolumeMeshMetricsFilter::VolumeMetric>(tets->currentIndex()));
                f->SetHexMetric(static_cast<VolumeMeshMetricsFilter::VolumeMetric>(VolumeMeshMetricsFilter::HEX_VOLUME + hexes->currentIndex()));
                return execute(f);
            };
        } else if (id == "point_line_interpolator" || id == "point_volume_interpolator") {
            const bool volume = id == "point_volume_interpolator";
            auto box = input->GetBoundingBox();
            std::array<QDoubleSpinBox*, 6> bounds;
            for (int i = 0; i < 6; ++i) {
                QString title = volume ? QStringLiteral("采样范围 ") : QStringLiteral("线端点 ");
                title += QString("%1 %2").arg(i < 3 ? 1 : 2).arg(QString("XYZ")[i % 3]);
                bounds[i] = panel->number(qPrintable(QString("bound%1").arg(i)), title,
                                          i < 3 ? box.min[i] : box.max[i - 3]);
            }
            if (!volume) {
                // Reuse the line interactor: both endpoints and the line itself
                // can be dragged, with coordinates synchronized in both directions.
                auto selection = LineSelection::New();
                const auto sync = [selection, bounds, this]() {
                    selection->Orig = Vector3d(bounds[0]->value(), bounds[1]->value(), bounds[2]->value());
                    selection->Target = Vector3d(bounds[3]->value(), bounds[4]->value(), bounds[5]->value());
                    selection->UpdateLine();
                    rendererWidget->update();
                };
                sync();
                QPointer<Batch3Panel> guard(panel);
                selection->SetSelectionCallBackEvent(
                    [guard, lineSelection = selection.GetPointer(), bounds, this](IGenum type, const std::vector<igIndex>&, Selection::Operate) {
                        if (type != IG_CHANGE || !guard) return;
                        for (int i = 0; i < 6; ++i) {
                            QSignalBlocker blocker(bounds[i]);
                            bounds[i]->setValue(i < 3 ? lineSelection->Orig[i] : lineSelection->Target[i - 3]);
                        }
                        rendererWidget->update();
                    }, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
                for (auto* coordinate : bounds)
                    connect(coordinate, qOverload<double>(&QDoubleSpinBox::valueChanged), panel,
                            [sync](double) { sync(); });
                QPointer<igQtRenderWidget> renderGuard(rendererWidget);
                panel->activatePreview = [scene, model, input, selection, renderGuard]() {
                    if (!renderGuard) return;
                    auto interactor = scene->GetInteractor();
                    interactor->SetDataObject(input);
                    interactor->SetPainter3D(model->GetPainter3D());
                    interactor->RequestResampleToLineStyle(selection);
                    renderGuard->update();
                };
                panel->deactivatePreview = [scene, selection, renderGuard]() {
                    if (!renderGuard) return;
                    // Clear the style's callback before destroying its preview.
                    selection->SetUpdateFunction([]() {});
                    scene->GetInteractor()->RequestBasicStyle();
                    renderGuard->update();
                };
                panel->form->addRow(new QLabel(QStringLiteral("拖动端点调整位置，拖动线段整体平移。")));
            }
            auto* useBounds = volume ? panel->check("inputBounds", QStringLiteral("使用输入包围盒"), true) : nullptr;
            std::array<QSpinBox*, 3> resolution{};
            for (int i = 0; i < (volume ? 3 : 1); ++i)
                resolution[i] = panel->integer(qPrintable(QString("resolution%1").arg(i)),
                    volume ? QStringLiteral("网格点数 ") + QString("XYZ")[i] : QStringLiteral("线段数"), volume ? 32 : 100, volume ? 2 : 1, volume ? 512 : 100000);
            QStringList kernels = {"Voronoi", "Gaussian", "Shepard"};
            if (volume) kernels << "Linear";
            auto* kernel = panel->choice("kernel", QStringLiteral("插值核"), kernels);
            auto* footprint = panel->choice("footprint", QStringLiteral("邻域"), {QStringLiteral("半径"), QStringLiteral("最近 N 点")});
            auto* radius = panel->number("radius", QStringLiteral("半径"), std::max(0.001, static_cast<double>(box.diag()) / 10), 0.00000001);
            auto* count = panel->integer("neighbors", QStringLiteral("邻点数"), 8, 1, 100000);
            auto* sharpness = panel->number("sharpness", "Gaussian sharpness", 2, 0.00000001);
            auto* power = panel->number("power", "Shepard power", 2, 0.00000001);
            auto* nulls = panel->choice("nullStrategy", QStringLiteral("空邻域处理"), {QStringLiteral("掩码"), QStringLiteral("空值"), QStringLiteral("最近点")});
            nulls->setCurrentIndex(2);
            auto* nullValue = panel->number("nullValue", QStringLiteral("空值"), 0);
            if (!volume) {
                auto* showChart = panel->check("showChart", QStringLiteral("显示沿线曲线"), false);
                auto* plotArray = panel->choice("plotArray", QStringLiteral("曲线数组"), {});
                const auto plotChoices = arrayChoices(input, plotArray, true);
                auto* plotComponent = panel->choice("plotComponent", QStringLiteral("数组分量"), {});
                const auto updateComponents = [plotChoices, plotComponent](int index) {
                    plotComponent->clear();
                    if (index < 0 || index >= static_cast<int>(plotChoices.size())) return;
                    for (int c = 0; c < plotChoices[index].array->GetDimension(); ++c)
                        plotComponent->addItem(QStringLiteral("分量 %1").arg(c));
                };
                connect(plotArray, qOverload<int>(&QComboBox::currentIndexChanged), panel, updateComponents);
                updateComponents(plotArray->currentIndex());
                const bool hasArrays = !plotChoices.empty();
                showChart->setChecked(hasArrays);
                showChart->setEnabled(hasArrays);
                plotArray->setEnabled(hasArrays);
                plotComponent->setEnabled(hasArrays);
                presentResult = [this, showChart, plotArray, plotComponent, plotChoices](DataObject::Pointer output) {
                    if (!showChart->isChecked()) return;
                    const int index = plotArray->currentIndex();
                    if (index < 0 || index >= static_cast<int>(plotChoices.size())) return;
                    auto& attribute = output->GetAttributeSet()->GetAttribute(plotChoices[index].array->GetName());
                    auto line = DynamicCast<PointSet>(output);
                    if (!line || attribute.IsNone() || !attribute.pointer) return;
                    std::vector<double> distances(line->GetNumberOfPoints(), 0.0);
                    for (IGsize i = 1; i < line->GetNumberOfPoints(); ++i)
                        distances[i] = distances[i - 1] + std::sqrt((line->GetPoint(i) - line->GetPoint(i - 1)).squaredLength());
                    auto* chart = new igQtCharts(this);
                    chart->setObjectName("pointLineInterpolatorChart");
                    chart->setAttribute(Qt::WA_DeleteOnClose);
                    chart->drawLineChart(attribute.pointer, distances, plotComponent->currentIndex(), QStringLiteral("沿线距离"));
                    chart->show(); chart->raise(); chart->activateWindow();
                };
            }
            run = [=]() -> DataObject::Pointer {
                if (volume) {
                    if (static_cast<qint64>(resolution[0]->value()) * resolution[1]->value() * resolution[2]->value() > 16000000)
                        throw std::runtime_error("网格过大，请降低采样点数（最多 1600 万点）。");
                    auto f = PointVolumeInterpolatorFilter::New();
                    f->SetResolution(resolution[0]->value(), resolution[1]->value(), resolution[2]->value());
                    f->SetUseInputBounds(useBounds->isChecked());
                    f->SetSamplingBounds(bounds[0]->value(), bounds[3]->value(), bounds[1]->value(), bounds[4]->value(), bounds[2]->value(), bounds[5]->value());
                    f->SetKernelType(static_cast<PointKernelType>(kernel->currentIndex()));
                    f->SetKernelFootprint(static_cast<PointKernelFootprint>(footprint->currentIndex()));
                    f->SetRadius(radius->value()); f->SetNumberOfPoints(count->value());
                    f->SetSharpness(sharpness->value()); f->SetPowerParameter(power->value());
                    f->SetNullPointsStrategy(static_cast<PointNullPointsStrategy>(nulls->currentIndex())); f->SetNullValue(nullValue->value());
                    return execute(f);
                }
                auto f = PointLineInterpolatorFilter::New();
                f->SetPoint1(Point(bounds[0]->value(), bounds[1]->value(), bounds[2]->value()));
                f->SetPoint2(Point(bounds[3]->value(), bounds[4]->value(), bounds[5]->value()));
                f->SetResolution(resolution[0]->value());
                f->SetKernelType(static_cast<PointLineInterpolatorFilter::KernelType>(kernel->currentIndex()));
                f->SetKernelFootprint(static_cast<PointLineInterpolatorFilter::KernelFootprint>(footprint->currentIndex()));
                f->SetRadius(radius->value()); f->SetNumberOfPoints(count->value());
                f->SetSharpness(sharpness->value()); f->SetPowerParameter(power->value());
                f->SetNullPointsStrategy(static_cast<PointLineInterpolatorFilter::NullPointsStrategy>(nulls->currentIndex())); f->SetNullValue(nullValue->value());
                return execute(f);
            };
        } else if (id == "point_set_to_octree_image") {
            auto* count = panel->integer("pointsPerCell", QStringLiteral("每体素平均点数"), 1, 1, 100000000);
            count->setToolTip(QStringLiteral("控制网格划分的目标疏密，不是每个体素实际包含的点数；实际数量由 Count 统计。"));
            auto* inherit = panel->check("inheritDisplayAttribute", QStringLiteral("继承输入显示属性"), false);
            inherit->setToolTip(QStringLiteral("独立于统计选项。开启后，将输入显示属性的体素均值映射到输出网格点，生成“原属性名_体素均值映射”用于着色；它不是原始属性。"));
            auto* process = panel->check("processArray", QStringLiteral("处理点属性数组"), false);
            process->setToolTip(QStringLiteral("勾选后，对所选点标量在各体素内计算统计值；未勾选时统计函数不生效。"));
            auto* arrays = panel->choice("array", QStringLiteral("点标量属性"), {});
            auto choices = arrayChoices(input, arrays, true);
            // Match the original panel: the algorithm accepts scalar tuples only.
            for (int i = static_cast<int>(choices.size()) - 1; i >= 0; --i) {
                if (choices[i].array->GetDimension() != 1 ||
                    choices[i].array->GetNumberOfElements() != DynamicCast<PointSet>(input)->GetNumberOfPoints()) {
                    choices.erase(choices.begin() + i);
                    arrays->removeItem(i);
                }
            }
            arrays->setToolTip(QStringLiteral("仅支持单分量点属性；多分量位移、应力数组需要先提取所需标量分量。"));
            std::array<QCheckBox*, 6> stats;
            const char* names[] = {"Last", "Min", "Max", "Count", "Sum", "Mean"};
            const QStringList labels = {
                QStringLiteral("Last — 最后处理的点值"), QStringLiteral("Min — 最小值"),
                QStringLiteral("Max — 最大值"), QStringLiteral("Count — 输入点数"),
                QStringLiteral("Sum — 数值之和"), QStringLiteral("Mean — 平均值")};
            const QStringList tips = {
                QStringLiteral("该体素内最后处理到的输入点的属性值，取决于处理顺序，不代表最大值。"),
                QStringLiteral("该体素内所选点属性的最小值。"),
                QStringLiteral("该体素内所选点属性的最大值。"),
                QStringLiteral("落入该体素的输入点数量，不是属性值，也不是输出网格点数。"),
                QStringLiteral("该体素内所选点属性值的总和。"),
                QStringLiteral("该体素内所选点属性值的平均值（Sum / Count）；会同时计算并输出 Count、Sum。")};
            for (int i = 0; i < 6; ++i) {
                stats[i] = panel->check(names[i], labels[i], i != 0 && i != 4);
                stats[i]->setToolTip(tips[i]);
            }
            auto* info = new QLabel;
            info->setObjectName("octreeInfo");
            info->setWordWrap(true);
            info->setTextInteractionFlags(Qt::TextSelectableByMouse);
            panel->form->addRow(info);
            const auto refreshInfo = [=]() {
                const bool enabled = process->isChecked();
                arrays->setEnabled(enabled);
                for (auto* stat : stats) stat->setEnabled(enabled);
                QStringList lines;
                const auto points = DynamicCast<PointSet>(input)->GetNumberOfPoints();
                lines << QStringLiteral("输入点数：%1；目标体素数约 %2。实际网格维度按包围盒各轴尺寸分配。")
                    .arg(static_cast<qulonglong>(points))
                    .arg(static_cast<qulonglong>(points / count->value()));
                if (!enabled) {
                    lines << QStringLiteral("未处理点属性数组：统计函数不参与计算。勾选后才能选择属性和统计函数。");
                } else if (choices.empty()) {
                    lines << QStringLiteral("当前模型没有可用的单分量点属性，请取消处理点属性数组或更换数据。");
                } else {
                    QStringList functions;
                    if (stats[0]->isChecked()) functions << "Last";
                    if (stats[1]->isChecked()) functions << "Min";
                    if (stats[2]->isChecked()) functions << "Max";
                    if (stats[3]->isChecked() || stats[5]->isChecked()) functions << "Count";
                    if (stats[4]->isChecked() || stats[5]->isChecked()) functions << "Sum";
                    if (stats[5]->isChecked()) functions << "Mean";
                    if (functions.empty()) {
                        lines << QStringLiteral("请至少选择一个统计函数。");
                    } else {
                        lines << QStringLiteral("单元统计数组：%1；分量顺序：%2。")
                            .arg(QString::fromStdString(choices[arrays->currentIndex()].array->GetName()), functions.join(" / "));
                    }
                    if (stats[5]->isChecked())
                        lines << QStringLiteral("Mean 需要 Count 和 Sum，二者即使未勾选也会同时输出。");
                }
                if (inherit->isChecked()) {
                    lines << QStringLiteral("已开启显示属性继承：优先沿用输入当前的点显示属性，无活动点属性时尝试 RGB 或非恒定点标量。生成“原属性名_体素均值映射”，属于平均后的派生点属性，不代表原始数据。");
                } else {
                    lines << QStringLiteral("未继承输入显示属性；未处理点属性数组时，输出只有 octree。开启统计后只增加所选统计数组。");
                }
                lines << QStringLiteral("统计针对每个体素内所选点属性的数值，不改变 octree 占用编码或网格形状。应用后，在新结果模型中选择统计数组及对应分量查看。");
                info->setText(lines.join("\n\n"));
            };
            connect(inherit, &QCheckBox::toggled, panel, [refreshInfo](bool) { refreshInfo(); });
            connect(process, &QCheckBox::toggled, panel, [refreshInfo](bool) { refreshInfo(); });
            connect(arrays, qOverload<int>(&QComboBox::currentIndexChanged), panel, [refreshInfo](int) { refreshInfo(); });
            connect(count, qOverload<int>(&QSpinBox::valueChanged), panel, [refreshInfo](int) { refreshInfo(); });
            for (auto* stat : stats)
                connect(stat, &QCheckBox::toggled, panel, [refreshInfo](bool) { refreshInfo(); });
            refreshInfo();
            run = [=]() {
                auto f = PointSetToOctreeFilter::New(); f->SetNumberOfPointsPerCell(count->value());
                f->SetProcessInputPointArray(process->isChecked());
                f->SetInheritDisplayAttribute(inherit->isChecked());
                if (process->isChecked()) {
                    if (arrays->currentIndex() < 0) throw std::runtime_error("模型没有可统计的点属性。");
                    f->SetInputPointArrayName(choices[arrays->currentIndex()].array->GetName());
                }
                f->SetComputeLastValue(stats[0]->isChecked()); f->SetComputeMin(stats[1]->isChecked());
                f->SetComputeMax(stats[2]->isChecked()); f->SetComputeCount(stats[3]->isChecked());
                f->SetComputeSum(stats[4]->isChecked()); f->SetComputeMean(stats[5]->isChecked());
                f->SetInput(input);
                if (!f->Execute()) throw std::runtime_error(f->GetMessage().empty() ? "转换失败，请检查模型及参数。" : f->GetMessage());
                panel->setProperty("resultDiagnostic", QString::fromStdString(f->GetMessage()));
                return f->GetOutput();
            };
        } else if (id == "shrink") {
            auto* factor = panel->number("factor", QStringLiteral("收缩系数"), 0.5, 0, 1);
            run = [=]() { auto f = ShrinkFilter::New(); f->SetShrinkFactor(factor->value()); return execute(f); };
        } else if (id == "threshold") {
            auto* arrays = panel->choice("array", QStringLiteral("阈值属性"), {});
            auto choices = arrayChoices(input, arrays, false);
            auto* component = panel->integer("component", QStringLiteral("分量（-1 为模长）"), 0, -1, 0);
            auto* lower = panel->number("lower", QStringLiteral("下界"), 0);
            auto* upper = panel->number("upper", QStringLiteral("上界"), 1);
            auto refresh = [=](int index) {
                if (index < 0) return;
                auto a = choices[index].array;
                component->setMaximum(a->GetDimension() - 1);
                double lo = std::numeric_limits<double>::infinity(), hi = -lo;
                for (IGsize i = 0; i < a->GetNumberOfElements(); ++i) {
                    double v = a->GetElementValue(i, component->value());
                    if (std::isfinite(v)) { lo = std::min(lo, v); hi = std::max(hi, v); }
                }
                if (lo <= hi) { lower->setValue(lo); upper->setValue(hi); }
            };
            connect(arrays, QOverload<int>::of(&QComboBox::currentIndexChanged), panel, refresh);
            connect(component, QOverload<int>::of(&QSpinBox::valueChanged), panel, [=](int) { refresh(arrays->currentIndex()); });
            refresh(arrays->currentIndex());
            auto* boundary = panel->choice("boundary", QStringLiteral("区间边界"), {"[min, max]", "(min, max)", "[min, max)", "(min, max]"});
            auto* evaluation = panel->choice("evaluation", QStringLiteral("点属性单元判定"), {QStringLiteral("全部顶点通过"), QStringLiteral("任一顶点通过")});
            run = [=]() {
                if (arrays->currentIndex() < 0) throw std::runtime_error("模型没有可用属性。");
                const auto a = choices[arrays->currentIndex()];
                auto f = ThresholdFilter::New();
                f->SetScalarData(a.array, a.association == IG_POINT ? ThresholdFilter::Association::Point : ThresholdFilter::Association::Cell, component->value());
                f->SetThreshold(lower->value(), upper->value());
                f->SetBoundaryMode(static_cast<ThresholdFilter::BoundaryMode>(boundary->currentIndex()));
                f->SetPointEvaluation(static_cast<ThresholdFilter::PointEvaluation>(evaluation->currentIndex()));
                return execute(f);
            };
        } else if (id == "mesh_tetrahedralize") {
            run = [=]() {
                auto f = MeshTetrahedralize::New(); f->SetInput(input);
                if (!f->Execute()) throw std::runtime_error(f->m_failReason);
                return f->GetOutput();
            };
        } else if (id == "volume_mesh_simplification") {
            auto* reduction = panel->number("reduction", QStringLiteral("目标简化比例"), 0.5, 0, 0.9999);
            auto* count = panel->integer("targetCount", QStringLiteral("目标四面体数（0 使用比例）"), 0, 0, 100000000);
            auto* boundary = panel->check("preserveBoundary", QStringLiteral("保留边界"), true);
            auto* attrs = panel->check("allAttributes", QStringLiteral("使用全部点属性"), true);
            run = [=]() {
                auto tetra = MeshTetrahedralize::New(); tetra->SetInput(input);
                if (!tetra->Execute()) throw std::runtime_error(tetra->m_failReason);
                auto f = TetraEdgeSimplification::New(); f->SetInput(tetra->GetOutput());
                f->SetTargetReduction(reduction->value()); f->SetTargetTetraCount(count->value());
                f->SetPreserveBoundary(boundary->isChecked()); f->SetUseAllPointAttributes(attrs->isChecked());
                if (!f->Execute()) throw std::runtime_error("体网格简化失败。");
                return f->GetOutput();
            };
        }
        panel->form->addRow(panel->status);
        panel->form->addRow(panel->apply);
        connect(panel->apply, &QPushButton::clicked, panel, [this, panel, run, presentResult, input, id]() {
            panel->apply->setEnabled(false);
            try {
                auto output = run();
                if (!output) throw std::runtime_error("未生成输出模型。");
                output->SetName(input->GetName() + "_" + id.toStdString());
                modelTreeWidget->addDataObjectToModelTree(output, Algorithm);
                if (auto draw = DynamicCast<DrawObject>(output)) draw->ForceReConvertToDrawableData();
                rendererWidget->update();
                if (presentResult) presentResult(output);
                panel->status->setText(QStringLiteral("完成：已添加结果模型。") +
                    (id == "point_set_to_octree_image" ? "\n" + panel->property("resultDiagnostic").toString() : QString()));
                panel->setProperty("lastApplySucceeded", true);
            } catch (const std::exception& error) {
                panel->status->setText(QString::fromUtf8(error.what()));
                panel->setProperty("lastApplySucceeded", false);
            }
            panel->apply->setEnabled(true);
        });
        addDockWidget(Qt::RightDockWidgetArea, panel);
        panel->show(); panel->raise(); resizeDocks({panel}, {440}, Qt::Horizontal);
    });
    return true;
}
