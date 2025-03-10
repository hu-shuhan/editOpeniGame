#include "IQCore/igQtMainWindow.h"
//
// Created by m_ky on 2024/4/10.
//
#include "Compression/iGameDecoder.h"
#include "Compression/iGameEncoder.h"
#include "Interactor/iGameInteractor.h"
#include "SurfaceMeshFilters/iGameGradient.h"
#include "SurfaceMeshFilters/iGameSimplification.h"
#include "SurfaceMeshFilters/iGameSimplification2.h"
#include "SurfaceMeshFilters/iGameSurfaceSimplification.h"
#include "SurfaceMeshFilters/iGameTriangulation.h"
#include "UndefinedFilters/iGameCurvatureFilter.h"
#include "UndefinedFilters/iGameGradientFilter.h"
#include "UndefinedFilters/iGameLaplacianFilter.h"
#include "UndefinedFilters/iGameVortexFilter.h"
#include "iGameARAPTest.h"
#include "iGameFileIO.h"
#include "iGameFilterIncludes.h"
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQComponents/igQtProgressBarWidget.h>
#include <IQCore/igQtFileLoader.h>
#include <IQCore/igQtOpenGLWidgetManager.h>
#include <IQWidgets/ColorManager/igQtColorManagerWidget.h>
#include <IQWidgets/igQtCharts.h>
#include <IQWidgets/igQtDeformationWidget.h>
#include <IQWidgets/igQtModelClipWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQWidgets/igQtModelInformationWidget.h>
#include <IQWidgets/igQtTensorWidget.h>
#include <Sources/iGameLineTypePointsSource.h>
#include <VolumeMeshAlgorithm/iGameVolumeMeshClipper.h>
#include <fcntl.h> // 用于 open
#include <iGameDataSource.h>
#include <iGamePointFinder.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMeshFilterTest.h>
#include <include/IQComponents/Dialog/igQtChangeBackGroundDialog.h>
#include <include/IQComponents/Dialog/igQtScreenShotOptionDialog.h>
#include <meshoptimizer.h>
#include <stdio.h>

#include <QDebug>
#include <QMessageBox>

igQtMainWindow::igQtMainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    initAllUnDefinedComponents();
    initToolbarComponent();
    initAllComponents();
    initAllFilters();
    initAllSources();
    initAllInteractor();
    updateRecentFilePaths();
    connect(modelTreeWidget, &igQtModelDialogWidget::Update, rendererWidget, &igQtRenderWidget::update);
}
void igQtMainWindow::initArgs(const QStringList& args) {
    int argc = args.size();
    for (int i = 1; i < argc; ++i) {
        const QString& cur_arg = args[i].toLower();
        if (cur_arg == "--filepath" && ++i < argc) {
            const QString& filePath = args[i];
            fileLoader->OpenFile(filePath.toStdString());
        }
    }
}
void igQtMainWindow::initAllUnDefinedComponents() {
    rendererWidget = new igQtModelDrawWidget(this);
    igQtOpenGLManager::Instance()->setQtRenderWidget(rendererWidget);
    //    rendererWidget->setParent(this);
    fileLoader = new igQtFileLoader(this);
    this->setCentralWidget(rendererWidget);
    this->ColorManagerWidget = new igQtColorManagerWidget;
    ColorManagerWidget->setGeometry(400, 500, 780, 1000);
    ui->dockWidget_ScalarField->hide();
    ui->dockWidget_VectorField->hide();
    ui->dockWidget_FlowField->hide();
    ui->dockWidget_TensorField->hide();
    ui->dockWidget_SearchInfo->hide();
    ui->dockWidget_QualityDetection->hide();
    ui->dockWidget_EditMode->hide();
    ui->dockWidget_Animation->hide();
    ui->dockWidget_ModelList->hide();
    ui->dockWidget_ContourExtract->hide();
    // Setup default GUI layout.
    this->setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);
    this->setTabPosition(Qt::RightDockWidgetArea, QTabWidget::North);
    this->setTabPosition(Qt::BottomDockWidgetArea, QTabWidget::North);
    // Set up the dock window corners to give the vertical docks more room.
    this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    modelTreeWidget = new igQtModelDialogWidget(this);
    modelTreeWidget->setFloating(false); // Make sure it's docked
    modelTreeWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::TopDockWidgetArea);
    modelTreeWidget->setFeatures(QDockWidget::NoDockWidgetFeatures); // Disable floating and moving
    this->addDockWidget(Qt::LeftDockWidgetArea, modelTreeWidget);


    SliceDockWidget = new QDockWidget(this);
    SliceDockWidget->setWindowTitle("网格切割");
    SliceWidget = new igQtModelClipWidget(SliceDockWidget);
    SliceDockWidget->setWidget(SliceWidget);
    SliceDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea);
    SliceDockWidget->hide();
    this->addDockWidget(Qt::LeftDockWidgetArea, SliceDockWidget);

    DeformationDockWidget = new QDockWidget(this);
    DeformationDockWidget->setWindowTitle("结构形变");
    DeformationWidget = new igQtDeformationWidget(DeformationDockWidget);
    DeformationDockWidget->setWidget(DeformationWidget);
    DeformationDockWidget->setAllowedAreas(Qt::RightDockWidgetArea);
    DeformationDockWidget->hide();
    this->addDockWidget(Qt::RightDockWidgetArea, DeformationDockWidget);
}
void igQtMainWindow::initToolbarComponent() {

    // viewStyleCombox = new QComboBox(this);
    // viewStyleCombox->addItem("Points");
    // viewStyleCombox->addItem("WireFrame");
    // viewStyleCombox->addItem("Surface");
    // viewStyleCombox->addItem("Surface With Edegs");
    // viewStyleCombox->addItem("Volume");
    // viewStyleCombox->addItem("Volume With Edegs");

    // viewStyleCombox->setStyleSheet("QComboBox {"
    //	"background-color: #f0f0f0;"
    //	"color: #202020;"              // 设置文本颜色为浅白色
    //	"border: 1px solid #ffffff;"   // 设置边框样式为灰色实线边框
    //	"padding: 5px;"                // 设置内边距
    //	"font-size: 16px;"              // 设置下拉菜单项字体大小为14px
    //	"}"
    //	"QComboBox QAbstractItemView {"
    //	"font-family: Arial;"           // 设置下拉菜单项字体为Arial
    //	"color: #404040;"               // 设置下拉菜单项字体颜色为浅灰色
    //	"}"
    //	"QComboBox::drop-down {"
    //	"subcontrol-origin: padding;"
    //	"subcontrol-position: top right;"
    //	"width: 20px;"
    //	"border-left: 1px solid #202020;"
    //	"border-color: #eeeeee;"
    //	"}"
    //);

    // connect(viewStyleCombox, SIGNAL(currentIndexChanged(QString)), this,
    // SLOT(ChangeViewStyle())); ui->toolBar_meshview->addWidget(viewStyleCombox);

    // attributeViewIndexCombox = new QComboBox(this);
    // attributeViewIndexCombox->addItem("None        ");
    // attributeViewIndexCombox->setStyleSheet("QComboBox {"
    //	"background-color: #f0f0f0;"
    //	"color: #202020;"              // 设置文本颜色为浅白色
    //	"border: 1px solid #ffffff;"   // 设置边框样式为灰色实线边框
    //	"padding: 5px;"                // 设置内边距
    //	"font-size: 16px;"              // 设置下拉菜单项字体大小为14px
    //	"}"
    //	"QComboBox QAbstractItemView {"
    //	"font-family: Arial;"           // 设置下拉菜单项字体为Arial
    //	"color: #404040;"               // 设置下拉菜单项字体颜色为浅灰色
    //	"}"
    //	"QComboBox::drop-down {"
    //	"subcontrol-origin: padding;"
    //	"subcontrol-position: top right;"
    //	"width: 20px;"
    //	"border-left: 1px solid #202020;"
    //	"border-color: #eeeeee;"
    //	"}"
    //);

    // connect(attributeViewIndexCombox, SIGNAL(activated(int)), this,
    // SLOT(ChangeScalarView()));
    // ui->toolBar_attribute_view_index->addWidget(attributeViewIndexCombox);

    // attributeViewDimCombox = new QComboBox(this);
    // attributeViewDimCombox->addItem("magnitude");
    // attributeViewDimCombox->setStyleSheet("QComboBox {"
    //	"background-color: #f0f0f0;"
    //	"color: #202020;"              // 设置文本颜色为浅白色
    //	"border: 1px solid #ffffff;"   // 设置边框样式为灰色实线边框
    //	"padding: 5px;"                // 设置内边距
    //	"font-size: 16px;"              // 设置下拉菜单项字体大小为14px
    //	"}"
    //	"QComboBox QAbstractItemView {"
    //	"font-family: Arial;"           // 设置下拉菜单项字体为Arial
    //	"color: #404040;"               // 设置下拉菜单项字体颜色为浅灰色
    //	"}"
    //	"QComboBox::drop-down {"
    //	"subcontrol-origin: padding;"
    //	"subcontrol-position: top right;"
    //	"width: 10px;"
    //	"border-left: 1px solid #202020;"
    //	"border-color: #eeeeee;"
    //	"}"
    //);

    // connect(attributeViewDimCombox, SIGNAL(activated(int)), this,
    // SLOT(ChangeScalarViewDim()));
    // ui->toolBar_attribute_view_dim->addWidget(attributeViewDimCombox);
}

