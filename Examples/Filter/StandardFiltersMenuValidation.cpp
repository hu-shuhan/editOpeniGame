// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/StandardFiltersMenuValidation.cpp
// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/StandardFiltersMenuValidation.cpp
// Regression (fix: feat: integrate second-batch standard filters): the batch-2 standard actions used to show the
// "not integrated" placeholder. Exercise the actual main-window actions,
// parameter-panel reuse and repeated Apply so missing callbacks or duplicate
// model-tree nodes cannot pass just because the backend examples compile.
// 回归（待提交）：当前分支把参数面板放在左侧 Tab，面板本身是 QWidget；
// 验证 QWidget 的复用，不能按旧版独立 QDockWidget 查找导致错误失败。
#include <IQCore/igQtMainWindow.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQWidgets/igQtCountCellVerticesWidget.h>
#include <IQWidgets/igQtLinearExtrusionWidget.h>
#include <IQWidgets/igQtElevationFilterPanel.h>
#include <iGameSurfaceMesh.h>
#include <iGameScene.h>
#include <iGameInteractor.h>
#include <iGameFileIO.h>
#include <QApplication>
#include <QDialog>
#include <QCheckBox>
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

// 回归（待提交）：Warp 输出在只缓存第一个属性范围时，其余范围被错误
// 复制成空数组，点击其他场会访问空内存。验证真实标准菜单 Apply 和
// 模型树 viewAttribute 路径，并检查标量、多分量和张量各分量的范围。
void CheckWarpMenu(igQtMainWindow& window, DataObject::Pointer input) {
    auto* scene = window.rendererWidget->GetScene();
    auto attrs = input->GetAttributeSet();
    Check(attrs && attrs->GetNumberOfAttributes() >= 2, "warp fixture needs multiple fields");
    for (IGsize a = 0; a < attrs->GetNumberOfAttributes(); ++a) attrs->GetAttribute(a).dataRange = nullptr;
    attrs->GetAttribute(0).GetDataRange();
    input->SetName("WarpAttributeSelectionFixture");
    const auto id = window.modelTreeWidget->addDataObjectToModelTree(input, Algorithm);
    scene->SetCurrentModel(scene->GetModelById(id));
    window.findChild<QAction*>("action_filter_warp_by_scalar")->trigger();
    igQtFilterDialogDockWidget* panel = nullptr;
    for (auto* candidate : window.findChildren<igQtFilterDialogDockWidget*>()) {
        // Warp's first parameter is its scalar-array combo; its description
        // distinguishes it from other parameter docks created by this test.
        if (candidate->isVisible()) panel = candidate;
    }
    Check(panel, "warp action did not open its parameter dock");
    panel->apply();
    auto result = scene->GetCurrentModel()->GetDataObject();
    Check(result != input, "warp Apply did not create a result");
    auto* item = window.modelTreeWidget->getItemFromObject(result);
    Check(item, "warp result missing from model tree");
    auto outputAttrs = result->GetAttributeSet();
    Check(outputAttrs->GetNumberOfAttributes() == attrs->GetNumberOfAttributes(), "warp lost fields");
    for (IGsize a = 0; a < outputAttrs->GetNumberOfAttributes(); ++a) {
        auto& attribute = outputAttrs->GetAttribute(a);
        std::cerr << "Checking warp tree selection " << attribute.pointer->GetName() << "...\n";
        item->setCurrentChild(item->child(static_cast<int>(a)));
        item->viewAttribute(static_cast<int>(a), -1);
        for (int dim = 0; dim < attribute.pointer->GetDimension(); ++dim) {
            item->viewAttribute(static_cast<int>(a), dim);
            auto range = attribute.GetDataRange();
            Check(range && range->GetNumberOfElements() == attribute.pointer->GetDimension() + 1,
                  "warp selected attribute has an invalid range");
        }
    }
}

