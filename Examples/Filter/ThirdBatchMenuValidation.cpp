// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/ThirdBatchMenuValidation.cpp
// Regression: third-batch actions were placeholders, while legacy tetra/simplify
// actions dereferenced an absent selection. Exercise actual menu callbacks and
// Apply on a volume fixture, including surface conversion, aliases, retained input,
// output insertion and parameter errors. Numerical details are covered separately.
// Integration fix: feat: integrate third-batch standard filters.
#include <IQCore/igQtMainWindow.h>
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <iGameScene.h>
#include <iGameInteractor.h>
#include <iGameUnstructuredMesh.h>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QLabel>
#include <IQWidgets/igQtProbeWidget.h>
#include <QPushButton>
#include <QTableWidget>
#include <IQWidgets/igQtCharts.h>
#include <QtCharts/QLineSeries>
#include <QCheckBox>
#include <QShowEvent>
#include <QHideEvent>
#include <QSettings>
#include <QTimer>
#include <iostream>
#include <cmath>
#include <stdexcept>

using namespace iGame;
namespace {
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
class MenuRenderer : public igQtModelDrawWidget {
public:
    explicit MenuRenderer(QWidget* parent) : igQtModelDrawWidget(parent) {
        m_Scene = SceneManager::Instance()->NewScene();
        m_Interactor = Interactor::New(); m_Interactor->Initialize(m_Scene);
        m_Scene->SetInteractor(m_Interactor);
    }
};
}
int main(int argc, char** argv) {
    Q_INIT_RESOURCE(iGameQtMainWindow);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("iGameTests");
    QCoreApplication::setApplicationName("ThirdBatchMenuValidation");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::currentPath()+"/batch3-menu-settings");
    try {
        igQtMainWindow window;
        window.rendererWidget = new MenuRenderer(&window);
        auto* scene = window.rendererWidget->GetScene();
        auto input = UnstructuredMesh::New(); input->SetName("Batch3Cube");
        const Point points[] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
        for (auto p : points) input->GetPoints()->AddPoint(p);
        igIndex ids[] = {0,1,2,3,4,5,6,7}; input->AddCell(ids, 8, IG_HEXAHEDRON);
        auto field = DoubleArray::New(); field->SetName("Field"); field->SetDimension(1);
        for (auto p : points) field->AddValue(p[0] + p[1] + p[2]);
        input->GetAttributeSet()->AddScalar(IG_POINT, field);
        const auto modelId = window.modelTreeWidget->addDataObjectToModelTree(input, Algorithm);
        const QStringList filters = {"angular_periodic", "append_location_attributes", "boundary_mesh_quality",
            "clean_to_grid", "clean_poly_data", "clean_cells_to_grid", "count_cell_faces",
            "feature_edges_region_ids", "generate_ids", "mesh_quality", "point_line_interpolator",
            "point_set_to_octree_image", "point_volume_interpolator", "shrink", "surface_normals",
            "threshold", "mesh_tetrahedralize", "volume_mesh_simplification"};
        bool unexpectedDialog = false;
        QTimer guard;
        QObject::connect(&guard, &QTimer::timeout, [&]() {
            for (auto* w : QApplication::topLevelWidgets())
                if (auto* d = qobject_cast<QDialog*>(w); d && d->isModal() && d->isVisible()) {
                    bool completed = false;
                    for (auto* label : d->findChildren<QLabel*>())
                        completed |= label->text().contains(QStringLiteral("Filter 执行完成。"));
                    if (!completed) unexpectedDialog = true;
                    d->accept();
                }
        });
        guard.start(50);
        for (const auto& id : filters) {
            std::cerr << "Apply " << id.toStdString() << '\n';
            scene->SetCurrentModel(scene->GetModelById(modelId));
            auto* action = window.findChild<QAction*>("action_filter_" + id);
            Check(action, "missing third-batch action"); action->trigger();
            action->trigger(); // Reopening must replace, not accumulate, panels.
            auto* panel = window.findChild<QDockWidget*>("thirdBatchFilterPanel_" + id);
            Check(panel, "action did not open parameter panel");
            Check(window.findChildren<QDockWidget*>(panel->objectName()).size() == 1, "duplicate panels");
            // Regression (fix pending commit): opening the interpolation panel
            // must activate the line preview; hiding it must remove that style.
            if (id == "point_line_interpolator") {
                QShowEvent shown;
                QApplication::sendEvent(panel, &shown);
                Check(!scene->GetInteractor()->IsBasicStyle(), "line preview interactor was not activated");
            }
            auto* button = panel->findChild<QPushButton*>("applyFilter");
            Check(button, "missing Apply button"); button->click();
            if (!panel->property("lastApplySucceeded").toBool()) {
                std::cerr << panel->findChild<QLabel*>("filterStatus")->text().toStdString() << '\n';
                throw std::runtime_error("menu Apply failed");
            }
            auto output = scene->GetCurrentModel()->GetDataObject();
            Check(output && output != input, "filter did not insert independent output");
            Check(input->GetNumberOfPoints() == 8 && input->GetNumberOfCells() == 1, "filter changed input topology");
            Check(input->GetAttributeSet()->GetNumberOfAttributes() == 1, "filter added attributes to source");
            if (id == "shrink") {
                button->click();
                Check(panel->property("lastApplySucceeded").toBool(), "repeated Apply failed");
                auto out = DynamicCast<PointSet>(scene->GetCurrentModel()->GetDataObject());
                Check(std::abs(out->GetPoint(0)[0] - 0.25) < 1e-6, "Apply used previous output as its input");
            }
            if (id == "threshold") {
                panel->findChild<QDoubleSpinBox*>("lower")->setValue(10);
                panel->findChild<QDoubleSpinBox*>("upper")->setValue(-10);
                button->click();
                Check(!panel->property("lastApplySucceeded").toBool(), "invalid threshold range accepted");
            }
            // Regression (fix pending commit / 待提交): display inheritance
            // must be an independent, initially unchecked choice in the panel.
            if (id == "point_set_to_octree_image") {
                auto* inherit = panel->findChild<QCheckBox*>("inheritDisplayAttribute");
                auto* process = panel->findChild<QCheckBox*>("processArray");
                Check(inherit && !inherit->isChecked() && !process->isChecked(), "octree inheritance/statistics defaults were not off");
                Check(output->GetAttributeSet()->GetNumberOfAttributes() == 1, "default octree UI copied an unrequested input field");
                inherit->setChecked(true);
                button->click();
                auto mapped = scene->GetCurrentModel()->GetDataObject();
                Check(!mapped->GetAttributeSet()->GetAttribute("Field_体素均值映射").IsNone(), "UI inheritance choice did not reach the filter");
                Check(!process->isChecked(), "display inheritance unexpectedly enabled statistics");
            }
            // Regression (fix pending commit): imported point-line UI omitted
            // the chart popup and distance/component overload. Verify default
            // popup, physical distance instead of sample index, and opt-out.
            if (id == "point_line_interpolator") {
                auto* curve = window.findChild<igQtCharts*>("pointLineInterpolatorChart");
                Check(curve && curve->isVisible(), "point-line Apply did not show its chart");
                auto* series = qobject_cast<QLineSeries*>(curve->getChartView()->chart()->series().front());
                Check(series && series->count() == 101, "point-line chart has wrong sample count");
                Check(std::abs(series->at(100).x() - std::sqrt(3.0)) < 1e-5,
                      "point-line chart plotted indices instead of physical distance");
                Check(std::abs(series->at(0).y()) < 1e-5 && std::abs(series->at(100).y() - 3.0) < 1e-5,
                      "point-line chart did not plot interpolated values");
                delete curve;
                panel->findChild<QCheckBox*>("showChart")->setChecked(false);
                button->click();
                Check(!window.findChild<igQtCharts*>("pointLineInterpolatorChart"), "disabled chart still opened");
            }
            panel->hide();
            if (id == "point_line_interpolator") {
                QHideEvent hidden;
                QApplication::sendEvent(panel, &hidden);
                Check(scene->GetInteractor()->IsBasicStyle(), "line preview interactor was not cleaned up");
            }
        }
        // Regression (fix pending commit): the aliases used separate stripped
        // dialogs and omitted the sphere widget. Both must reuse one full panel.
        QDockWidget* probePanel = nullptr;
        for (const auto& id : {"probe", "probe_location"}) {
            scene->SetCurrentModel(scene->GetModelById(modelId));
            auto* action = window.findChild<QAction*>(QString("action_filter_") + id);
            Check(action, "missing probe alias"); action->trigger();
            auto* dock = window.findChild<QDockWidget*>("probeFilterPanel");
            Check(dock && qobject_cast<igQtProbeWidget*>(dock->widget()), "probe did not open sphere panel");
            Check(!probePanel || probePanel == dock, "probe aliases created separate panels");
            probePanel = dock;
            Check(QMetaObject::invokeMethod(dock->widget(), "onProbeClicked", Qt::DirectConnection),
                  "probe action could not execute");
            auto* table = dock->widget()->findChild<QTableWidget*>();
            Check(table && table->rowCount() == 1 && table->columnCount() >= 6,
                  "probe did not populate interpolated field and validity mask");
            Check(scene->GetCurrentModel()->GetDataObject() == input, "probe changed the selected source");
        }
        // A vector must plot the chosen tuple component, rather than flattening
        // components into samples. Replotting must replace the previous axes.
        igQtCharts componentChart;
        auto vector = DoubleArray::New(); vector->SetName("Vector"); vector->SetDimension(3);
        vector->AddElement3(1, 10, 100); vector->AddElement3(2, 20, 200);
        componentChart.drawLineChart(vector, {0.0, 2.5}, 2, QStringLiteral("沿线距离"));
        auto* vectorSeries = qobject_cast<QLineSeries*>(componentChart.getChartView()->chart()->series().front());
        Check(vectorSeries && vectorSeries->count() == 2 && vectorSeries->at(1) == QPointF(2.5, 200),
              "curve ignored selected vector component or distance");
        componentChart.drawLineChart(vector, {0.0, 2.5}, 1, QStringLiteral("沿线距离"));
        Check(componentChart.getChartView()->chart()->axes().size() == 2, "replot left duplicate axes");
        Check(!unexpectedDialog, "unexpected modal error");
        std::cout << "PASS: 17 filter groups / 20 menu entries, output and input retention.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