void igQtMainWindow::initAllComponents() {
    connect(ui->action_ChangeBackground, &QAction::triggered, this, [&]() {
        igQtChangeBackGroundDialog dialog(this);
        dialog.setWindowTitle("Change BackGround Color.");
        int R = 0, G = 0, B = 0;
        if (dialog.exec() == QDialog::Accepted) {
            auto input = dialog.getInput();
            R = input[0], G = input[1], B = input[2];
        }
        iGame::SceneManager::Instance()->GetCurrentScene()->SetBackGround(R, G, B);
    });
    connect(ui->action_VolumeRendering, &QAction::triggered, this,
            [&](bool toggled) { iGame::SceneManager::Instance()->GetCurrentScene()->SetVolumeRendering(toggled); });
    // init ProgressBar
    progressBarWidget = new igQtProgressBarWidget(this);
    this->statusBar()->addPermanentWidget(progressBarWidget);

    connect(ui->action_compress, &QAction::triggered, this, [&](bool checked) {
        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
        dialog->setFilterTitle("压缩");
        dialog->setFilterDescription("自定义量化位数: 截断浮点数位数(1~23位); FP16: 半精度浮点数; 无量化: "
                                     "保留原始格式; 1ULP误差: 浮点数绝对误差/(2^-23)");
        std::vector<QString> defaultValue1;
        defaultValue1.push_back("自定义量化位数");
        defaultValue1.push_back("FP16");
        defaultValue1.push_back("无量化");
        std::vector<QString> defaultValue2;
        defaultValue2.push_back("自定义量化位数");
        defaultValue2.push_back("FP16");
        defaultValue2.push_back("无量化");
        int id1 = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, "顶点坐标量化模式", defaultValue1);

        int id2 = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "顶点坐标量化位数", "16");

        int id3 = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, "属性量化模式", defaultValue2);

        int id4 = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "属性量化位数", "16");

        // 量化位数控制
        QRegExp regExp("^(1|[2-9]|1[0-9]|2[0-3])$");
        QLineEdit* widget2 = qobject_cast<QLineEdit*>(dialog->getWidget(id2));
        QLineEdit* widget4 = qobject_cast<QLineEdit*>(dialog->getWidget(id4));
        QRegExpValidator* validator = new QRegExpValidator(regExp, this);
        widget2->setValidator(validator);
        widget4->setValidator(validator);

        std::vector<QString> defaultValue3;
        defaultValue3.push_back("不计算");
        defaultValue3.push_back("MAPE");
        //defaultValue3.push_back("平均1ULP误差");
        defaultValue3.push_back("全部");

        int id5 = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, "误差统计", defaultValue3);

        std::vector<QString> defaultValue4;
        defaultValue4.push_back("不计算");
        defaultValue4.push_back("BPV");
        defaultValue4.push_back("压缩比");
        defaultValue4.push_back("全部");

        int id6 = dialog->addParameter(igQtFilterDialogDockWidget::QT_COMBO_BOX, "压缩比统计", defaultValue4);

        // 量化位数参数仅在自定义量化位数时生效
        QWidget* widget1 = dialog->getWidget(id1);
        QWidget* widget3 = dialog->getWidget(id3);

        if (widget1) {
            connect(qobject_cast<QComboBox*>(widget1), QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [=](int index) { dialog->getWidget(id2)->setEnabled(index == 0); });
        }

        if (widget3) {
            connect(qobject_cast<QComboBox*>(widget3), QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [=](int index) { dialog->getWidget(id4)->setEnabled(index == 0); });
        }

        dialog->show();
        dialog->setApplyFunctor([=]() {
            std::string destFilePath =
                    QFileDialog::getSaveFileName(nullptr, "Compress file as ", "", "Compress Mesh(*.igc)")
                            .toStdString();
            if (destFilePath.empty()) {
                igDebug("Could not save file with error file path\n");
                return;
            }

            bool ok;
            std::vector<std::string> errorStatus;
            std::vector<std::string> compactnessStatus;

            ok = fileLoader->Compress(dialog->getComboIndex(id1, ok), dialog->getInt(id2, ok),
                                      dialog->getComboIndex(id3, ok), dialog->getInt(id4, ok),
                                      dialog->getComboIndex(id5, ok), dialog->getComboIndex(id6, ok),
                                      dialog->getComboIndex(id5, ok) != 0 ? &errorStatus : nullptr,
                                      dialog->getComboIndex(id6, ok) != 0 ? &compactnessStatus : nullptr, destFilePath);

            if (ok) {
                QString result = "压缩成功\n\n";
                if (dialog->getComboIndex(id5, ok) != 0) {
                    result += "误差统计\n";
                    for (const std::string& l: errorStatus) {
                        result += QString::fromStdString(l);
                        result += "\n";
                    }
                }
                if (dialog->getComboIndex(id6, ok) != 0) {
                    result += "\n压缩率统计\n";
                    for (const std::string& l: compactnessStatus) {
                        result += QString::fromStdString(l);
                        result += "\n";
                    }
                }

                QMessageBox::information(this, "压缩成功", result);
                dialog->close();
            } else
                QMessageBox::information(this, "压缩失败", "请检查是否正确载入数据");
        });
    });

    // connect(ui->action_SaveScreenShot, &QAction::triggered, rendererWidget,
    // &igQtModelDrawWidget::SaveScreenShoot);
    connect(ui->action_LoadFile, &QAction::triggered, fileLoader, &igQtFileLoader::LoadFile);
    connect(ui->action_CS, &QAction::triggered, fileLoader, &igQtFileLoader::LoadOnlineS);
    connect(ui->action_C, &QAction::triggered, fileLoader, &igQtFileLoader::LoadOnlineC);
    // connect(ui->action_SaveMesh, &QAction::triggered, fileLoader,
    // &igQtFileLoader::SaveFile);
    connect(ui->action_SaveMeshAs, &QAction::triggered, fileLoader, &igQtFileLoader::SaveFileAs);
    // connect(ui->action_CopyMesh, &QAction::triggered, this, [&]() {
    //	iGame::iGameManager::Instance()->CopyMesh();
    //	});
    // connect(ui->action_RecoverMesh, &QAction::triggered, this, [&]() {
    //	iGame::iGameManager::Instance()->RecoverMesh();
    //	});
    connect(ui->action_UseOrthographic, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_UseOrthographic->isChecked()) {
            SceneManager::Instance()->GetCurrentScene()->ChangeCameraType(Camera::Type::ORTHOGRAPHIC);
        } else {
            SceneManager::Instance()->GetCurrentScene()->ChangeCameraType(Camera::Type::PERSPECTIVE);
        }
        rendererWidget->update();
    });
    connect(ui->action_ResetCameraView, &QAction::triggered, this, [&]() {
        SceneManager::Instance()->GetCurrentScene()->ResetCameraView();
        rendererWidget->update();
    });
    // connect(ui->action_PickCenter, &QAction::triggered, this, [&]() {
    //	float x = -1.0, y = -1.0, z = -1.0;
    //	iGame::iGameManager::Instance()->UpdateCenter(x, y, z);
    //	rendererWidget->update();
    //	});
    //    connect(ui->action_DeleteMesh, &QAction::triggered, ui->modelTreeView,
    //            &igQtModelListView::DeleteCurrentFile);
    // connect(ui->action_DeleteMesh, &QAction::triggered, this,
    // &igQtMainWindow::updateCurrentSceneWidget); connect(ui->action_NextMesh,
    // &QAction::triggered, ui->modelTreeView,
    // &igQtModelListView::ChangeSelected2NextItem); connect(ui->action_NextMesh,
    // &QAction::triggered, rendererWidget,
    // &igQtModelDrawWidget::changeCurrentModel2Next);
    // connect(ui->action_LastMesh, &QAction::triggered, ui->modelTreeView,
    // &igQtModelListView::ChangeSelected2LastItem); connect(ui->action_LastMesh,
    // &QAction::triggered, rendererWidget,
    // &igQtModelDrawWidget::changeCurrentModel2Last);

    connect(ui->action_setViewToPositiveX, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToPositiveX();
        rendererWidget->update();
    });
    connect(ui->action_setViewToNegativeX, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToNegativeX();
        rendererWidget->update();
    });
    connect(ui->action_setViewToPositiveY, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToPositiveY();
        rendererWidget->update();
    });
    connect(ui->action_setViewToNegativeY, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToNegativeY();
        rendererWidget->update();
    });
    connect(ui->action_setViewToPositiveZ, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToPositiveZ();
        rendererWidget->update();
    });
    connect(ui->action_setViewToNegativeZ, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToNegativeZ();
        rendererWidget->update();
    });
    connect(ui->action_setViewToIsometric, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->ResetCameraViewToIsometric();
        rendererWidget->update();
    });
    connect(ui->action_rotateNinetyClockwise, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->RotateNinetyClockwise();
        rendererWidget->update();
    });
    connect(ui->action_rotateNinetyCounterClockwise, &QAction::triggered, this, [&](bool checked) {
        rendererWidget->GetScene()->RotateNinetyCounterClockwise();
        rendererWidget->update();
    });
    connect(ui->action_SaveScreenShot, &QAction::triggered, this, [&]() {
        QString path =
                QFileDialog::getSaveFileName(nullptr, "Save Screen shot", "", "PNG Images(*.png);;BMP Images(*.bmp)");
        igQtScreenShotOptionDialog dialog(this);
        dialog.setWindowTitle("Save ScreenShot Option.");
        int oldwidth = rendererWidget->width(), oldheight = rendererWidget->height();
        int ratio_pixel = rendererWidget->devicePixelRatio();
        int width = 1920, height = 1080;
        if (dialog.exec() == QDialog::Accepted) {
            auto input = dialog.getInput();
            width = input.first, height = input.second;
        }

        width /= ratio_pixel, height /= ratio_pixel;
        rendererWidget->resize(width, height);
        QImage saved_image = rendererWidget->grabFramebuffer();
        rendererWidget->resize(oldwidth, oldheight);
        if (saved_image.save(path, "BMP")) {
            QMessageBox::information(this, "", "保存成功");
        } else {
            QMessageBox::information(this, "", "保存失败");
        }
    });

    connect(ui->action_SaveAnimation, &QAction::triggered, this, [&]() { ui->widget_Animation->saveAnimation(); });

    connect(ui->action_SetThreadNum, &QAction::triggered, this, [&](bool checked) {
        // 创建对话框
        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
        dialog->previousInFocusChain()->hide();
        dialog->setFilterTitle("设置并行线程数");
        // 获取当前线程池的默认线程数
        int currentThreadCount = iGame::ThreadPool::GetDefaultThreadCount();
        int maxThreads = std::thread::hardware_concurrency();
        QString recommendedThreads = QString::number(maxThreads / 2);
        dialog->setFilterDescription(QString("当前并行线程数: %1<br>"
                                             "硬件支持的最大线程数: %2<br>"
                                             "推荐线程数: %3<br>"
                                             "注意: 设置并行线程数会影响程序的性能。<br>"
                                             "建议根据硬件配置合理设置线程数。")
                                             .arg(currentThreadCount)
                                             .arg(maxThreads)
                                             .arg(recommendedThreads));

        // 添加参数：线程数输入框
        int id1 = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "并行线程数",
                                       QString::number(currentThreadCount));
        // 显示对话框
        dialog->show();
        // 设置应用按钮的回调函数
        dialog->setApplyFunctor([=]() {
            bool ok;
            // 获取用户输入的线程数
            int newThreadCount = dialog->getInt(id1, ok);
            // 检查输入是否有效
            if (ok && newThreadCount > 0) {
                // 检查线程数是否超过硬件支持的最大值
                /*if (newThreadCount > maxThreads) {
					QMessageBox::warning(this, "错误", QString("线程数不能超过硬件支持的最大值: %1").arg(maxThreads));
					return;
				}*/
                // 设置新的线程数
                iGame::ThreadPool::SetDefaultThreadCount(newThreadCount);
                QMessageBox::information(this, "成功", QString("并行线程数已设置为: %1").arg(newThreadCount));
                dialog->close();
            } else {
                QMessageBox::warning(this, "错误", "请输入有效的线程数（大于0的整数）。");
            }
        });
    });

    initAllDockWidgetConnectWithAction();
    initAllMySignalConnections();
}
igQtMainWindow::~igQtMainWindow() {}


#include "SurfaceMeshFilters/meshsimplifier/meshsimplifier.h"
#include "SurfaceMeshFilters/simplifier.h"

