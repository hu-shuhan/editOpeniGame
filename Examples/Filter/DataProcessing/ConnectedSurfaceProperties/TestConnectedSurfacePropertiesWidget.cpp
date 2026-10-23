// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
#include <IQCore/igQtMainWindow.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQWidgets/igQtConnectedSurfacePropertiesWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQWidgets/igQtSearchInfoWidget.h>

#include <iGameDrawObject.h>
#include <iGameSceneManager.h>

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDockWidget>
#include <QEvent>
#include <QEventLoop>
#include <QPushButton>
#include <QRadioButton>
#include <QSurfaceFormat>
#include <QTableWidget>
#include <QTimer>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckHeap(const char* stage) {
#if defined(_MSC_VER) && defined(_DEBUG)
    if (_CrtCheckMemory() == 0) {
        throw std::runtime_error(std::string("heap corruption detected: ") + stage);
    }
#else
    (void)stage;
#endif
    std::cout << "heap check passed: " << stage << std::endl;
}

QPushButton* FindExecuteButton(QWidget* panel) {
    for (auto* button : panel->findChildren<QPushButton*>()) {
        if (button->text() == QStringLiteral("执行并生成输出")) return button;
    }
    return nullptr;
}

void ProcessEventsFor(int milliseconds) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

bool HasColumn(QTableWidget* table, const QString& name) {
    if (!table) return false;
    for (int column = 0; column < table->columnCount(); ++column) {
        auto* item = table->horizontalHeaderItem(column);
        if (item && item->text() == name) return true;
    }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    std::cout << "starting Qt panel heap test" << std::endl;
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
#if defined(Q_OS_WIN)
        qputenv("QT_QPA_PLATFORM", "windows");
#else
        qputenv("QT_QPA_PLATFORM", "offscreen");
#endif
    }
    QApplication app(argc, argv);
    std::cout << "QApplication created" << std::endl;

    try {
        iGame::Log::Init();
        std::cout << "log initialized" << std::endl;
        igQtMainWindow owner;
        owner.show();
        app.processEvents();
        CheckHeap("main window constructed");
        owner.initArgs({QStringLiteral("testConnectedSurfacePropertiesWidget"),
                        QStringLiteral("--filepath"),
                        QStringLiteral("./Models/ConnectedSurfacePropertiesTwoBoxes.vtk")});
        ProcessEventsFor(1500);
        CheckHeap("input model loaded");
        auto* scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto sourceModel = scene ? scene->GetCurrentModel() : nullptr;
        Check(sourceModel != nullptr, "missing source model");
        auto sourceData = sourceModel->GetDataObject();

        QAction* filterAction = nullptr;
        for (auto* action : owner.findChildren<QAction*>()) {
            if (action->data().toString() == QStringLiteral("compute_connected_surface_properties")) {
                filterAction = action;
                break;
            }
        }
        Check(filterAction != nullptr, "missing Connected Surface Properties action");
        filterAction->trigger();
        ProcessEventsFor(250);
        auto* panel = dynamic_cast<igQtConnectedSurfacePropertiesWidget*>(
                owner.findChild<QDialog*>(QStringLiteral("ConnectedSurfacePropertiesWidget")));
        Check(panel != nullptr && panel->isReady() && panel->isVisible(),
              "filter panel did not open with the selected SurfaceMesh");
        Check(panel->isWindow() && panel->size() == QSize(600, 300),
              "filter panel must remain a compact standalone window");
        Check(owner.findChild<QDockWidget*>(
                      QStringLiteral("standardFilterPanel_compute_connected_surface_properties")) == nullptr,
              "filter panel must not be stretched inside a dock widget");
        CheckHeap("filter panel opened");
        auto* execute = FindExecuteButton(panel);
        Check(execute != nullptr, "missing execute button");

        CheckHeap("before first execution");
        execute->click();
        app.processEvents();

        auto* searchDock = owner.findChild<QDockWidget*>(QStringLiteral("dockWidget_SearchInfo"));
        auto* searchWidget = owner.findChild<igQtSearchInfoWidget*>();
        auto* objectData = searchWidget
                ? searchWidget->findChild<QRadioButton*>(QStringLiteral("radioButton_Objects"))
                : nullptr;
        auto* cellData = searchWidget
                ? searchWidget->findChild<QRadioButton*>(QStringLiteral("radioButton_Cells"))
                : nullptr;
        auto* resultTable = searchWidget
                ? searchWidget->findChild<QTableWidget*>(QStringLiteral("tableWidget_Results"))
                : nullptr;
        Check(searchDock && searchDock->isVisible(), "SearchInfo dock did not open after execution");
        Check(objectData && objectData->isChecked(), "SearchInfo did not select object data");
        Check(resultTable && resultTable->rowCount() == 2, "object data table must contain two objects");
        Check(HasColumn(resultTable, QStringLiteral("ObjectValidity")) &&
                      HasColumn(resultTable, QStringLiteral("ObjectAreas")) &&
                      HasColumn(resultTable, QStringLiteral("ObjectVolumes")) &&
                      HasColumn(resultTable, QStringLiteral("ObjectCentroids[0]")) &&
                      HasColumn(resultTable, QStringLiteral("ObjectCentroids[1]")) &&
                      HasColumn(resultTable, QStringLiteral("ObjectCentroids[2]")),
              "object data table is missing connected-surface properties");

        cellData->click();
        app.processEvents();
        Check(HasColumn(resultTable, QStringLiteral("FaceValue")),
              "output lost an original cell attribute");
        Check(HasColumn(resultTable, QStringLiteral("ObjectIds")) &&
                      HasColumn(resultTable, QStringLiteral("Areas")) &&
                      HasColumn(resultTable, QStringLiteral("Volumes")),
              "cell data table is missing computed properties");
        auto* modelTree = owner.findChild<igQtModelDialogWidget*>();
        auto outputModel = scene->GetCurrentModel();
        auto outputData = outputModel ? outputModel->GetDataObject() : nullptr;
        Check(modelTree && outputData && outputData != sourceData,
              "filter output was not added as an independent model");
        Check(!sourceModel->GetVisibility(),
              "source model should be hidden while its output is displayed");
        auto* outputItem = modelTree->getItemFromObject(outputData);
        Check(outputItem != nullptr, "missing output item in model tree");
        modelTree->setCurrentItem(outputItem);
        modelTree->deleteCurrentModel();
        app.processEvents();
        auto sourceDraw = iGame::DynamicCast<iGame::DrawObject>(sourceData);
        Check(modelTree->getItemFromObject(outputData) == nullptr,
              "output item was not deleted from the model tree");
        Check(sourceModel->GetVisibility() && sourceDraw && sourceDraw->GetVisibility(),
              "deleting the output must restore the source model visibility");
        CheckHeap("output deleted and source restored");
        CheckHeap("first execution completed");
        panel->close();
        app.processEvents();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        Check(!panel->isVisible(),
              "closing must hide the reusable panel without destroying it");
        CheckHeap("filter panel closed");

        filterAction->trigger();
        ProcessEventsFor(250);
        auto* reopenedPanel = dynamic_cast<igQtConnectedSurfacePropertiesWidget*>(
                owner.findChild<QDialog*>(QStringLiteral("ConnectedSurfacePropertiesWidget")));
        Check(panel == reopenedPanel && panel->isVisible(),
              "the same panel instance must reopen for the current model");
        execute->click();
        app.processEvents();
        CheckHeap("reopened panel executed");
        panel->close();
        app.processEvents();
        CheckHeap("filter panel closed a second time");

        owner.close();
        app.processEvents();
        CheckHeap("main window closed");
        std::cout << "Connected-surface-properties Qt panel heap test passed.\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Connected-surface-properties Qt panel test failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