// 回归（待提交）：PR #22 把轴开关创建、注册/解绑和渲染信号接线追加到
// 函数外，合并无文本冲突但无法编译。检查 UI 创建和实际会话注册、端点
// 同步、重新绑定及解绑；否则仅移动语句就可能留下无法显示/清理的轴。
void CheckElevationAxisSession(igQtMainWindow& window, DataObject::Pointer input) {
    auto* scene = window.rendererWidget->GetScene();
    auto interactor = scene->GetInteractor();
    auto* panel = window.findChild<igQtElevationFilterPanel*>();
    Check(panel && interactor, "missing elevation panel or interactor");
    auto* checkbox = panel->findChild<QCheckBox*>();
    Check(checkbox && !checkbox->isChecked(), "missing/default-enabled Show Axis control");
    auto filter = ElevationFilter::New();
    filter->SetInput(input);
    filter->SetLowPoint(0, 0, 0);
    filter->SetHighPoint(1, 0, 0);
    Check(filter->Execute(), "elevation fixture execution failed");
    window.modelTreeWidget->addDataObjectToModelTree(filter->GetOutput(), Algorithm);
    panel->BindSession(input, filter);
    auto style = DynamicCast<LowHighAxisStyle>(interactor->GetSpecialInteractor("ElevationAxis"));
    Check(style && style->GetLowPoint() == Point(0, 0, 0) &&
          style->GetHighPoint() == Point(1, 0, 0), "elevation axis did not register/sync");
    checkbox->setChecked(true);
    Check(checkbox->isChecked(), "Show Axis toggle failed");
    Point low(0, 0, 0), high(0, 1, 0);
    Check(QMetaObject::invokeMethod(panel, "onAxisDragChanged", Qt::DirectConnection,
          Q_ARG(iGame::Point, low), Q_ARG(iGame::Point, high)), "axis drag callback missing");
    Check(QMetaObject::invokeMethod(panel, "onApply", Qt::DirectConnection), "elevation Apply missing");
    Check(style->GetHighPoint() == high, "Apply did not synchronize axis endpoints");
    panel->BindSession(input, filter);
    Check(!checkbox->isChecked(), "rebinding kept the old visible-axis state");
    panel->UnbindSession();
    Check(!interactor->GetSpecialInteractor("ElevationAxis"), "unbinding left the axis interactor registered");
    checkbox->setChecked(true);
    Check(!interactor->GetSpecialInteractor("ElevationAxis"), "unbound checkbox registered an axis");
    checkbox->setChecked(false);
    std::cerr << "PASS: elevation axis UI, binding, Apply and cleanup\n";
}
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
        // 回归（待提交）：新增标准入口和 append_datasets 更名必须保留真实 QAction。
        // 防止只迁移开发菜单而遗漏标准菜单；已有输入下 outline 与挤出必须产生结果。
        const QStringList migratedIds = {
            "rename_arrays", "append_arc_length", "surface_vectors", "perlin_noise",
            "outline", "reverse_sense", "warp_by_scalar", "warp_by_vector",
            "texture_map_to_plane", "texture_map_to_cylinder", "texture_map_to_sphere",
            "decimate_polyline", "histogram", "compute_quartiles", "integrate_variables",
            "append_attributes", "compute_connected_surface_properties", "generate_surface_tangents",
            "connectivity", "extract_selection", "tube", "ribbon", "smooth", "subdivide",
            "linear_extrusion", "tensor_principal_invariants", "yield_criteria", "group_datasets",
            "extract_block", "extract_time_steps", "temporal_shift_scale", "extract_enclosed_points",
            "resample_with_dataset", "median", "temporal_statistics", "validate_cells",
            "extract_cells_by_type", "extract_location", "append_datasets"};
        for (const auto& id : migratedIds) {
            auto* action = window.findChild<QAction*>("action_filter_" + id);
            Check(action && action->data().toString() == id, "missing or misidentified migrated standard action");
        }
        Check(!window.findChild<QAction*>("action_filter_append_reduce"), "old append_reduce standard identifier remains");
        auto* renamed = window.findChild<QAction*>("action_filter_append_datasets");
        Check(renamed->text().contains("append_datasets"), "append_datasets display name not updated");
        const QStringList panelIds={"count_cell_vertices","extract_edges","global_point_and_cell_ids",
            "point_and_cell_ids","process_ids","axis_aligned_reflection","extract_component",
            "merge_vector_components","resample_to_image","resample_to_line","triangle_strips"};
        // Regression (b23c0dbb): removing unfinished filter implementations also
        // removed their menu actions. Keep both actions visible so they use the
        // standard "not integrated" message until their algorithms are ready.
        // Fix commit: Restore unfinished filter menu actions; find it with
        // git log --format="%h %s" -- Examples/Filter/StandardFiltersMenuValidation.cpp
        for (const auto& id : {"feature_edges_region_ids", "point_set_to_octree_image"}) {
            Check(window.findChild<QAction*>(QStringLiteral("action_filter_") + id),
                  "unfinished filter action disappeared from the menu");
        }
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
        CheckElevationAxisSession(window, mesh);
        for(const auto& id:panelIds) {
            std::cerr << "Checking action " << id.toStdString() << "...\n";
            scene->SetCurrentModel(scene->GetModelById(modelId));
            auto* action=window.findChild<QAction*>("action_filter_"+id);
            Check(action,"missing standard filter action");
            action->trigger();
            auto* dock=window.findChild<QWidget*>("standardFilterPanel_"+id);
            Check(dock,"standard action did not open its parameter panel");
            action->trigger();
            Check(window.findChildren<QWidget*>("standardFilterPanel_"+id).size()==1,
                  "reopening an action duplicated its panel");
            dock->hide();
        }
        scene->SetCurrentModel(scene->GetModelById(modelId));
        std::cerr << "Checking reflection alias...\n";
        window.findChild<QAction*>("action_filter_reflect")->trigger();
        Check(window.findChildren<QWidget*>("standardFilterPanel_axis_aligned_reflection").size()==1,
              "reflect alias did not reuse the reflection panel");
        std::cerr << "Checking cell_size execution...\n";
        window.findChild<QAction*>("action_filter_cell_size")->trigger();
        Check(!scene->GetCurrentModel()->GetDataObject()->GetAttributeSet()->GetAttribute("Area").IsNone(),
              "cell_size action did not produce attributes");
        Check(mesh->GetAttributeSet()->GetAttribute("Area").IsNone(),"menu action modified input");
        scene->SetCurrentModel(scene->GetModelById(modelId));
        std::cerr << "Checking repeated count Apply...\n";
        window.findChild<QAction*>("action_filter_count_cell_vertices")->trigger();
        auto* panel=window.findChild<igQtCountCellVerticesWidget*>();
        panel->ExecuteCount();
        auto* tree=window.findChild<igQtModelTreeWidget*>();
        Check(tree,"missing model tree");
        const int count=tree->topLevelItemCount();
        panel->ExecuteCount();
        Check(tree->topLevelItemCount()==count,"repeated Apply added duplicate output nodes");
        std::cerr << "Checking outline execution...\n";
        window.findChild<QAction*>("action_filter_outline")->trigger();
        auto outline = DynamicCast<UnstructuredMesh>(scene->GetCurrentModel()->GetDataObject());
        Check(outline && outline->GetNumberOfPoints() == 8 && outline->GetNumberOfCells() == 12,
              "standard outline action did not produce bounding-box geometry");
        scene->SetCurrentModel(scene->GetModelById(modelId));
        std::cerr << "Checking linear_extrusion execution...\n";
        window.findChild<QAction*>("action_filter_linear_extrusion")->trigger();
        auto* extrusion = window.findChild<igQtLinearExtrusionWidget*>();
        Check(extrusion, "standard linear_extrusion action did not create its panel");
        extrusion->Apply();
        Check(scene->GetCurrentModel()->GetDataObject() != mesh,
              "standard linear_extrusion Apply did not insert a result");
        extrusion->window()->hide();
        // Keep a small fixture in the normal regression; a CLI path allows
        // verification with the user's Quad_Plane_Tensor.vtk without copying it.
        auto warpFixture = SurfaceMesh::New();
        warpFixture->SetPoints(points);
        warpFixture->SetFaces(faces);
        const char* names[] = {"test_1", "test_2", "Position", "Curvature", "TEST_TENSOR"};
        const int dims[] = {3, 1, 3, 1, 9};
        for (int a = 0; a < 5; ++a) {
            auto values = FloatArray::New();
            values->SetName(names[a]);
            values->SetDimension(dims[a]);
            values->Resize(3);
            for (IGsize v = 0; v < values->GetNumberOfValues(); ++v) values->SetValue(v, float(v + 1));
            warpFixture->GetAttributeSet()->AddAttribute(a == 4 ? IG_TENSOR : IG_SCALAR, IG_POINT, values);
        }
        CheckWarpMenu(window, warpFixture);
        if (argc > 1) {
            auto userModel = FileIO::ReadFile(argv[1]);
            Check(userModel != nullptr, "cannot load the user warp model");
            CheckWarpMenu(window, userModel);
        }
        Check(!unexpectedDialog,"menu action produced an unexpected error dialog");
        std::cout<<"PASS: standard menu actions, reflection alias, result insertion and repeated Apply.\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1;
    }
}