void igQtMainWindow::initAllFilters() {
    QMenu* mesh_processing = ui->menu_filters->addMenu("Remeshing Simplification");
    connect(mesh_processing->addAction("Simplification"), &QAction::triggered, this, [&](bool checked) {
        // VolumeMeshFilterTest::Pointer fp = VolumeMeshFilterTest::New();
        // fp->SetInput(rendererWidget->GetScene()->GetCurrentModel()->GetDataObject());
        // fp->Execute();
        // rendererWidget->update();
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
        int reductionId = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "Reduction (0..1)", "0.5");
        int preserveId =
                dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "Preserve Boundary of the mesh", "true");
        int scalarId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "Check All Scalars of the mesh ",
                                            "true");
        int checkId = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "Geometric similarity measure ",
                                           "false");
        dialog->show();
        dialog->setApplyFunctor([=]() {
            Triangulation::Pointer triangulation = Triangulation::New();
            auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
            triangulation->SetInput(obj);
            triangulation->Execute();
            obj = triangulation->GetOutput();

            Simplification::Pointer filter = Simplification::New();

            bool ok;
            filter->SetTargetReduction(dialog->getDouble(reductionId, ok));
            filter->SetPreserveBoundary(dialog->getChecked(preserveId, ok));
            filter->SetAllScalarCheck(dialog->getChecked(scalarId, ok));
            filter->SetInput(obj);
            //filter->SetActivedAttribIndices({0});
            ok = filter->Execute();

            if (ok) {

                auto oldMesh = DynamicCast<SurfaceMesh>(obj);
                auto outObj = filter->GetOutput();
                auto newMesh = DynamicCast<SurfaceMesh>(outObj);
                auto oldPoints = oldMesh->GetPoints();
                auto newPoints = newMesh->GetPoints();
                //PointFinder::Pointer oldPicker = PointFinder::New();
                //oldPicker->SetPoints(oldPoints);
                //oldPicker->Initialize();
                QString result = "";
                if (dialog->getChecked(checkId, ok)) {
                    PointFinder::Pointer newPicker = PointFinder::New();
                    newPicker->SetPoints(newPoints);
                    newPicker->Initialize();

                    double w1 = 0.0, w2 = 0.0;
                    // 计算原始网格的表面积
                    for (int i = 0; i < oldMesh->GetNumberOfFaces(); i++) {
                        igIndex f[3]{};
                        oldMesh->GetFacePointIds(i, f);
                        Point v0 = oldMesh->GetPoint(f[0]);
                        Point v1 = oldMesh->GetPoint(f[1]);
                        Point v2 = oldMesh->GetPoint(f[2]);

                        Vector3f d10 = v1 - v0;
                        Vector3f d20 = v2 - v0;

                        w1 += CrossProduct(d10, d20).norm() / 2.0;
                    }
                    //for (int i = 0; i < newMesh->GetNumberOfFaces(); i++) {
                    //    igIndex f[3]{};
                    //    newMesh->GetFacePointIds(i, f);
                    //    Point v0 = newMesh->GetPoint(f[0]);
                    //    Point v1 = newMesh->GetPoint(f[1]);
                    //    Point v2 = newMesh->GetPoint(f[2]);

                    //    Vector3f d10 = v1 - v0;
                    //    Vector3f d20 = v2 - v0;

                    //    w2 += CrossProduct(d10, d20).norm() / 2.0;
                    //}

                    double d1 = 0.0, d2 = 0.0;
                    double d3 = 0.0, d4 = 0.0;

                    iGame::ProgressObserver* ProgressBar = iGame::ProgressObserver::Instance();
                    ProgressBar->UpdateProgress(0);
                    int blockNum = oldPoints->GetNumberOfPoints() / 100, progress = 0;
                    // 计算平均平方距离
                    for (int i = 0; i < oldPoints->GetNumberOfPoints(); i++) {
                        if (i > progress * blockNum) {
                            ProgressBar->UpdateProgress(progress * 0.01);
                            progress++;
                        }
                        auto p = oldPoints->GetPoint(i);

                        igIndex id = newPicker->FindClosestPoint(p);
                        if (id != -1) {
                            Point cp = newPoints->GetPoint(id);
                            d1 += (p - cp).squaredNorm();
                            d3 += (p - cp).norm();
                        }
                    }

                    //for (int i = 0; i < newPoints->GetNumberOfPoints(); i++) {
                    //    auto p = newPoints->GetPoint(i);

                    //    igIndex id = oldPicker->FindClosestPoint(p);
                    //    if (id != -1) {
                    //        Point cp = oldPoints->GetPoint(id);
                    //        d2 += (p - cp).squaredNorm();
                    //        d4 += (p - cp).norm();
                    //    }
                    //}

                    double d = 1.0 / w1 * d1 /*+ 1.0 / w2 * d2*/;
                    double dd =
                            1.0 / oldPoints->GetNumberOfPoints() * d3 /*+ 1.0 / newPoints->GetNumberOfPoints() * d4*/;

                    result += "\n几何相似性度量";
                    result += "\n Squared Mean Distance: " + QString::number(d);
                    result += "\n Mean Distance: " + QString::number(dd);
                    result += "\nSquared Mean Distance: " + QString::number(d * 100) + "%";
                    result += "\nMean Distance: " + QString::number(dd);
                    result += "\n\n累计几何误差: " + QString::number(filter->GetError());
                } else {
                    result += "\n累计几何误差: " + QString::number(filter->GetError());
                }

                modelTreeWidget->addDataObjectToModelTree(outObj, Algorithm);
                rendererWidget->update();

                QMessageBox::information(this, "简化成功", result);
            }

            dialog->close();
        });
    });

    /*
    connect(mesh_processing->addAction("Simplification"), &QAction::triggered, this, [&](bool checked) {
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        int index = DynamicCast<DrawObject>(obj)->GetAttributeIndex();
        if (index == -1) index = 0;
        Triangulation::Pointer triangulation = Triangulation::New();
        triangulation->SetInput(obj);
        triangulation->Execute();
        obj = triangulation->GetOutput();
        auto mesh = DynamicCast<SurfaceMesh>(obj);

        unsigned int* destination;
        unsigned int* indices = reinterpret_cast<unsigned int*>(mesh->GetFaces()->GetCellIdArray()->RawPointer());
        const size_t index_count = mesh->GetNumberOfFaces() * 3;
        float* vertex_positions = mesh->GetPoints()->RawPointer();
        size_t vertex_count = mesh->GetNumberOfPoints();
        size_t vertex_positions_stride = sizeof(float) * 3;
        float* vertex_attributes = nullptr;

        vertex_attributes = DynamicCast<FloatArray>(mesh->GetAttributeSet()->GetAttribute(index).pointer)->RawPointer();
        size_t vertex_attributes_stride = sizeof(float);
        float attribute_weights[1]{1};
        size_t attribute_count = 1;
        unsigned char* vertex_lock = nullptr;
        size_t target_index_count = index_count * 0.5;
        float target_error = 0.01f;
        unsigned int options = 0;
        float out_error = 0.0f;


        float* result_error = &out_error;

        destination = new unsigned int[index_count];

        clock_t start, end;

        start = clock();
        size_t result_size = simplify_trimesh_with_attriubtes(
                destination, indices, index_count, vertex_positions, vertex_count, vertex_attributes, attribute_weights,
                attribute_count, target_index_count, target_error, result_error);
        end = clock();
        std::cout << "simplify_trimesh_with_attriubtes cost time: " << end - start << std::endl;
        //size_t result_size = tri::simplifyWithAttributes(destination, indices, index_count, vertex_positions, vertex_count,
        //                               vertex_positions_stride, vertex_attributes, vertex_attributes_stride,
        //                               attribute_weights, attribute_count, vertex_lock, target_index_count,
        //                               target_error, options, result_error);

        //size_t result_size = meshopt_simplifyWithAttributes<unsigned int>(
        //        destination, indices, index_count, vertex_positions, vertex_count, vertex_positions_stride,
        //        vertex_attributes, vertex_attributes_stride, attribute_weights, attribute_count, vertex_lock,
        //        target_index_count, target_error, options, result_error);


        //size_t result_size =
        //        meshopt_simplifySloppy(destination, indices, index_count, vertex_positions, vertex_count,
        //                               vertex_positions_stride, target_index_count, target_error, result_error);
        auto Mesh = SurfaceMesh::New();
        auto Faces = CellArray::New();
        std::cout << result_size << std::endl;
        for (int i = 0; i < target_index_count / 3; i++) {
            Faces->AddCellId3(destination[i * 3], destination[i * 3 + 1], destination[i * 3 + 2]);
        }

        Mesh->SetPoints(mesh->GetPoints());
        Mesh->SetFaces(Faces);
        Mesh->SetAttributeSet(mesh->GetAttributeSet());
        modelTreeWidget->addDataObjectToModelTree(Mesh, Algorithm);
        rendererWidget->update();
    });
    */

    QAction* mesh_Sphere = ui->menu_filters->addAction("球形判断");
    connect(mesh_Sphere, &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!obj) return;
        if (obj->HasSubDataObject()) { obj = obj->GetSubDataObject(0); }
        QString result = "";
        auto mesh = DynamicCast<PointSet>(obj);
        auto array = DoubleArray::New();
        array->SetName("Compute_Radius");
        array->Reserve(mesh->GetNumberOfPoints());
        auto error = DoubleArray::New();
        error->SetName("Error");
        error->Reserve(mesh->GetNumberOfPoints());
        auto points = mesh->GetPoints();
        double radius = 0.0;
        double r = 0.0;
        double wucha = 0.0;
        double Min = 1e9;
        double Max = 0;
        for (int i = 0; i < mesh->GetNumberOfPoints(); i++) {
            auto p = mesh->GetPoint(i);
            r = sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
            array->AddValue(r);
        }
        auto bound = mesh->GetBoundingBox();
        double a = bound.max[0] - bound.min[0];
        double b = bound.max[1] - bound.min[1];
        double c = bound.max[2] - bound.min[2];
        radius = (a + b + c) / 6;
        double value = 0.0;
        for (int i = 0; i < mesh->GetNumberOfPoints(); i++) {
            value = abs(array->GetValue(i) - radius);
            wucha += value;
            error->AddValue(value);
            Max = std::max(Max, value);
            Min = std::min(Min, value);
        }
        std::cout << wucha << '\n';
        wucha /= mesh->GetNumberOfPoints();
        wucha /= radius;
        wucha *= 100;
        DoubleArray::Pointer range = DoubleArray::New();
        range->AddValue(0);
        range->AddValue(Max * 4);

        mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, array);
        mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, error, range);

        modelTreeWidget->updateAllAttriubute(mesh);
        result += "\n计算得到半径为" + QString::number(radius);
        result += "\n误差为" + QString::number(wucha) + "%";
        QMessageBox::information(this, "球形判断", result);
    });

    /*
	connect(mesh_processing->addAction("Simplification"), &QAction::triggered,
			this, [&](bool checked) {

		auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
		int index = DynamicCast<DrawObject>(obj)->GetAttributeIndex();
		if (index == -1) index = 0;
		Triangulation::Pointer triangulation = Triangulation::New();
		triangulation->SetInput(obj);
		triangulation->Execute();
		obj = triangulation->GetOutput();
		auto mesh = DynamicCast<SurfaceMesh>(obj);

		unsigned int* destination;
		unsigned int* indices = reinterpret_cast<unsigned int*>(mesh->GetFaces()->GetCellIdArray()->RawPointer());
		const size_t index_count = mesh->GetNumberOfFaces() * 3;
		float* vertex_positions = mesh->GetPoints()->RawPointer();
		size_t vertex_count = mesh->GetNumberOfPoints();
		size_t vertex_positions_stride = sizeof(float) * 3;
		float* vertex_attributes = nullptr;

		// vertex_attributes = DynamicCast<FloatArray>(mesh->GetAttributeSet()->GetAttribute(index).pointer)->RawPointer();
		size_t vertex_attributes_stride = sizeof(float);
		float attribute_weights[1]{1};
		size_t attribute_count = 0;
		unsigned char* vertex_lock = nullptr;
		size_t target_index_count = index_count * 0.1;
		float target_error = 0.01f;
		unsigned int options = 0;
		float out_error = 0.0f;


		float* result_error = &out_error;

		destination = new unsigned int[index_count];
		size_t result_size = meshsmp_simplifyTriMeshWithAttributes(
				indices, index_count, vertex_positions, vertex_count, vertex_attributes, attribute_count,
				attribute_weights, target_index_count, target_error, result_error);

		//size_t result_size = tri::simplifyWithAttributes(destination, indices, index_count, vertex_positions, vertex_count,
		//                               vertex_positions_stride, vertex_attributes, vertex_attributes_stride,
		//                               attribute_weights, attribute_count, vertex_lock, target_index_count,
		//                               target_error, options, result_error);

		//size_t result_size =
		//        meshopt_simplify(destination, indices, index_count, vertex_positions, vertex_count,
		//                         vertex_positions_stride, target_index_count, target_error, options, result_error);


		//size_t result_size =
		//        meshopt_simplifySloppy(destination, indices, index_count, vertex_positions, vertex_count,
		//                               vertex_positions_stride, target_index_count, target_error, result_error);
		auto Mesh = SurfaceMesh::New();
		auto Faces = CellArray::New();
		std::cout << result_size << std::endl;
		for (int i = 0; i < target_index_count / 3; i++) {
			Faces->AddCellId3(destination[i * 3], destination[i * 3 + 1], destination[i * 3 + 2]);
		}

		Mesh->SetPoints(mesh->GetPoints());
		Mesh->SetFaces(Faces);
		Mesh->SetAttributeSet(mesh->GetAttributeSet());
		modelTreeWidget->addDataObjectToModelTree(Mesh, Algorithm);
		rendererWidget->update();
			});
	connect(mesh_processing->addAction("Simplification"), &QAction::triggered,
			this, [&](bool checked) {

			//SurfaceMesh::Pointer mesh = SurfaceMesh::New();
			//clock_t start, end;
			//start = clock();
			//Points::Pointer points = Points::New();
			//CellArray::Pointer cells = CellArray::New();
			//for (int i = 0; i < 10000000; i++) {
			//    points->AddPoint(Point(1, 0, 0));
			//    points->AddPoint(Point(0, 1, 0));
			//    points->AddPoint(Point(0, 0, 1));
			//    igIndex face[3]{i * 3 + 0, i * 3 + 1, i * 3 + 2};
			//    cells->AddCellIds(face, 3);
			//}
			//mesh->SetPoints(points);
			//mesh->SetFaces(cells);
			//end = clock();
			//std::cout << end - start << std::endl;
			//start = clock();
			//for (int i = 0; i < 10000000; i++) {
			//    igIndex face[3]{};
			//    mesh->GetFacePointIds(i, face);
			//    if (face[0] == 0) { face[0] = 1; }
			//}
			//end = clock();
			//std::cout << end - start << std::endl;
			//return;
			if(false){
				auto getCot = [](const Vector3d& a, const Vector3d& b, const Vector3d& c) -> double {
						auto ba = a - b;
						auto ca = a - c;
						return ba.dot(ca) / ba.cross(ca).length();
				};
				auto getArea = [](const Vector3d& a, const Vector3d& b, const Vector3d& c) -> double {
					Vector3d d10 = b - a;
					Vector3d d20 = c - a;

					return CrossProduct(d10, d20).norm() / 2;
				};


				auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
				int index = DynamicCast<DrawObject>(obj)->GetAttributeIndex();
				if (index == -1) index = 0;
				Triangulation::Pointer triangulation = Triangulation::New();
				triangulation->SetInput(obj);
				triangulation->Execute();
				obj = triangulation->GetOutput();
				auto mesh = DynamicCast<SurfaceMesh>(obj);

				//auto mesh = DynamicCast<SurfaceMesh>(DynamicCast<UnstructuredMesh>(obj)->TransferToSurfaceMesh());

				auto& attrb = mesh->GetAttributeSet()->GetAttribute(index).pointer;
				int d = attrb->GetDimension();
				FloatArray::Pointer scalar = FloatArray::New();
				scalar->SetDimension(d);
				mesh->RequestEditStatus();

				std::vector<double> L(mesh->GetNumberOfPoints());

				for (int i = 0; i < mesh->GetNumberOfPoints(); i++) {
					double area = 0.0;
					double val[8]{0.0};
					double sum_cot = 0.0;
					igIndex ids[64]{0};
					int size = mesh->GetPointToNeighborEdges(i, ids);
					for (int j = 0; j < size; j++) {
						igIndex e[2]{0}, id[2]{0}, f[3]{0};
						mesh->GetEdgePointIds(ids[j], e);
						int size1 = mesh->GetEdgeToNeighborFaces(ids[j], id);
						double cot = 0.0;
						for (int k = 0; k < size1; k++) {
							mesh->GetFacePointIds(id[k], f);
							cot += getCot(mesh->GetPoint(f[0] + f[1] + f[2] - e[0] - e[1]), mesh->GetPoint(e[0]),
								   mesh->GetPoint(e[1]));
						}
						cot /= size1;
						sum_cot += cot;

						for (int k = 0; k < d; k++) {
							val[k] += cot *
									  (attrb->GetValue(i * d + k) - attrb->GetValue((e[0] + e[1] - i) * d + k));
						}
					}
					for (int k = 0; k < d; k++) {
						val[k] = val[k] / sum_cot;
					}
					scalar->AddElement(val);
					L[i] = val[0];
					//igIndex ids[64]{0};
					//int size = mesh->GetPointToNeighborFaces(i, ids);
					//double p = attrb->GetValue(i);
					//Point v = mesh->GetPoint(i);
					//for (int j = 0; j < size; j++) {
					//    igIndex f[3]{0};
					//    mesh->GetFacePointIds(ids[j], f);
					//    val += (p - (attrb->GetValue(f[0]) + attrb->GetValue(f[1]) + attrb->GetValue(f[2])) / 3) /
					//           (v - (mesh->GetPoint(f[0]) + mesh->GetPoint(f[1]) + mesh->GetPoint(f[2])) / 3).norm();
					//}
					//val /= size;
					//scalar->AddValue(val);
				}
				//double minVal = 1e27;
				//double maxVal = -1e27;
				//for (int i = 0; i < scalar->GetNumberOfValues(); i++) {
				//    //if (std::abs(scalar->GetValue(i)) > 100) {
				//    //    scalar->SetValue(i, 100 * scalar->GetValue(i) / std::abs(scalar->GetValue(i)));
				//    //}
				//    double val = scalar->GetValue(i);
				//    minVal = std::min(minVal, val);
				//    maxVal = std::max(maxVal, val);
				//}
				//for (int i = 0; i < scalar->GetNumberOfValues(); i++) {
				//    scalar->SetValue(i, (scalar->GetValue(i) - minVal) / (maxVal - minVal));
				//}


				//SurfaceMesh::Pointer Mesh = SurfaceMesh::New();
				//igIndex edgeId = 34523;
				//igIndex e[2]{0};
				//mesh->GetEdgePointIds(edgeId, e);
				//double L1 = L[e[0]];
				//double L2 = L[e[1]];
				//auto& p1 = mesh->GetPoint(e[0]);
				//auto& p2 = mesh->GetPoint(e[1]);
				//igIndex ids[64]{}, f[2]{}, fids[3]{};
				//auto new_p = (p1 + p2) / 2;
				//int size = mesh->GetEdgeToOneRingFaces(edgeId, ids);
				//mesh->GetEdgeToNeighborFaces(edgeId, f);
				//CellArray::Pointer cells = CellArray::New();
				//std::vector<int> visited(mesh->GetNumberOfFaces(), 0);
				//for (int i = 0; i < size; i++) {
				//    if (ids[i] != f[0] && ids[i] != f[1]) {
				//        mesh->GetFacePointIds(ids[i], fids);
				//        for (int j = 0; j < 3; j++) {
				//            if (fids[j] == e[0] || fids[j] == e[1]) {
				//                fids[j] = e[0];
				//                break;
				//            }
				//        }
				//        visited[ids[i]] = 1;
				//        cells->AddCellIds(fids, 3);
				//    }
				//}
				//mesh->SetPoint(e[0], new_p);
				//mesh->SetPoint(e[1], new_p);
				//for (int i = 0; i < mesh->GetNumberOfFaces(); i++) {
				//    if (visited[i] == 0) {
				//        mesh->DeleteFace(i);
				//    }
				//}
				//
				//mesh->GarbageCollection();
				mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, scalar);
				modelTreeWidget->addDataObjectToModelTree(mesh, Algorithm);
				rendererWidget->update();


				//Mesh->SetPoints(mesh->GetPoints());
				//Mesh->SetFaces(cells);
				//modelTreeWidget->addDataObjectToModelTree(Mesh, Algorithm);
				//rendererWidget->update();

				return;
			}


		if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;

		Triangulation::Pointer triangulation = Triangulation::New();
		auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

		triangulation->SetInput(obj);
		triangulation->Execute();
		obj = triangulation->GetOutput();

		Simplification::Pointer filter = Simplification::New();
		filter->SetTargetReduction(0.0001);
		filter->SetInput(obj);
		filter->SetAllScalarCheck(false);
		filter->SetActivedAttribIndices({0, 1, 2});
		filter->Execute();

		modelTreeWidget->addDataObjectToModelTree(obj, Algorithm);
		rendererWidget->update();
		return;
		bool flag = false;
		switch (obj->GetDataObjectType()) {
			case IG_SURFACE_MESH:
				flag = true;
				break;
			case IG_UNSTRUCTURED_MESH: {
				auto mesh = DynamicCast<UnstructuredMesh>(obj)->TransferToSurfaceMesh();
				if (mesh) {
					obj = mesh;
					flag = true;
					break;
				} else {
					mesh = DynamicCast<UnstructuredMesh>(obj)->TransferToVolumeMesh();
					if (mesh) {
						auto show = DynamicCast<UnstructuredMesh>(obj)->GetDisplayObject();
						if (show) {
							flag = true;
							obj = show;
						}
					}
				}
			} break;
			default:
				break;
		}

		//    bool flag = false;
		//    switch (obj->GetDataObjectType()) {
		//        case IG_SURFACE_MESH:
		//            flag = true;
		//            break;
		//        case IG_UNSTRUCTURED_MESH:
		//        {
		//            auto mesh = DynamicCast<UnstructuredMesh>(obj)
		//                                ->TransferToSurfaceMesh();
		//            if (mesh) {
		//                obj = mesh;
		//                flag = true;
		//                break;
		//            }
		//            else {
		//                mesh = DynamicCast<UnstructuredMesh>(obj)
		//                               ->TransferToVolumeMesh();
		//                if (mesh) {
		//                    auto show = DynamicCast<UnstructuredMesh>(obj)
		//                                        ->GetDisplayObject();
		//                    if (show) {
		//                        flag = true;
		//                        obj = show;
		//                    }
		//                }
		//            }
		//        }
		//            break;
		//        default:
		//            break;
		//    }

		//    triangulation->SetModel(rendererWidget->GetScene()->GetCurrentModel());
		//    triangulation->SetInput(obj);
		//    triangulation->Execute();

		//    obj = triangulation->GetOutput();
		//

		//Simplification::Pointer filter = Simplification::New();
		////Gradient::Pointer filter = Gradient::New();
		//filter->SetInput(obj);
		////filter->SetModel(rendererWidget->GetScene()->GetCurrentModel());
		//filter->Execute();

		////auto mesh = DynamicCast<SurfaceMesh>(obj);
		////mesh->RequestEditStatus();
		////igIndex ids[8]{};
		////for (int i = 0; i < mesh->GetNumberOfEdges(); i++) {
		////    int size = mesh->GetEdgeToNeighborFaces(i, ids);
		////    if (size > 2) { std::cout << "123\n"; }
		////}
		//modelTreeWidget->addDataObjectToModelTree(obj, Algorithm);
		//rendererWidget->update();
	});
	*/
    connect(ui->action_test_02, &QAction::triggered, this, [&](bool checked) {
        StructuredMesh::Pointer mesh = StructuredMesh::New();
        Points::Pointer points = Points::New();
        int x = 100, y = 100, z = 100;
        igIndex dim[3] = {x, y, z};
        double step[3] = {1.0 / x, 1.0 / y, 1.0 / z};
        points->Reserve(x * y * z);
        for (int i = 0; i < x; i++) {
            for (int j = 0; j < y; j++) {
                for (int k = 0; k < z; k++) {
                    points->AddPoint(Point{float(i * step[0]), float(j * step[1]), float(k * step[2])});
                }
            }
        }
        mesh->SetDimensionSize(dim);
        mesh->SetPoints(points);
        mesh->GenStructuredCellConnectivities();
        mesh->SetName("undefined_mesh");
        modelTreeWidget->addDataObjectToModelTree(mesh, ItemSource::File);
    });

    connect(ui->action_test_03, &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        SurfaceMesh::Pointer mesh = SurfaceMesh::New();
        Points::Pointer points = Points::New();
        points->AddPoint(0, 0, 0);
        points->AddPoint(1, 0, 0);
        points->AddPoint(0, 1, 0);

        CellArray::Pointer faces = CellArray::New();
        faces->AddCellId3(0, 1, 2);

        mesh->SetPoints(points);
        mesh->SetFaces(faces);
        mesh->SetName("undefined_mesh");
        rendererWidget->AddDataObject(mesh);
    });

    connect(ui->action_test_04, &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        SurfaceMeshFilterTest::Pointer fp = SurfaceMeshFilterTest::New();
        fp->SetInput(rendererWidget->GetScene()->GetCurrentModel()->GetDataObject());
        fp->Execute();
        rendererWidget->update();
    });

    // connect(ui->action_test_05, &QAction::triggered, this, [&](bool checked) {
    //     VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(
    //         rendererWidget->GetScene()->GetCurrentModel()->GetDataObject());
    //     for (int i = 0; i < 100; i++) {
    //         mesh->DeleteVolume(i);
    //     }

    //});

    connect(ui->action_test_06, &QAction::triggered, this, [&](bool checked) {
        //      LineSource::Pointer source = LineSource::New();
        //      Points::Pointer points = Points::New();
        //      FloatArray::Pointer colors = FloatArray::New();
        //      CellArray::Pointer polylines = CellArray::New();
        //      CellArray::Pointer lines = CellArray::New();
        //      IdArray::Pointer pl = IdArray::New();
        //      for (int i = 0; i < 10; ++i) {
        //          float theta = 2 * M_PI * i / 10;
        //          float x = std::cos(theta);
        //          float y = std::sin(theta);
        //          IGsize ptId = points->AddPoint(Point{ x, y, 0.0f});
        //          colors->AddValue((x + y) / 2);
        //          pl->AddId(ptId);
        //      }
        //      polylines->AddCellIds(pl);

        // IGsize ptId0 = points->A
        //  ddPoint(Point{0, 0, 0});
        // IGsize ptId1 = points->AddPoint(Point{0, 0, 1});
        //       colors->AddValue(0.3);
        //       colors->AddValue(0.4);
        //       lines->AddCellId2(ptId0, ptId1);

        //      source->SetPoints(points);
        //      source->SetColors(colors);
        //      source->SetLines(lines);
        //      source->SetPolyLines(polylines);
        //      source->SetName("undefined_line_source");
        //      rendererWidget->AddDataObject(source);
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        UnstructuredMesh::Pointer mesh = UnstructuredMesh::New();
        Points::Pointer points = Points::New();
        CellArray::Pointer cells = CellArray::New();
        UnsignedIntArray::Pointer types = UnsignedIntArray::New();
        FloatArray::Pointer property = FloatArray::New();
        property->SetName("scalar");

        points->AddPoint(-0.5, -0.5, 0);
        points->AddPoint(0.5, -0.5, 0);
        points->AddPoint(0, 0.5, 0);
        points->AddPoint(0, 0, 0.6);

        points->AddPoint(1, 1, 0);
        points->AddPoint(2, 1, 0);
        points->AddPoint(1, 2, 0);

        property->AddValue(-0.5);
        property->AddValue(0.5);
        property->AddValue(0);
        property->AddValue(0);
        property->AddValue(1);
        property->AddValue(2);
        property->AddValue(1);

        cells->AddCellId4(0, 1, 2, 3);
        types->AddValue(IG_TETRA);

        cells->AddCellId3(4, 5, 6);
        types->AddValue(IG_TRIANGLE);

        StringArray::Pointer sa = StringArray::New();
        sa->AddElement("scalar");

        mesh->GetMetadata()->AddStringArray(ATTRIBUTE_NAME_ARRAY, sa);
        mesh->GetAttributeSet()->AddScalar(IG_POINT, property);
        mesh->SetPoints(points);
        mesh->SetCells(cells, types);
        mesh->SetName("undefined_unstructured_mesh");

        modelTreeWidget->addDataObjectToModelTree(mesh, ItemSource::File);
        // this->updateCurrentDataObject();
    });


    //    auto action_subdivision = ui->menuTest->addAction("rgbscalar");
    //    connect(action_subdivision, &QAction::triggered, this, [&](bool checked) {
    //        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
    //        auto input = rendererWidget->GetScene()
    //            ->GetCurrentModel()
    //            ->GetDataObject();
    //        auto mesh=DynamicCast<UnstructuredMesh>(input)->TransferToSurfaceMesh();
    //        auto attributeset=input->GetAttributeSet();
    //        double rgb[3]={0,0,0};
    //        int fcnt= mesh->GetNumberOfFaces();
    //        auto array = DoubleArray::New();
    //        array->SetDimension(3);
    //        array->Reserve(fcnt);
    //        array->SetName("rgb");
    //        for (int i = 0; i < fcnt; i++) {
    //            rgb[1]=double(i)/double(fcnt);
    //            array->AddElement(rgb);
    //        }
    //        attributeset->AddAttribute(IG_RGB,IG_CELL,array);
    //        modelTreeWidget->addDataObjectToModelTree(mesh,
    //            ItemSource::File);
    //    });

    //     cp


    QMenu* view = ui->menu_filters->addMenu("特征提取");
    QAction* curvature = view->addAction("Get Curvature");
    connect(curvature, &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        CurvatureFilter::Pointer filter = CurvatureFilter::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        if (filter->Execute()) { modelTreeWidget->updateAllAttriubute(data); }
    });

    QAction* gradient = view->addAction("Get Gradient");
    connect(gradient, &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        GradientFilter::Pointer filter = GradientFilter::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        if (filter->Execute()) { modelTreeWidget->updateAllAttriubute(data); }
    });

    QAction* laplacian = view->addAction("Get Laplacian");
    connect(laplacian, &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        LaplacianFilter::Pointer filter = LaplacianFilter::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        if (filter->Execute()) { modelTreeWidget->updateAllAttriubute(data); }
    });

    QAction* vortex = view->addAction("Get Vortex");
    connect(vortex, &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        VortexFilter::Pointer filter = VortexFilter::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();


        //auto mesh = data->GetDisplayObject();
        //if (mesh) {
        //    filter->SetInput(mesh);
        //    filter->Execute();
        //    modelTreeWidget->addDataObjectToModelTree(mesh, Algorithm);
        //

        //} else {
        //    filter->SetInput(data);
        //    filter->Execute();
        //    modelTreeWidget->updateAllAttriubute(data);
        //}

        filter->SetInput(data);
        if (filter->Execute()) { modelTreeWidget->updateAllAttriubute(data); }
    });
}

