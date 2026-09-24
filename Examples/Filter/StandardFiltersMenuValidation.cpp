// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/StandardFiltersMenuValidation.cpp
// Regression (fix: feat: integrate second-batch standard filters): the batch-2 standard actions used to show the
// "not integrated" placeholder. Exercise the actual main-window actions,
// parameter-panel reuse and repeated Apply so missing callbacks or duplicate
// model-tree nodes cannot pass just because the backend examples compile.
#include <IQCore/igQtMainWindow.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQWidgets/igQtCountCellVerticesWidget.h>
#include <iGameSurfaceMesh.h>
#include <iGameScene.h>
#include <iGameInteractor.h>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QDockWidget>
#include <QSettings>
#include <QTimer>
#include <iostream>
#include <stdexcept>

using namespace iGame;
namespace {
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
// Menu wiring only needs CPU scene/model state. The production renderer
// initializes that state in initializeGL, which offscreen Qt does not call.
class MenuTestRenderer : public igQtModelDrawWidget {
public:
    explicit MenuTestRenderer(QWidget* parent) : igQtModelDrawWidget(parent) {
        m_Scene=SceneManager::Instance()->NewScene();
        m_Interactor=Interactor::New();
        m_Interactor->Initialize(m_Scene);
        m_Scene->SetInteractor(m_Interactor);
    }
};
}
int main(int argc,char** argv) {
    Q_INIT_RESOURCE(iGameQtMainWindow);
    QApplication app(argc,argv);
    QCoreApplication::setOrganizationName("iGameTests");
    QCoreApplication::setApplicationName("StandardFiltersMenuValidation");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,QDir::currentPath()+"/menu-test-settings");
    try {
        igQtMainWindow window;
        window.rendererWidget=new MenuTestRenderer(&window);
        auto* scene=window.rendererWidget->GetScene();
        Check(scene,"main window did not create a scene");
        auto mesh=SurfaceMesh::New(); mesh->SetName("MenuFixture");
        auto points=Points::New();
        points->AddPoint(Point(0,0,0)); points->AddPoint(Point(1,0,0)); points->AddPoint(Point(0,1,0));
        mesh->SetPoints(points);
        igIndex ids[]={0,1,2}; auto faces=CellArray::New(); faces->AddCellIds(ids,3); mesh->SetFaces(faces);
        const auto modelId=window.modelTreeWidget->addDataObjectToModelTree(mesh,Algorithm);
        const QStringList panelIds={"count_cell_vertices","extract_edges","global_point_and_cell_ids",
            "point_and_cell_ids","process_ids","axis_aligned_reflection","extract_component",
            "merge_vector_components","resample_to_image","resample_to_line","triangle_strips",
            "point_set_to_octree_image"};
        // Dismiss unexpected modal error dialogs and fail instead of hanging.
        bool unexpectedDialog=false;
        QTimer guard;
        QObject::connect(&guard,&QTimer::timeout,[&]() {
            for(auto* widget:QApplication::topLevelWidgets()) {
                auto* dialog=qobject_cast<QDialog*>(widget);
                if(dialog && dialog->isModal() && dialog->isVisible()) {
                    unexpectedDialog=true; dialog->reject();
                }
            }
        });
        guard.start(50);
        for(const auto& id:panelIds) {
            std::cerr << "Checking action " << id.toStdString() << "...\n";
            scene->SetCurrentModel(scene->GetModelById(modelId));
            auto* action=window.findChild<QAction*>("action_filter_"+id);
            Check(action,"missing standard filter action");
            action->trigger();
            auto* dock=window.findChild<QDockWidget*>("standardFilterPanel_"+id);
            Check(dock,"standard action did not open its parameter panel");
            action->trigger();
            Check(window.findChildren<QDockWidget*>("standardFilterPanel_"+id).size()==1,
                  "reopening an action duplicated its panel");
            dock->hide();
        }
        scene->SetCurrentModel(scene->GetModelById(modelId));
        window.findChild<QAction*>("action_filter_reflect")->trigger();
        Check(window.findChildren<QDockWidget*>("standardFilterPanel_axis_aligned_reflection").size()==1,
              "reflect alias did not reuse the reflection panel");
        window.findChild<QAction*>("action_filter_cell_size")->trigger();
        Check(!scene->GetCurrentModel()->GetDataObject()->GetAttributeSet()->GetAttribute("Area").IsNone(),
              "cell_size action did not produce attributes");
        Check(mesh->GetAttributeSet()->GetAttribute("Area").IsNone(),"menu action modified input");
        scene->SetCurrentModel(scene->GetModelById(modelId));
        window.findChild<QAction*>("action_filter_feature_edges_region_ids")->trigger();
        auto* regions=window.findChild<igQtFilterDialogDockWidget*>("standardFilterParameters_feature_edges_region_ids");
        Check(regions,"feature region action did not create parameters");
        regions->apply();
        Check(!scene->GetCurrentModel()->GetDataObject()->GetAttributeSet()->GetAttribute("Region Id").IsNone(),
              "feature region action did not connect its result");
        scene->SetCurrentModel(scene->GetModelById(modelId));
        window.findChild<QAction*>("action_filter_count_cell_vertices")->trigger();
        auto* panel=window.findChild<igQtCountCellVerticesWidget*>();
        panel->ExecuteCount();
        auto* tree=window.findChild<igQtModelTreeWidget*>();
        Check(tree,"missing model tree");
        const int count=tree->topLevelItemCount();
        panel->ExecuteCount();
        Check(tree->topLevelItemCount()==count,"repeated Apply added duplicate output nodes");
        Check(!unexpectedDialog,"menu action produced an unexpected error dialog");
        std::cout<<"PASS: standard menu actions, reflection alias, result insertion and repeated Apply.\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1;
    }
}
