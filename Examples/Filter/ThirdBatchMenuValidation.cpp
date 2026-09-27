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
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <iostream>
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
            panel->hide();
        }
        for (const auto& id : {"probe", "probe_location"}) {
            scene->SetCurrentModel(scene->GetModelById(modelId));
            auto old = window.findChildren<igQtFilterDialogDockWidget*>();
            auto* action = window.findChild<QAction*>(QString("action_filter_") + id);
            Check(action, "missing probe alias"); action->trigger();
            igQtFilterDialogDockWidget* panel = nullptr;
            for (auto* candidate : window.findChildren<igQtFilterDialogDockWidget*>())
                if (!old.contains(candidate)) panel = candidate;
            Check(panel, "probe did not open parameters"); panel->apply();
            auto out = scene->GetCurrentModel()->GetDataObject();
            Check(out != input && !out->GetAttributeSet()->GetAttribute("Field").IsNone(), "probe did not interpolate field");
        }
        Check(!unexpectedDialog, "unexpected modal error");
        std::cout << "PASS: 17 filter groups / 20 menu entries, output and input retention.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