void igQtMainWindow::initAllDockWidgetConnectWithAction() {
    // connect(ui->action_SearchInfo, &QAction::triggered, this, [&](bool checked)
    // { 	ui->dockWidget_SearchInfo->sh
    //  ow();
    //	});
    connect(ui->action_IsShowColorBar, &QAction::triggered, this, &igQtMainWindow::updateColorBarShow);
    connect(ui->action_ExportAnimation, &QAction::triggered, this,
            [&](bool checked) { ui->dockWidget_Animation->show(); });
    connect(ui->action_Scalar, &QAction::triggered, this, [&](bool checked) { ui->dockWidget_ScalarField->show(); });
    connect(ui->action_Vector, &QAction::triggered, this, [&](bool checked) {
        ui->dockWidget_VectorField->show();
        ui->widget_VectorField->updateVectorNameList();
    });
    connect(ui->action_Scalar, &QAction::triggered, this, [&](bool checked) { ui->dockWidget_ScalarField->show(); });
    connect(ui->action_Glyph, &QAction::triggered, this, [&](bool checked) {
        ui->dockWidget_VectorField->show();
        ui->widget_VectorField->updateVectorNameList();
    });
    connect(ui->action_Tensor, &QAction::triggered, this, [&](bool checked) {
        ui->dockWidget_TensorField->show();
        ui->widget_TensorField->UpdateScalarsNameList();
        ui->widget_TensorField->UpdateTensorsNameList();
    });
    connect(ui->action_FlowField, &QAction::triggered, this, [&](bool checked) {
        ui->dockWidget_FlowField->show();
        ui->widget_FlowField->updateVectorNameList();
    });
    // connect(ui->action_SearchInfo, &QAction::triggered, this, [&](bool checked)
    // { 	ui->dockWidget_SearchInfo->show();
    //	});
    //  connect(ui->action_EditMode, &QAction::triggered, this, [&](bool checked)
    //  {
    //	ui->dockWidget_EditMode->show();
    //	});
    //  connect(ui->action_QualityDetection, &QAction::triggered, this, [&](bool
    //  checked) { 	ui->dockWidget_QualityDetection->show();
    //	});
    connect(ui->action_ContourExtract, &QAction::triggered, this, [&](bool checked) {
        ui->dockWidget_ContourExtract->show();
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        if (!scene) return;
        auto CurrentModel = scene->GetCurrentModel();
        if (!CurrentModel) return;
        auto dataObject = CurrentModel->GetDataObject();
        if (!dataObject) return;
        ui->widget_ContourExtract->SetOriginDataObject(dataObject);
    });
    connect(ui->action_GenerateChart, &QAction::triggered, this, [&](bool checked) {
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        if (!scene) return;
        auto CurrentModel = scene->GetCurrentModel();
        if (!CurrentModel) return;
        auto dataObject = CurrentModel->GetDataObject();
        if (!dataObject) return;
        auto attributeSet = dataObject->GetAttributeSet();
        auto attrIndex = dataObject->GetAttributeIndex();
        auto attrDimension = dataObject->GetAttributeDimension();
        if (attrIndex < 0) { return; }
        auto array = attributeSet->GetAttribute(attrIndex).pointer;
        if (array == nullptr) return;
        ArrayObject::Pointer drawArray = nullptr;
        if (array->GetDimension() <= 1) {
            drawArray = array;
        } else {
            auto tmpArray = FloatArray::New();
            int size = array->GetNumberOfElements();
            tmpArray->Reserve(size);
            tmpArray->SetName(array->GetName());
            for (int i = 0; i < size; i++) { tmpArray->AddValue(array->GetElementValue(i, attrDimension)); }
            drawArray = tmpArray;
        }
        auto chart = new igQtCharts;
        chart->drawBarChart(drawArray);
        chart->exec();
    });
    auto DrawSurfaceMeshByPointer = [](SurfaceMesh::Pointer m, Painter3D* painter, const float color[3]) -> void {
        // 1. draw faces
        painter->SetPen(Pen::Style::NoPen);
        painter->SetBrush(color[0], color[1], color[2]);
        igIndex cell[32]{};
        for (int i = 0; i < m->GetNumberOfFaces(); i++) {
            int ncell = m->GetFacePointIds(i, cell);
            for (int j = 2; j < ncell; j++) {
                painter->DrawTriangle(m->GetPoint(cell[0]), m->GetPoint(cell[j - 1]), m->GetPoint(cell[j]));
            }
        }
        // 2. draw lines
        painter->SetPen(Color::Black);
        painter->SetBrush(Brush::Style::NoBrush);
        if (m->GetEdges() == nullptr) { m->BuildEdges(); }
        for (int i = 0; i < m->GetNumberOfEdges(); i++) {
            int ncell = m->GetEdgePointIds(i, cell);
            if (cell[0] < 0 || cell[1] < 0) {
                throw std::runtime_error("The index of the edge is negative.");
            } else {
                painter->DrawLine(m->GetPoint(cell[0]), m->GetPoint(cell[1]));
            }
        }
        painter->Modified();
    };

    auto AddClippingMeshToScene = [DrawSurfaceMeshByPointer](const std::string& mainName, SurfaceMesh::Pointer OV,
                                                             SurfaceMesh::Pointer t_IV, SurfaceMesh::Pointer OIV,
                                                             igQtModelDialogWidget* modelTreeWidget) {
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

        const std::string OVName = "__" + mainName + "_OV";   // 临时模型
        const std::string IVName = "__" + mainName + "_IV";   // 临时模型
        const std::string OIVName = "__" + mainName + "_OIV"; // 临时模型
        const float OVColor[3]{1.f, 1.f, 1.f};
        const float IVColor[3]{1.f, 1.f, 0.f};
        const float OIVColor[3]{1.f, 1.f, 1.f};
        const float OIVAlpha = 0.2f;

        SurfaceMesh::Pointer IV = SurfaceMesh::New();
        OV->SetName(OVName);
        IV->SetName(IVName);
        OIV->SetName(OIVName);

        Model* IVModel{nullptr};
        bool exist[3]{false, false, false};

        auto modelList = scene->GetModelList();
        for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
            auto id = it->first;
            auto model = it->second;

            if (model->GetDataObject()->GetName() == OVName) {
                auto model = scene->GetModelById(id);
                model->SetDataObject(OV);
                exist[0] = true;
            } else if (model->GetDataObject()->GetName() == IVName) {
                auto model = scene->GetModelById(id);
                model->SetDataObject(IV);
                exist[1] = true;
                IVModel = model;
            } else if (model->GetDataObject()->GetName() == OIVName) {
                auto model = scene->GetModelById(id);
                model->SetDataObject(OIV);
                exist[2] = true;
            }
        }
        if (!exist[0]) modelTreeWidget->addDataObjectToModelTree(OV, ItemSource::Algorithm);
        if (!exist[1]) {
            int id = modelTreeWidget->addDataObjectToModelTree(IV, ItemSource::Algorithm);
            IVModel = scene->GetModelById(id);
        }
        if (!exist[2]) modelTreeWidget->addDataObjectToModelTree(OIV, ItemSource::Algorithm);

        DrawSurfaceMeshByPointer(t_IV, IVModel->GetPainter3D(), IVColor);

        //OV->SetFaceColor(OVColor);
        OV->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
        //IV->SetFaceColor(IVColor);
        //IV->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
        //OIV->SetFaceColor(OIVColor);
        OIV->SetTransparency(OIVAlpha);
        OIV->SetViewStyle(IG_SURFACE);
    };

    connect(ui->action_BoxClipping_Better, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        bool ok;
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto oldCurrentModel = scene->GetCurrentModel();
        auto dataObject = scene->GetCurrentModel()->GetDataObject();
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        static std::vector<int> supportTypes = {IG_VOLUME_MESH, IG_UNSTRUCTURED_MESH, IG_STRUCTURED_MESH};
        if (std::find(supportTypes.begin(), supportTypes.end(), dataObject->GetDataObjectType()) ==
            supportTypes.end()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        auto inputMesh = DynamicCast<iGame::DataObject>(dataObject);
        auto box = inputMesh->GetBoundingBox();
        auto center = (box.min + box.max) * 0.5;
        auto size = box.max - box.min;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
        int x_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_min(0..1)", "0.0");
        int y_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_min(0..1)", "0.0");
        int z_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_min(0..1)", "0.0");
        int x_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_max(0..1)", "0.5");
        int y_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_max(0..1)", "1.0");
        int z_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_max(0..1)", "1.0");
        int flip_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip", "false");
        dialog->show();

        dialog->setApplyFunctor([=]() {
            bool ok;
            auto Clamp = [](double x, double l, double r) -> double {
                if (x < l) return l;
                if (x > r) return r;
                return x;
            };

            double x_min = box.min[0] + size[0] * Clamp(dialog->getDouble(x_min_id, ok), 0., 1.);
            double y_min = box.min[1] + size[1] * Clamp(dialog->getDouble(y_min_id, ok), 0., 1.);
            double z_min = box.min[2] + size[2] * Clamp(dialog->getDouble(z_min_id, ok), 0., 1.);
            double x_max = box.min[0] + size[0] * Clamp(dialog->getDouble(x_max_id, ok), 0., 1.);
            double y_max = box.min[1] + size[1] * Clamp(dialog->getDouble(y_max_id, ok), 0., 1.);
            double z_max = box.min[2] + size[2] * Clamp(dialog->getDouble(z_max_id, ok), 0., 1.);
            bool flip = dialog->getChecked(flip_id, ok);

            auto clipper = iGameVolumeMeshClipper::New();
            clipper->SetInput(0, dataObject);
            clipper->SetExtent(x_min, x_max, y_min, y_max, z_min, z_max, flip);
            clipper->Execute();
            auto OV = DynamicCast<SurfaceMesh>(clipper->GetOutput(0));
            auto IV = DynamicCast<SurfaceMesh>(clipper->GetOutput(1));
            auto OIV = DynamicCast<SurfaceMesh>(clipper->GetOutput(2));
            AddClippingMeshToScene(dataObject->GetName(), OV, IV, OIV, modelTreeWidget);
            drawObject->SetVisibility(false);
            scene->SetCurrentModel(oldCurrentModel);
            rendererWidget->update();
        });
    });

    connect(ui->action_PlaneClipping_Better, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        bool ok;
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto oldCurrentModel = scene->GetCurrentModel();
        auto dataObject = scene->GetCurrentModel()->GetDataObject();
        auto drawObject = DynamicCast<DrawObject>(dataObject);
        static std::vector<int> supportTypes = {IG_VOLUME_MESH, IG_UNSTRUCTURED_MESH, IG_STRUCTURED_MESH};
        if (std::find(supportTypes.begin(), supportTypes.end(), dataObject->GetDataObjectType()) ==
            supportTypes.end()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        auto inputMesh = DynamicCast<iGame::DataObject>(dataObject);
        auto box = inputMesh->GetBoundingBox();
        auto center = (box.min + box.max) * 0.5;
        auto size = box.max - box.min;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
        int origin_x_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_x(0..1)", "0.5");
        int origin_y_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_y(0..1)", "0.5");
        int origin_z_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_z(0..1)", "0.5");
        int normal_x_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_x(-1..1)", "1.0");
        int normal_y_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_y(-1..1)", "0.0");
        int normal_z_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_z(-1..1)", "0.0");
        int flip_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip", "false");
        dialog->show();

        dialog->setApplyFunctor([=]() {
            bool ok;
            auto Clamp = [](double x, double l, double r) -> double {
                if (x < l) return l;
                if (x > r) return r;
                return x;
            };

            double origin_x = box.min[0] + size[0] * Clamp(dialog->getDouble(origin_x_id, ok), 0., 1.);
            double origin_y = box.min[1] + size[1] * Clamp(dialog->getDouble(origin_y_id, ok), 0., 1.);
            double origin_z = box.min[2] + size[2] * Clamp(dialog->getDouble(origin_z_id, ok), 0., 1.);
            double normal_x = Clamp(dialog->getDouble(normal_x_id, ok), -1., 1.);
            double normal_y = Clamp(dialog->getDouble(normal_y_id, ok), -1., 1.);
            double normal_z = Clamp(dialog->getDouble(normal_z_id, ok), -1., 1.);
            bool flip = dialog->getChecked(flip_id, ok);
            if (normal_x == 0. && normal_y == 0. && normal_z == 0.) {
                std::cout << "Normal is a vector of zero" << std::endl;
                return;
            }

            auto clipper = iGameVolumeMeshClipper::New();
            clipper->SetInput(0, dataObject);
            clipper->SetPlane(origin_x, origin_y, origin_z, normal_x, normal_y, normal_z, flip);
            clipper->Execute();
            auto OV = DynamicCast<SurfaceMesh>(clipper->GetOutput(0));
            auto IV = DynamicCast<SurfaceMesh>(clipper->GetOutput(1));
            auto OIV = DynamicCast<SurfaceMesh>(clipper->GetOutput(2));
            AddClippingMeshToScene(dataObject->GetName(), OV, IV, OIV, modelTreeWidget);
            drawObject->SetVisibility(false);
            scene->SetCurrentModel(oldCurrentModel);
            rendererWidget->update();
        });
    });

    connect(ui->action_slice, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene() || !rendererWidget->GetScene()->GetCurrentModel()) { return; }
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!obj) return;
        //if (!rendererWidget->getInteractor()->IsBase()) {
        //    rendererWidget->getInteractor()->RequestBasicStyle();
        //    return;
        //}
        if (SliceDockWidget->isHidden() == false) { return; }
        SliceDockWidget->show();
        SliceWidget->SetOriginDataObject(obj);

        rendererWidget->getInteractor()->SetDataObject(obj);
        rendererWidget->getInteractor()->SetPainter3D(rendererWidget->GetScene()->GetCurrentModel()->GetPainter3D());

        //if (rendererWidget->GetScene()->GetInteractor()) {
        //    rendererWidget->GetScene()->GetInteractor()->SetCallBack(&igQtModelClipWidget::FilterSignal, SliceWidget);
        //}

        rendererWidget->getInteractor()->RequestSlicingStyle(SliceWidget->GetSelection());
    });
    connect(SliceWidget, &igQtModelClipWidget::DrawClipModel, this,
            [&](DrawObject::Pointer mesh) { modelTreeWidget->addDataObjectToModelTree(mesh, ItemSource::Algorithm); });
    connect(SliceWidget, &igQtModelClipWidget::UpdateClipModel, this, [&](DrawObject::Pointer mesh) {
        modelTreeWidget->updateCurrentModelInfo();
        rendererWidget->update();
    });
    connect(SliceWidget, &igQtModelClipWidget::ResetInteractor, this, [&]() {
        if (!rendererWidget->getInteractor()->IsBasicStyle()) {
            rendererWidget->getInteractor()->RequestBasicStyle();
            return;
        }
    });
    connect(ui->action_deformation, &QAction::triggered, this, [&](bool checked) {
        if (checked) DeformationDockWidget->show();
        else
            DeformationDockWidget->hide();
    });
    QAction* action_SetAccelerate = ui->menu_file->addAction("关闭/启用加速结构");
    connect(action_SetAccelerate, &QAction::triggered, this, [&](bool checked) {
        this->modelTreeWidget->SetAccelerateState(!this->modelTreeWidget->GetAccelerateState());
    });
}
void igQtMainWindow::initAllMySignalConnections() {
    // connect(rendererWidget, &igQtModelDrawWidget::insertToModelListView,
    // ui->modelTreeView, &igQtModelListView::InsertModel);

    connect(fileLoader, &igQtFileLoader::NewModel, modelTreeWidget, &igQtModelDialogWidget::addDataObjectToModelTree);
    connect(fileLoader, &igQtFileLoader::FinishReading, this, &igQtMainWindow::updateRecentFilePaths);
    connect(ui->action_DeleteMesh, &QAction::triggered, modelTreeWidget, &igQtModelDialogWidget::deleteCurrentModel);


    // connect(fileLoader, &igQtFileLoader::FinishReading, this,
    // &igQtMainWindow::updateViewStyleAndCloudPicture); connect(fileLoader,
    // &igQtFileLoader::FinishReading, this,
    // &igQtMainWindow::updateCurrentSceneWidget);
    connect(fileLoader, &igQtFileLoader::FinishReading, ui->widget_Animation,
            &igQtAnimationWidget::initAnimationComponents);
    connect(fileLoader, &igQtFileLoader::FinishReading, DeformationWidget, &igQtDeformationWidget::updateInfo);


    connect(ui->widget_FlowField, &igQtStreamTracerWidget::AddStreamObject, this, [&](iGame::DataObject::Pointer res) {
        modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
    });
    connect(ui->widget_FlowField, &igQtStreamTracerWidget::UpdateStreamObject, this,
            [&](iGame::DataObject::Pointer res) {
                res->Modified();
                rendererWidget->update();
            });
    // connect(fileLoader, &igQtFileLoader::FinishReading, ui->widget_ScalarField,
    // &igQtScalarViewWidget::getScalarsName); connect(fileLoader,
    // &igQtFileLoader::FinishReading, ui->widget_TensorField, [&]() {
    //	ui->widget_TensorField->UpdateTensorsNameList();
    //	});
    // connect(fileLoader, &igQtFileLoader::FinishReading, ui->widget_SearchInfo,
    // &igQtSearchInfoWidget::updateDataProducer);

    connect(fileLoader, &igQtFileLoader::AddFileToModelList, ui->modelTreeView, &igQtModelListView::AddModel);
    // connect(rendererWidget, &igQtRenderWidget::AddDataObjectToModelList,
    // ui->modelTreeView, &igQtModelListView::AddModel); connect(rendererWidget,
    // &igQtRenderWidget::UpdateCurrentDataObject, this,
    // &igQtMainWindow::updateCurrentDataObject);

    // connect(fileLoader, &igQtFileLoader::LoadAnimationFile,
    // ui->widget_Animation, &igQtAnimationWidget::initAnimationComponents);

    connect(ui->widget_Animation, &igQtAnimationWidget::UpdateScene, this, &igQtMainWindow::UpdateRenderingWidget);

    // connect(ui->widget_Animation,
    // &igQtAnimationWidget::PlayAnimation_interpolate, rendererWidget,
    // &igQtModelDrawWidget::PlayAnimation_interpolate);

    // connect(ui->widget_FlowField, &igQtStreamTracerWidget::sendstreams,
    // rendererWidget, &igQtModelDrawWidget::DrawStreamline);
    // connect(ui->widget_FlowField, &igQtStreamTracerWidget::updatestreams,
    // rendererWidget, &igQtModelDrawWidget::UpdateStreamline);

    // connect(ui->modelTreeView, &igQtModelListView::UpdateCurrentScene, this,
    // &igQtMainWindow::updateCurrentSceneWidget); connect(ui->modelTreeView,
    // &igQtModelListView::UpdateCurrentItemToOtherQtModule, this,
    // &igQtMainWindow::updateCurrentDataObject);

    // connect(ui->modelTreeView, &igQtModelListView::ChangeModelVisible,
    // rendererWidget, &igQtModelDrawWidget::changeTargetModelVisible);

    //connect(ui->widget_QualityDetection,
    //&igQtQualityDetectionWidget::updateCurrentModelColor, rendererWidget,
    //&igQtModelDrawWidget::UpdateCurrentModel);
    connect(ui->widget_ScalarField, &igQtScalarViewWidget::changeColorBarShow, this,
            &igQtMainWindow::updateColorBarShow);
    connect(this->modelTreeWidget, &igQtModelDialogWidget::CloudPictureChanged, ui->widget_ScalarField,
            &igQtScalarViewWidget::showScalarView);
    connect(ui->widget_ScalarField, &igQtScalarViewWidget::ChangeShowColorManager, this, [&]() {
        if (this->ColorManagerWidget->isHidden()) {
            this->ColorManagerWidget->resetColorRange();
            this->ColorManagerWidget->show();
        } else {
            this->ColorManagerWidget->hide();
        }
    });

    connect(this->ColorManagerWidget, &igQtColorManagerWidget::UpdateColorBarFinished, this, [&]() {
        ui->widget_ScalarField->updateDrawStyle();
        this->rendererWidget->getColorBarWidget()->update();
    });
    // connect(ui->widget_EditMode, &igQtEditModeWidget::ChangeCutFlag,
    // rendererWidget, &igQtModelDrawWidget::UpdateCutFlag);
    // connect(ui->widget_EditMode,
    // &igQtEditModeWidget::UpdateCurrentModelCutPlane, rendererWidget,
    // &igQtModelDrawWidget::UpdateCutPlane); connect(ui->widget_EditMode,
    // &igQtEditModeWidget::ChangeEditModeToModelView, rendererWidget,
    // &igQtModelDrawWidget::UpdateEditModeToModelView);
    // connect(ui->widget_EditMode, &igQtEditModeWidget::ChangeEditModeToPickItem,
    // rendererWidget, &igQtModelDrawWidget::UpdateEditModeToPickItem);
    // connect(this->rendererWidget,
    // &igQtModelDrawWidget::updateSelectedFramePlane, this, [&]() {
    //	ui->widget_SearchInfo->searchDataWithFramePlane();
    //	});
    // connect(this->rendererWidget, &igQtModelDrawWidget::updatePickRay, this,
    // [&](Vector3f p, Vector3f dir) {
    // ui->widget_SearchInfo->searchDataWithRay(p,
    // dir);
    //	});
    // connect(ui->widget_SearchInfo, &igQtSearchInfoWidget::showSearchedPoint,
    // this, [&](iGameFloatArray* points) {
    //	this->rendererWidget->DrawSelectedPoint(points);
    //	});
    //
    connect(ui->widget_VectorField, &igQtVectorWidget::DrawDireVector, this, [&](iGame::DataObject::Pointer res) {
        modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
    });
    connect(ui->widget_VectorField, &igQtVectorWidget::UpdateDireVector, this, [&](iGame::DataObject::Pointer res) {
        res->Modified();
        modelTreeWidget->updateItemName(res);
        rendererWidget->update();
    });
    connect(ui->widget_TensorField, &igQtTensorWidget::DrawTensorGlyphs, this, [&](iGame::DataObject::Pointer res) {
        modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
    });
    connect(ui->widget_TensorField, &igQtTensorWidget::UpdateTensorGlyphs, this, [&](iGame::DataObject::Pointer res) {
        //res->Modified();
        rendererWidget->update();
    });
    connect(ui->widget_TensorField, &igQtTensorWidget::UpdateAttributes, this, [&](iGame::DataObject::Pointer res) {
        modelTreeWidget->updateAllAttriubute(res);
        // modelTreeWidget->addDataObjectToModelTree(res,
        // ItemSource::Algorithm);
    });

    connect(ui->widget_ContourExtract, &igQtContourExtractWidget::DrawContourModel, this,
            [&](iGame::DataObject::Pointer res) {
                modelTreeWidget->addDataObjectToModelTree(res, ItemSource::Algorithm);
            });
    connect(ui->widget_ContourExtract, &igQtContourExtractWidget::UpdateContourModel, this,
            [&](DataObject::Pointer mesh) {
                modelTreeWidget->updateCurrentModelInfo();
                rendererWidget->update();
            });
    // reset clipping
    connect(ui->action_ResetClipping, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto inputMesh = DynamicCast<iGame::DrawObject>(scene->GetCurrentModel()->GetDataObject());
        if (!inputMesh->GetClipped()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        inputMesh->GetClipper()->DisableAll();
        inputMesh->SetVisibility(true);

        const std::string OVName = "__" + inputMesh->GetName() + "_OV";   // 临时模型
        const std::string IVName = "__" + inputMesh->GetName() + "_IV";   // 临时模型
        const std::string OIVName = "__" + inputMesh->GetName() + "_OIV"; // 临时模型
        bool exist[3]{false, false, false};
        //for (auto& [id, model]: scene->GetModelList()) {
        //    if (model->GetDataObject()->GetName() == OVName) {
        //        auto model = scene->GetModelById(id);
        //        model->GetDataObject()->SetVisibility(false);
        //    } else if (model->GetDataObject()->GetName() == IVName) {
        //        auto model = scene->GetModelById(id);
        //        model->GetDataObject()->SetVisibility(false);
        //    } else if (model->GetDataObject()->GetName() == OIVName) {
        //        auto model = scene->GetModelById(id);
        //        model->GetDataObject()->SetVisibility(false);
        //    }
        //}
        auto modelList = scene->GetModelList();
        for (auto it = modelList->Begin(); it != modelList->End(); ++it) {
            auto id = it->first;
            auto model = it->second;
            
            auto drawObject = DynamicCast<DrawObject>(model->GetDataObject());

            if (drawObject->GetName() == OVName) {
                drawObject->SetVisibility(false);
            } else if (drawObject->GetName() == IVName) {
                drawObject->SetVisibility(false);
            } else if (drawObject->GetName() == OIVName) {
                drawObject->SetVisibility(false);
            }
        }

        rendererWidget->update();
    });


    // box clipping
    connect(ui->action_BoxClipping, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        bool ok;

        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
        auto inputMesh = DynamicCast<iGame::DrawObject>(scene->GetCurrentModel()->GetDataObject());
        if (!inputMesh->GetClipped()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        auto box = inputMesh->GetBoundingBox();
        auto center = (box.min + box.max) * 0.5;
        auto size = box.max - box.min;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
        int x_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_min(0..1)", "0.0");
        int y_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_min(0..1)", "0.0");
        int z_min_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_min(0..1)", "0.0");
        int x_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_max(0..1)", "0.5");
        int y_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_max(0..1)", "1.0");
        int z_max_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_max(0..1)", "1.0");
        int flip_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip", "false");
        dialog->show();

        dialog->setApplyFunctor([=]() {
            bool ok;
            auto Clamp = [](double x, double l, double r) -> double {
                if (x < l) return l;
                if (x > r) return r;
                return x;
            };

            auto clipper = inputMesh->GetClipper();

            auto& cbox = clipper->m_Box;
            cbox.m_Use = true;

            cbox.m_Bmin[0] = box.min[0] + size[0] * Clamp(dialog->getDouble(x_min_id, ok), 0., 1.);
            cbox.m_Bmin[1] = box.min[1] + size[1] * Clamp(dialog->getDouble(y_min_id, ok), 0., 1.);
            cbox.m_Bmin[2] = box.min[2] + size[2] * Clamp(dialog->getDouble(z_min_id, ok), 0., 1.);
            cbox.m_Bmax[0] = box.min[0] + size[0] * Clamp(dialog->getDouble(x_max_id, ok), 0., 1.);
            cbox.m_Bmax[1] = box.min[1] + size[1] * Clamp(dialog->getDouble(y_max_id, ok), 0., 1.);
            cbox.m_Bmax[2] = box.min[2] + size[2] * Clamp(dialog->getDouble(z_max_id, ok), 0., 1.);
            cbox.m_Flip = dialog->getChecked(flip_id, ok);

            clipper->Modified();

            rendererWidget->update();
        });
    });


    // plane clipping
    connect(ui->action_PlaneClipping, &QAction::triggered, this, [&](bool checked) {
        if (!rendererWidget->GetScene()->GetCurrentModel()) {
            std::cout << "Need Input" << std::endl;
            return;
        }
        bool ok;

        auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

        auto inputMesh = DynamicCast<iGame::DrawObject>(scene->GetCurrentModel()->GetDataObject());
        if (!inputMesh->GetClipped()) {
            std::cout << "This type of mesh can't be clipped" << std::endl;
            return;
        }

        auto box = inputMesh->GetBoundingBox();
        auto center = (box.min + box.max) * 0.5;
        auto size = box.max - box.min;

        igQtFilterDialogDockWidget* dialog = new igQtFilterDialogDockWidget(this);
        int origin_x_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_x(0..1)", "0.5");
        int origin_y_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_y(0..1)", "0.5");
        int origin_z_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "origin_z(0..1)", "0.5");
        int normal_x_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_x(-1..1)", "1.0");
        int normal_y_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_y(-1..1)", "0.0");
        int normal_z_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT, "normal_z(-1..1)", "0.0");
        int flip_id = dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip", "false");
        dialog->show();

        dialog->setApplyFunctor([=]() {
            bool ok;
            auto Clamp = [](double x, double l, double r) -> double {
                if (x < l) return l;
                if (x > r) return r;
                return x;
            };

            auto clipper = inputMesh->GetClipper();

            auto& cplane = clipper->m_Plane;
            cplane.m_Use = true;

            cplane.m_Origin[0] = box.min[0] + size[0] * Clamp(dialog->getDouble(origin_x_id, ok), 0., 1.);
            cplane.m_Origin[1] = box.min[1] + size[1] * Clamp(dialog->getDouble(origin_y_id, ok), 0., 1.);
            cplane.m_Origin[2] = box.min[2] + size[2] * Clamp(dialog->getDouble(origin_z_id, ok), 0., 1.);
            cplane.m_Normal[0] = Clamp(dialog->getDouble(normal_x_id, ok), -1., 1.);
            cplane.m_Normal[1] = Clamp(dialog->getDouble(normal_y_id, ok), -1., 1.);
            cplane.m_Normal[2] = Clamp(dialog->getDouble(normal_z_id, ok), -1., 1.);
            cplane.m_Flip = dialog->getChecked(flip_id, ok);
            if (cplane.m_Normal[0] == 0. && cplane.m_Normal[1] == 0. && cplane.m_Normal[2] == 0.) {
                std::cout << "Normal is a vector of zero" << std::endl;
                return;
            }

            clipper->Modified();

            rendererWidget->update();
        });
    });
}
void igQtMainWindow::updateRecentFilePaths() {
    ui->menu_RecentFiles->clear();
    auto recentFileActions = fileLoader->GetRecentActionList();
    for (auto i = recentFileActions.size() - 1; i >= 0; i--) {
        ui->menu_RecentFiles->addAction(recentFileActions.at(i));
    }
}
void igQtMainWindow::updateColorBarShow() {
    auto colorBar = this->rendererWidget->getColorBarWidget();
    if (!colorBar) { return; }
    colorBar->update();
    if (colorBar->isHidden()) {
        colorBar->show();
    } else {
        colorBar->hide();
    }
}

