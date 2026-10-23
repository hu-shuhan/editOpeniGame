// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
// Regression (fix pending commit): moving region widgets synchronously scanned
// every cell and highlighted the selection, freezing the UI. Verify drags leave
// selection untouched, all three widgets capture only left drags, misses rotate
// the scene, and Apply computes/reuses a result including the Plane half-space.
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQCore/igQtMainWindow.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <iGameBoxStyle.h>
#include <iGameInteractor.h>
#include <iGameRadialStyle.h>
#include <iGameRegionPlaneStyle.h>
#include <iGameScene.h>
#include <iGameUnstructuredMesh.h>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <iostream>

static igm::vec2 Project(iGame::Interactor* interactor, const iGame::Point& p) {
    const auto clip = interactor->GetMVP() * igm::vec4(p[0], p[1], p[2], 1.f);
    return {(clip.x / clip.w + 1.f) * interactor->GetWidth() * .5f,
            (1.f - clip.y / clip.w) * interactor->GetHeight() * .5f};
}
static void Drag(iGame::Interactor* i, igm::vec2 p, igm::vec2 delta,
                 iGame::MouseButton button = iGame::LeftButton) {
    i->FilterEvent({iGame::IEvent::MouseMove, iGame::NoButton, p, 0.});
    i->FilterEvent({iGame::IEvent::MousePress, button, p, 0.});
    i->FilterEvent({iGame::IEvent::MouseMove, button, p + delta, 0.});
    i->FilterEvent({iGame::IEvent::MouseRelease, button, p + delta, 0.});
}
static double MatrixDistance(const igm::mat4& a, const igm::mat4& b) {
    double d = 0.;
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) d += std::abs(a[i][j] - b[i][j]);
    return d;
}
int main(int argc, char** argv) {
    // Load the same static resources as the application.
    Q_INIT_RESOURCE(iGameQtMainWindow);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
        QDir::current().absoluteFilePath(".region-ui-settings"));
    QApplication app(argc, argv);
    igQtMainWindow window;
    window.resize(1280, 900);
    window.show();
    app.processEvents();
    auto mesh = iGame::UnstructuredMesh::New();
    auto points = iGame::Points::New();
    for (double x : {-2., -.5, .5, 2.}) {
        const igIndex base = points->GetNumberOfPoints();
        points->AddPoint(iGame::Vector3d(x, -.1, -.1));
        points->AddPoint(iGame::Vector3d(x + .1, -.1, -.1));
        points->AddPoint(iGame::Vector3d(x, .1, -.1));
        points->AddPoint(iGame::Vector3d(x, 0., .1));
        igIndex cell[] = {base, base + 1, base + 2, base + 3};
        mesh->AddCell(cell, 4, IG_TETRA);
    }
    mesh->SetPoints(points);
    mesh->SetName("RegionInteractionRegression");
    window.modelTreeWidget->addDataObjectToModelTree(mesh, File);
    auto scene = window.rendererWidget->GetScene();
    auto interactor = scene->GetInteractor();
    app.processEvents();
    auto selection = scene->GetCurrentModel()->GetSelection();
    selection->SelectionCallBackEvent(IG_CELL, std::vector<igIndex>{0}, iGame::Selection::Operate::Add);
    const auto selectedBefore = selection->GetSelectedCells();
    auto* menu = window.findChild<QMenu*>(QStringLiteral("menu_developing_filters"));
    QAction* action = nullptr;
    for (auto* candidate : window.findChildren<QAction*>())
        if (candidate->data().toString() == "extract_cells_by_region") action = candidate;
    if (!action) return 1;
    action->trigger();
    app.processEvents();
    igQtFilterDialogDockWidget* dialog = nullptr;
    for (auto* d : window.findChildren<igQtFilterDialogDockWidget*>())
        if (d->windowTitle() == QStringLiteral("按区域提取单元")) dialog = d;
    if (!dialog) return 2;
    auto* preview = dialog->findChild<QPushButton*>("previewButton");
    if (!preview || preview->isVisible()) { std::cerr << "Unexpected region preview\n"; return 3; }
    auto* shape = qobject_cast<QComboBox*>(dialog->getWidget(0));
    if (!shape || shape->count() != 3 || shape->itemText(2) != "Plane") return 4;
    auto edit = [&](int id, double value) {
        auto* field = qobject_cast<QLineEdit*>(dialog->getWidget(id));
        field->setText(QString::number(value, 'g', 12));
    };
    auto applyEdits = [&](int id) { QMetaObject::invokeMethod(dialog->getWidget(id), "editingFinished"); app.processEvents(); };
    auto unchangedSelection = [&]() { return selection->GetSelectedCells() == selectedBefore; };
    const auto cameraBefore = (interactor->GetCamera()->GetViewMatrix() * scene->GetModelMatrix());
    auto box = iGame::DynamicCast<iGame::BoxStyle>(interactor->GetSpecialInteractor("SelectBox"));
    if (!box) return 5;
    const auto lengthBefore = box->GetBox()->GetLength();
    Drag(interactor, Project(interactor, box->GetBox()->GetOpePoints()[3]), {35.f, 0.f});
    app.processEvents();
    if ((box->GetBox()->GetLength() - lengthBefore).length() < 1.e-6 ||
        MatrixDistance(cameraBefore, (interactor->GetCamera()->GetViewMatrix() * scene->GetModelMatrix())) > 1.e-5 || !unchangedSelection()) {
        std::cerr << "Box left resize moved camera or selection\n"; return 6;
    }
    const auto rotationBefore = box->GetBox()->GetRotationMatrix();
    const auto boxCenter = box->GetBox()->GetMidPoint();
    const auto boxSize = box->GetBox()->GetLength();
    Drag(interactor, Project(interactor, boxCenter + iGame::Point(boxSize[0] * .2, boxSize[1] * .2, boxSize[2] * .5)), {22.f, 18.f});
    if (MatrixDistance(rotationBefore, box->GetBox()->GetRotationMatrix()) < 1.e-6 ||
        MatrixDistance(cameraBefore, (interactor->GetCamera()->GetViewMatrix() * scene->GetModelMatrix())) > 1.e-5) {
        std::cerr << "Box face left drag did not rotate the widget\n"; return 20;
    }
    if (argc > 1) window.grab().save(QString::fromLocal8Bit(argv[1]) + "-box.png");
    shape->setCurrentIndex(1);
    app.processEvents();
    auto sphere = iGame::DynamicCast<iGame::RadialStyle>(interactor->GetSpecialInteractor("ExtractCellsByRegionSphere"));
    if (!sphere || interactor->HaveSpecialInteractor("SelectBox")) return 7;
    const auto centerBefore = sphere->GetStartPoint();
    const auto sphereCameraBefore = (interactor->GetCamera()->GetViewMatrix() * scene->GetModelMatrix());
    Drag(interactor, Project(interactor, centerBefore), {30.f, 15.f});
    app.processEvents();
    if ((sphere->GetStartPoint() - centerBefore).length() < 1.e-6 ||
        MatrixDistance(sphereCameraBefore, (interactor->GetCamera()->GetViewMatrix() * scene->GetModelMatrix())) > 1.e-5 || !unchangedSelection()) {
        std::cerr << "Sphere center delta=" << (sphere->GetStartPoint() - centerBefore).length()
                  << " camera delta=" << MatrixDistance(sphereCameraBefore, (interactor->GetCamera()->GetViewMatrix() * scene->GetModelMatrix()))
                  << " switch delta=" << MatrixDistance(cameraBefore, sphereCameraBefore)
                  << " selection unchanged=" << unchangedSelection() << '\n';
        if (argc > 1) window.grab().save(QString::fromLocal8Bit(argv[1]) + "-sphere-failed.png");
        return 8;
    }
    const auto r = (sphere->GetEndPoint() - sphere->GetStartPoint()).length();
    Drag(interactor, Project(interactor, sphere->GetRadiusHandle()), {30.f, 0.f});
    if (std::abs((sphere->GetEndPoint() - sphere->GetStartPoint()).length() - r) < 1.e-6) return 9;
    const auto sphereCenter = sphere->GetStartPoint();
    Drag(interactor, Project(interactor, sphereCenter), {20.f, 10.f}, iGame::MiddleButton);
    if ((sphere->GetStartPoint() - sphereCenter).length() > 1.e-6) return 10;
    if (argc > 1) window.grab().save(QString::fromLocal8Bit(argv[1]) + "-sphere.png");
    shape->setCurrentIndex(2);
    app.processEvents();
    auto plane = iGame::DynamicCast<iGame::RegionPlaneStyle>(interactor->GetSpecialInteractor("ExtractCellsByRegionPlane"));
    if (!plane || sphere->GetShow()) return 11;
    auto origin = plane->GetOrigin();
    Drag(interactor, Project(interactor, origin), {25.f, 20.f});
    if ((plane->GetOrigin() - origin).length() < 1.e-6) return 12;
    if (argc > 1) { window.grab().save(QString::fromLocal8Bit(argv[1]) + "-plane.png"); dialog->grab().save(QString::fromLocal8Bit(argv[1]) + "-panel.png"); }
    // Misses must reach the regular model rotation, even after a widget drag.
    const auto outsideCameraBefore = (interactor->GetCamera()->GetViewMatrix() * scene->GetModelMatrix());
    Drag(interactor, {15.f, 15.f}, {35.f, 25.f});
    if (MatrixDistance(outsideCameraBefore, (interactor->GetCamera()->GetViewMatrix() * scene->GetModelMatrix())) < 1.e-6) return 14;
    if (!unchangedSelection()) return 15;
    // Apply uses a half-space, preserves whole cells and updates the same result.
    for (int i = 1; i <= 3; ++i) edit(i, 0.);
    for (int i = 8; i <= 10; ++i) edit(i, i == 8 ? 1. : 0.);
    applyEdits(10);
    dialog->apply();
    app.processEvents();
    auto resultModel = scene->GetCurrentModel();
    auto output = iGame::DynamicCast<iGame::UnstructuredMesh>(resultModel->GetDataObject());
    if (!output || output->GetNumberOfCells() != 2 || !unchangedSelection()) return 16;
    shape->setCurrentIndex(1);
    edit(7, 100.);
    dialog->apply();
    app.processEvents();
    output = iGame::DynamicCast<iGame::UnstructuredMesh>(scene->GetCurrentModel()->GetDataObject());
    if (scene->GetCurrentModel() != resultModel || !output || output->GetNumberOfCells() != 4) return 17;
    shape->setCurrentIndex(0);
    for (int i = 4; i <= 6; ++i) edit(i, 10.);
    dialog->apply();
    app.processEvents();
    output = iGame::DynamicCast<iGame::UnstructuredMesh>(scene->GetCurrentModel()->GetDataObject());
    if (scene->GetCurrentModel() != resultModel || !output || output->GetNumberOfCells() != 4) return 18;
    for (int i = 1; i <= 3; ++i) edit(i, 100.);
    dialog->apply();
    app.processEvents();
    output = iGame::DynamicCast<iGame::UnstructuredMesh>(scene->GetCurrentModel()->GetDataObject());
    if (scene->GetCurrentModel() != resultModel || !output || output->GetNumberOfCells() != 4) return 22;
    dialog->close();
    app.processEvents();
    std::cout << "PASS region widgets: left drag, camera fallback, no live selection, three Apply modes\n";
    return 0;
}