void igQtMainWindow::initAllSources() {
    // connect(ui->action_LineSource, &QAction::triggered, this, [&]() {
    //	UnstructuredMesh::Pointer newLinePointSet = UnstructuredMesh::New();
    //	newLinePointSet->SetViewStyle(IG_POINTS);
    //	newLinePointSet->AddPoint(Point(0.f, 0.f, 0.f));
    //	newLinePointSet->AddPoint(Point(1.f, 1.0f, 1.f));
    //	igIndex cell[1] = { 0 };
    //	newLinePointSet->AddCell(cell, 1, IG_VERTEX);
    //	cell[0] = 1;
    //	newLinePointSet->AddCell(cell, 1, IG_VERTEX);
    //	auto curScene = SceneManager::Instance()->GetCurrentScene();

    //	LineTypePointsSource::Pointer lineSource = LineTypePointsSource::New();

    //	lineSource->SetInput(newLinePointSet);
    //	lineSource->SetResolution(20);
    //	lineSource->GetOutput()->SetName("lineSource");

    //	auto model = curScene->CreateModel(lineSource->GetOutput());
    //	modelTreeWidget->addModelToModelTree(model);
    //	auto interactor = LineSourceInteractor::New();

    //	//        auto interactor = PointDragInteractor::New();
    //	interactor->SetPointSet(DynamicCast<PointSet>(SceneManager::Instance()
    //		->GetCurrentScene()
    //		->GetCurrentModel()
    //		->GetDataObject()));

    //	rendererWidget->ChangeInteractor(interactor);
    //	});
}

void igQtMainWindow::initAllInteractor() {
    connect(ui->action_select_point, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_select_point->isChecked()) {
            if (ui->action_select_face->isChecked()) { ui->action_select_face->setChecked(false); }
            rendererWidget->ChangeInteractorStyle(Interactor::SinglePointSelectionStyle);
        } else {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
    });
    //connect(ui->action_select_points, &QAction::triggered, this,
    //        [&](bool checked) {
    //            if (ui->action_select_points->isChecked()) {
    //                if (ui->action_select_point->isChecked()) {
    //                    ui->action_select_point->setChecked(false);
    //                }
    //                rendererWidget->ChangeInteractorStyle(
    //                        Interactor::MultiPointSelectionStyle);
    //            } else {
    //                rendererWidget->ChangeInteractorStyle(
    //                        Interactor::BasicStyle);
    //            }
    //        });

    connect(ui->action_select_face, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_select_face->isChecked()) {
            if (ui->action_select_point->isChecked()) { ui->action_select_point->setChecked(false); }
            rendererWidget->ChangeInteractorStyle(Interactor::SingleFaceSelectionStyle);
        } else {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
    });
    //connect(ui->action_select_faces, &QAction::triggered, this,
    //        [&](bool checked) {
    //            if (ui->action_select_faces->isChecked()) {
    //                if (ui->action_select_face->isChecked()) {
    //                    ui->action_select_face->setChecked(false);
    //                }
    //                rendererWidget->ChangeInteractorStyle(
    //                        Interactor::MultiFaceSelectionStyle);
    //            } else {
    //                rendererWidget->ChangeInteractorStyle(
    //                        Interactor::BasicStyle);
    //            }
    //        });
    connect(ui->action_drag_point, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_drag_point->isChecked()) {
            rendererWidget->ChangeInteractorStyle(Interactor::DragPointStyle);
        } else {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
    });

    //connect(ui->action_slicing, &QAction::triggered, this,
    //    [&](bool checked) {
    //        if (ui->action_slicing->isChecked()) {
    //            rendererWidget->ChangeInteractorStyle(
    //                    Interactor::SlicingStyle);
    //        } else {
    //            rendererWidget->ChangeInteractorStyle(
    //                    Interactor::BasicStyle);
    //        }
    //    });
}

void igQtMainWindow::UpdateRenderingWidget() { rendererWidget->update(); }
