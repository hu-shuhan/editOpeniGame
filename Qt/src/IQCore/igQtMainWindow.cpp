#include "IQCore/igQtMainWindow.h"
//
// Created by m_ky on 2024/4/10.
//

#include "SurfaceMeshFilters/Tests/iGameGradient.h"
#include "SurfaceMeshFilters/Tests/iGameSimplification2.h"
#include "SurfaceMeshFilters/Tests/iGameSurfaceSimplification.h"
#include "SurfaceMeshFilters/Tests/meshsimplifier/meshsimplifier.h"
#include "SurfaceMeshFilters/Tests/simplifier.h"

#include "Convert/iGameConvertPolyhedralCells.h"
#include "Convert/iGameConvertToPointCloud.h"
#include "Convert/iGameConvertToSurfaceMesh.h"
#include "Convert/iGameConvertToVolumeMesh.h"
#include "UndefinedFilters/iGameVortexDetection.h"

#include "Interactor/iGameInteractor.h"
#include "SurfaceMeshFilters/iGameMeshSimplifier.h"

#include "SurfaceMeshFilters/iGameSimplification.h"
#include "SurfaceMeshFilters/iGameTriangulation.h"
#include "Tests/iGameARAPTest.h"
#include "UndefinedFilters/iGameCurvatureFilter.h"
#include "UndefinedFilters/iGameGradientFilter.h"
#include "UndefinedFilters/iGameLaplacianFilter.h"
#include "UndefinedFilters/iGameVortexFilter.h"
#include "iGameAttribute.h"
#include "iGameFileIO.h"
#include "iGameFilterIncludes.h"
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQComponents/igQtProgressBarWidget.h>
#include <IQCore/igQtFileLoader.h>
#include <IQCore/igQtOpenGLWidgetManager.h>
#include <IQWidgets/ColorManager/igQtColorManagerWidget.h>
#include <IQWidgets/igQtAiChat/igQtAiChatWidget.h>
#include <IQWidgets/igQtAiChat/igQtCommandManager.h>
#include <IQWidgets/igQtCharts.h>
#include <IQWidgets/igQtDeformationWidget.h>
#include <IQWidgets/igQtModelClipWidget.h>
#include <IQWidgets/igQtModelDrawWidget.h>
#include <IQWidgets/igQtModelInformationWidget.h>
#include <IQWidgets/igQtParallelCoordinatesWidget.h>
#include <IQWidgets/igQtTensorWidget.h>
#include <IQWidgets/igQtVariableCorrelationWidget.h>
#include <Sources/iGameLineTypePointsSource.h>
#include <Tests/iGameVolumeMeshFilterTest.h>
#include <VolumeMeshAlgorithm/iGameVolumeMeshClipper.h>
#include <fcntl.h>
#include <iGameCtxPresObjData.h>
#include <iGameDataSource.h>
#include <iGamePointFinder.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>
#include <include/IQComponents/Dialog/igQtChangeBackGroundDialog.h>
#include <include/IQComponents/Dialog/igQtMeshCodecDialog.h>
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

    // 初始化命令管理器并建立与 MCP Tool Server 的连接
    commandManager = new igQtCommandManager(this);
    if (!commandManager->startConnection("localhost", 12345)) {
        qWarning() << "iGameVis 与 MCP Tool Server 连接失败！";
    }

    ThreadPool::Instance();
}
igQtMainWindow::~igQtMainWindow() {
    // 清理命令管理器
    if (commandManager) {
        commandManager->stopConnection();
        delete commandManager;
        commandManager = nullptr;
    }
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

    // 初始化AI聊天DockWidget
    aiChatDockWidget = new QDockWidget(this);
    aiChatDockWidget->setWindowTitle("AI聊天助手");
    aiChatWidget = new igQtAiChatWidget(aiChatDockWidget, this);
    aiChatDockWidget->setWidget(aiChatWidget);
    aiChatDockWidget->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    aiChatDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    aiChatDockWidget->hide(); // 初始隐藏
    this->addDockWidget(Qt::RightDockWidgetArea, aiChatDockWidget);

    // 设置DockWidget的默认大小
    aiChatDockWidget->resize(400, 600);
    ui->dockWidget_ScalarField->hide();
    ui->dockWidget_VectorField->hide();
    ui->dockWidget_FlowField->hide();
    ui->dockWidget_TensorField->hide();
    ui->dockWidget_ParallelCoordinatesField->hide();
    ui->dockWidget_VariableCorrelationField->hide();
    ui->dockWidget_VariableDensityField->hide();
    ui->dockWidget_DataChangeField->hide();
    ui->dockWidget_SelectionField->hide();
    ui->dockWidget_ContextPreservingShowField->hide();
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
void igQtMainWindow::initToolbarComponent() {}

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
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return false;
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        if (!obj) return false;

        igQtMeshCodecDialog* d = new igQtMeshCodecDialog(this, obj);
        d->exec();

        return true;
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


    connect(ui->action_ShowCenter, &QAction::toggled, this, [&](bool checked) {
        /*qDebug() << "Toggle state:" << checked;*/

        rendererWidget->GetScene()->ToggleCenterAxes();
        ui->action_ShowCenter->setChecked(checked);

        rendererWidget->update();
    });

    connect(ui->action_PickCenter, &QAction::triggered, this, [&](bool checked) {
        //qDebug() << "PickCenter Toggle state:" << checked;
        //if (ui->action_PickCenter->isChecked()) {
        //    rendererWidget->ChangeInteractorStyle(Interactor::PickCenterStyle);
        //} else {
        //    // 退出选择模式
        //    rendererWidget->setProperty("isPickingCenter", false);
        //    rendererWidget->setCursor(Qt::ArrowCursor);
        //    rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        //}
        /*rendererWidget->update();*/
        //拖拽
        if (checked) {
            // 显示坐标轴并进入拖拽模式
            rendererWidget->GetScene()->GetCenterAxesModel()->SetVisibility(true);
            rendererWidget->ChangeInteractorStyle(Interactor::DragCenterStyle);
            //rendererWidget->setCursor(Qt::CrossCursor);
        } else {
            // 退出选择模式
            rendererWidget->setCursor(Qt::ArrowCursor);
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
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

    // AI聊天助手
    connect(ui->action_AiChat, &QAction::triggered, this, [&](bool checked) {
        if (aiChatDockWidget->isVisible()) {
            aiChatDockWidget->hide();
        } else {
            aiChatDockWidget->show();
        }
    });

    initAllDockWidgetConnectWithAction();
    initAllMySignalConnections();
}

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
                    result += "\nMean Distance: " + QString::number(dd / oldMesh->GetBoundingBox().diag() * 100) + "%";
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

    if (false)
        connect(mesh_processing->addAction("Simplification with half-edge"), &QAction::triggered, this,
                [&](bool checked) {
                    auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

                    // ConvertToSurfaceMesh::Pointer converter = ConvertToSurfaceMesh::New();
                    // converter->SetInput(obj);
                    // converter->Execute();

                    auto mesh = DynamicCast<DrawObject>(obj)->GetRenderableObject();

                    // std::vector<int> PointDegree(mesh->GetNumberOfPoints(), 0);
                    // AttributeSet::Pointer AttrSet = AttributeSet::New();
                    //
                    // igIndex face[IGAME_CELL_MAX_SIZE]{};
                    // for (int i = 0; i < mesh->GetNumberOfFaces(); ++i) {
                    //     int size = mesh->GetFacePointIds(i, face);
                    //
                    //     PointDegree[face[0]]++;
                    //     PointDegree[face[1]]++;
                    //     PointDegree[face[2]]++;
                    // }
                    //
                    // for (int i = 0; mesh->GetAttributeSet() && i < mesh->GetAttributeSet()->GetNumberOfAttributes();
                    //      ++i) {
                    //     auto& attr = mesh->GetAttributeSet()->GetAttribute(i);
                    //     if (attr.attachmentType == IG_POINT) {
                    //         int dim = attr.pointer->GetDimension();
                    //
                    //
                    //         FloatArray::Pointer arr = FloatArray::New();
                    //         arr->SetName(attr.pointer->GetName());
                    //         arr->SetDimension(dim);
                    //         for (int k = 0; k < attr.pointer->GetNumberOfValues(); ++k) {
                    //             arr->AddValue(static_cast<float>(attr.pointer->GetValue(k)));
                    //         }
                    //         AttrSet->AddAttribute(IG_VECTOR, IG_POINT, arr);
                    //
                    //     } else if (attr.attachmentType == IG_CELL) {
                    //         int dim = attr.pointer->GetDimension();
                    //
                    //         FloatArray::Pointer arr = FloatArray::New();
                    //         arr->SetName(attr.pointer->GetName());
                    //         arr->SetDimension(dim);
                    //         arr->Resize(mesh->GetNumberOfPoints());
                    //
                    //         for (int j = 0; j < mesh->GetNumberOfFaces(); ++j) {
                    //             int size = mesh->GetFacePointIds(j, face);
                    //             float cell[IGAME_CELL_MAX_SIZE];
                    //             attr.pointer->GetElement(j, cell);
                    //             for (int k = 0; k < size; ++k) {
                    //                 for (int d = 0; d < dim; ++d) {
                    //                     arr->SetValue(face[k] * dim + d, arr->GetValue(face[k] * dim + d) +
                    //                                                              cell[d] / PointDegree[face[k]]);
                    //                 }
                    //             }
                    //         }
                    //
                    //         AttrSet->AddAttribute(IG_VECTOR, IG_POINT, arr);
                    //     }
                    // }
                    //
                    // SurfaceMesh::Pointer newMesh = SurfaceMesh::New();
                    // newMesh->SetPoints(mesh->GetPoints());
                    // newMesh->SetFaces(mesh->GetFaces());
                    // newMesh->SetAttributeSet(AttrSet);
                    // newMesh->SetName(mesh->GetName());

                    Triangulation::Pointer triangulation = Triangulation::New();
                    triangulation->SetInput(mesh);
                    triangulation->Execute();
                    mesh = DynamicCast<SurfaceMesh>(triangulation->GetOutput());

                    modelTreeWidget->addDataObjectToModelTree(mesh, Algorithm);
                    rendererWidget->update();

                    return;
                    //auto& attr = mesh->GetAttributeSet()->GetAttribute(mesh->GetAttributeIndex());
                    //FloatArray::Pointer att = FloatArray::New();
                    //att->SetName(attr.pointer->GetName());
                    //for (int i = 0; i < attr.pointer->GetNumberOfElements(); i++) {
                    //    float val[3]{};
                    //    attr.pointer->GetElement(i, val);
                    //    att->AddValue(std::min(1500.f, std::sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2])));
                    //}
                    //mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, att);
                    //modelTreeWidget->updateAllAttriubute(mesh);

                    // mesh->BuildFaceLinks();
                    // auto& attr = mesh->GetAttributeSet()->GetAttribute(3);
                    // FloatArray::Pointer att = FloatArray::New();
                    // att->SetName(attr.pointer->GetName());
                    // for (int i = 0; i < mesh->GetNumberOfPoints(); i++) {
                    //     igIndex cell[32]{};
                    //     int size = mesh->GetPointToNeighborFaces(i, cell);
                    //     float val = 0;
                    //     for (int j = 0; j < size; j++) { val += attr.pointer->GetValue(cell[j]); }
                    //     val /= size;
                    //     att->AddValue(val);
                    // }
                    // mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, att);
                    // modelTreeWidget->updateAllAttriubute(mesh);
                });


    connect(mesh_processing->addAction("Triangulation"), &QAction::triggered, this, [&](bool checked) {
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

        Triangulation::Pointer triangulation = Triangulation::New();
        triangulation->SetInput(obj);
        triangulation->Execute();
        auto mesh = DynamicCast<SurfaceMesh>(triangulation->GetOutput());

        modelTreeWidget->addDataObjectToModelTree(mesh, Algorithm);
        rendererWidget->update();
    });

    connect(mesh_processing->addAction("提取体网格表面网格"), &QAction::triggered, this, [&](bool checked) {
        auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();

        if (VolumeMesh::Pointer mesh = DynamicCast<VolumeMesh>(obj)) { 
            auto new_mesh = mesh->GetRenderableObject();
            new_mesh->SetName(mesh->GetName() + "_surface");
            modelTreeWidget->addDataObjectToModelTree(new_mesh, Algorithm);
            rendererWidget->update();
        } else if (UnstructuredMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(obj)) {
            auto new_mesh = mesh->GetRenderableObject();
            new_mesh->SetName(mesh->GetName() + "_surface");
            modelTreeWidget->addDataObjectToModelTree(new_mesh, Algorithm);
            rendererWidget->update();
        }
    });

    //if (false)
    //connect(mesh_processing->addAction("Simplification with half-edge"), &QAction::triggered, this, [&](bool checked) {
    //    auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
    //    int index = DynamicCast<DrawObject>(obj)->GetAttributeIndex();
    //    if (index == -1) index = 0;
    //    Triangulation::Pointer triangulation = Triangulation::New();
    //    triangulation->SetInput(obj);
    //    triangulation->Execute();
    //    obj = triangulation->GetOutput();
    //    auto mesh = DynamicCast<SurfaceMesh>(obj);

    //    unsigned int* destination;
    //    unsigned int* indices = reinterpret_cast<unsigned int*>(mesh->GetFaces()->GetCellIdArray()->RawPointer());
    //    const size_t index_count = mesh->GetNumberOfFaces() * 3;
    //    float* vertex_positions = mesh->GetPoints()->RawPointer();
    //    size_t vertex_count = mesh->GetNumberOfPoints();
    //    size_t vertex_positions_stride = sizeof(float) * 3;
    //    float* vertex_attributes = nullptr;

    //    vertex_attributes = DynamicCast<FloatArray>(mesh->GetAttributeSet()->GetAttribute(index).pointer)->RawPointer();
    //    size_t vertex_attributes_stride = sizeof(float);
    //    float attribute_weights[3]{1,1,1};
    //    size_t attribute_count = 3;
    //    unsigned char* vertex_lock = nullptr;
    //    size_t target_index_count = index_count * 0.1;
    //    float target_error = 0.01f;
    //    unsigned int options = 0;
    //    float out_error = 0.0f;


    //    float* result_error = &out_error;

    //    destination = new unsigned int[index_count];

    //    clock_t start, end;

    //    start = clock();
    //    size_t result_size = simplify_trimesh_with_attriubtes(
    //            destination, indices, index_count, vertex_positions, vertex_count, vertex_attributes, attribute_weights,
    //            attribute_count, target_index_count, target_error, result_error);
    //    end = clock();
    //    std::cout << "simplify_trimesh_with_attriubtes cost time: " << end - start << std::endl;
    //    //size_t result_size = tri::simplifyWithAttributes(destination, indices, index_count, vertex_positions, vertex_count,
    //    //                               vertex_positions_stride, vertex_attributes, vertex_attributes_stride,
    //    //                               attribute_weights, attribute_count, vertex_lock, target_index_count,
    //    //                               target_error, options, result_error);

    //    //size_t result_size = meshopt_simplifyWithAttributes<unsigned int>(
    //    //        destination, indices, index_count, vertex_positions, vertex_count, vertex_positions_stride,
    //    //        vertex_attributes, vertex_attributes_stride, attribute_weights, attribute_count, vertex_lock,
    //    //        target_index_count, target_error, options, result_error);


    //    //size_t result_size =
    //    //        meshopt_simplifySloppy(destination, indices, index_count, vertex_positions, vertex_count,
    //    //                               vertex_positions_stride, target_index_count, target_error, result_error);
    //    auto Mesh = SurfaceMesh::New();
    //    auto Faces = CellArray::New();
    //    auto Points = Points::New();
    //    auto Attrs = AttributeSet::New();
    //    auto oldAttrs = mesh->GetAttributeSet();
    //    for (int j = 0; j < oldAttrs->GetNumberOfAttributes(); j++) {
    //        auto& att = oldAttrs->GetAttribute(j);
    //        auto arr = FloatArray::New();
    //        arr->SetName(att.pointer->GetName());
    //        arr->SetDimension(att.pointer->GetDimension());
    //        Attrs->AddAttribute(att.type, att.attachmentType, arr);
    //    }

    //    std::cout << result_size << std::endl;
    //    std::vector<int> is_deleted(vertex_count, 1);
    //    for (int i = 0; i < target_index_count; i++) {
    //        is_deleted[destination[i]] = 0;
    //    }
    //    int count = 0;
    //    float val[32];
    //    std::vector<int> vertex_map(vertex_count, 0);
    //    for (int i = 0; i < is_deleted.size(); i++) {
    //        if (!is_deleted[i]) {
    //            vertex_map[i] = count;
    //            count++;
    //            Points->AddPoint(mesh->GetPoint(i));
    //            for (int j = 0; j < oldAttrs->GetNumberOfAttributes(); j++) {
    //                oldAttrs->GetAttribute(j).pointer->GetElement(i, val);
    //                Attrs->GetAttribute(j).pointer->AddElement(val);
    //            }
    //        }
    //    }
    //    for (int i = 0; i < target_index_count; i++) {
    //        destination[i] = vertex_map[destination[i]];
    //    }
    //    for (int i = 0; i < target_index_count / 3; i++) {
    //        Faces->AddCellId3(destination[i * 3], destination[i * 3 + 1], destination[i * 3 + 2]);
    //    }
    //    Mesh->SetName(mesh->GetName() + "_new");
    //    Mesh->SetPoints(Points);
    //    Mesh->SetFaces(Faces);
    //    Mesh->SetAttributeSet(Attrs);
    //    modelTreeWidget->addDataObjectToModelTree(Mesh, Algorithm);
    //    rendererWidget->update();
    //    return;
    //    {
    //        auto oldMesh = mesh;
    //        auto newMesh = Mesh;
    //        auto oldPoints = oldMesh->GetPoints();
    //        auto newPoints = newMesh->GetPoints();
    //        QString result = "";
    //        PointFinder::Pointer newPicker = PointFinder::New();
    //        newPicker->SetPoints(newPoints);
    //        newPicker->Initialize();

    //        double w1 = 0.0, w2 = 0.0;
    //        // 计算原始网格的表面积
    //        for (int i = 0; i < oldMesh->GetNumberOfFaces(); i++) {
    //            igIndex f[3]{};
    //            oldMesh->GetFacePointIds(i, f);
    //            Point v0 = oldMesh->GetPoint(f[0]);
    //            Point v1 = oldMesh->GetPoint(f[1]);
    //            Point v2 = oldMesh->GetPoint(f[2]);

    //            Vector3f d10 = v1 - v0;
    //            Vector3f d20 = v2 - v0;

    //            w1 += CrossProduct(d10, d20).norm() / 2.0;
    //        }
    //        //for (int i = 0; i < newMesh->GetNumberOfFaces(); i++) {
    //        //    igIndex f[3]{};
    //        //    newMesh->GetFacePointIds(i, f);
    //        //    Point v0 = newMesh->GetPoint(f[0]);
    //        //    Point v1 = newMesh->GetPoint(f[1]);
    //        //    Point v2 = newMesh->GetPoint(f[2]);

    //        //    Vector3f d10 = v1 - v0;
    //        //    Vector3f d20 = v2 - v0;

    //        //    w2 += CrossProduct(d10, d20).norm() / 2.0;
    //        //}

    //        double d1 = 0.0, d2 = 0.0;
    //        double d3 = 0.0, d4 = 0.0;

    //        iGame::ProgressObserver* ProgressBar = iGame::ProgressObserver::Instance();
    //        ProgressBar->UpdateProgress(0);
    //        int blockNum = oldPoints->GetNumberOfPoints() / 100, progress = 0;
    //        // 计算平均平方距离
    //        for (int i = 0; i < oldPoints->GetNumberOfPoints(); i++) {
    //            if (i > progress * blockNum) {
    //                ProgressBar->UpdateProgress(progress * 0.01);
    //                progress++;
    //            }
    //            auto p = oldPoints->GetPoint(i);

    //            igIndex id = newPicker->FindClosestPoint(p);
    //            if (id != -1) {
    //                Point cp = newPoints->GetPoint(id);
    //                d1 += (p - cp).squaredNorm();
    //                d3 += (p - cp).norm();
    //            }
    //        }

    //        //for (int i = 0; i < newPoints->GetNumberOfPoints(); i++) {
    //        //    auto p = newPoints->GetPoint(i);

    //        //    igIndex id = oldPicker->FindClosestPoint(p);
    //        //    if (id != -1) {
    //        //        Point cp = oldPoints->GetPoint(id);
    //        //        d2 += (p - cp).squaredNorm();
    //        //        d4 += (p - cp).norm();
    //        //    }
    //        //}

    //        //double d = 1.0 / w1 * d1 /*+ 1.0 / w2 * d2*/;
    //        double dd = 1.0 / oldPoints->GetNumberOfPoints() * d3 /*+ 1.0 / newPoints->GetNumberOfPoints() * d4*/;

    //        result += "\n几何相似性度量";
    //        //result += "\n Squared Mean Distance: " + QString::number(d);
    //        result += "\n Mean Distance: " + QString::number(dd);
    //        //result += "\nSquared Mean Distance: " + QString::number(d * 100) + "%";
    //        result += "\nMean Distance: " + QString::number(dd / oldMesh->GetBoundingBox().diag() * 100) + "%";
    //        //result += "\n\n累计几何误差: " + QString::number(filter->GetError());

    //        QMessageBox::information(this, "简化成功", result);
    //    }
    //});
    //
    if (true)
        connect(mesh_processing->addAction("New Simplification"), &QAction::triggered, this, [&](bool checked) {
            auto obj = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
            //auto attrSet = AttributeSet::New();
            //attrSet->AddAttribute(IG_POINT, IG_SCALAR, obj->GetAttributeSet()->GetAttribute("U").pointer);
            //obj->SetAttributeSet(attrSet);
            //modelTreeWidget->addDataObjectToModelTree(obj, Algorithm);
            //rendererWidget->update();
            //return;

            //Triangulation::Pointer triangulation = Triangulation::New();
            //triangulation->SetInput(obj);
            //triangulation->Execute();
            //obj = triangulation->GetOutput();


            //obj = DynamicCast<UnstructuredMesh>(obj)->GetDisplayObject();
            //obj->SetAttributeSet(AttributeSet::New());
            MeshSimplifier::Pointer Sim = MeshSimplifier::New();
            Sim->SetInput(obj);
            if (Sim->Execute()) {
                auto new_mesh = Sim->GetOutput(0);

                modelTreeWidget->addDataObjectToModelTree(new_mesh, Algorithm);
                rendererWidget->update();
            }
        });


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

        //data = DynamicCast<DrawObject>(data)->GetDisplayObject();

        filter->SetInput(data);
        if (filter->Execute()) {
            //modelTreeWidget->addDataObjectToModelTree(data, Algorithm);

            modelTreeWidget->updateAllAttriubute(data);
            DynamicCast<DrawObject>(data)->ConvertToDrawableData();
        }
    });

    QAction* vortexPrection = view->addAction("PredictVortex");
    connect(vortexPrection, &QAction::triggered, this, [&](bool checked) {
        if (rendererWidget->GetScene()->GetCurrentModel() == nullptr) return;
        VortexDetection::Pointer filter = VortexDetection::New();
        auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
        filter->SetInput(data);
        if (filter->Execute()) {
            //modelTreeWidget->addDataObjectToModelTree(data, Algorithm);

            modelTreeWidget->updateAllAttriubute(data);
            DynamicCast<DrawObject>(data)->ConvertToDrawableData();
        }
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
        ui->widget_TensorField->InitTensorWidget();
    });
    connect(ui->action_ParallelCoordinates, &QAction::triggered, this, [&](bool checked) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        ui->dockWidget_ParallelCoordinatesField->show();
        ui->widget_ParallelCoordinatesField->SetParallelCoordinates(model);
    });

    connect(ui->widget_ParallelCoordinatesField, &igQtParallelCoordinatesWidget::SIGNAL_RefreshDataClicked, this,
            [&]() {
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) return;
                ui->widget_ParallelCoordinatesField->SetParallelCoordinates(model);
            });
    connect(ui->action_VariableCorrelation, &QAction::triggered, this, [&](bool checked) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        ui->dockWidget_VariableCorrelationField->show();
        ui->widget_VariableCorrelationField->SetModel(model);
    });

    connect(ui->widget_VariableCorrelationField, &igQtVariableCorrelationWidget::SIGNAL_RefreshDataClicked, this,
            [&]() {
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) return;
                ui->widget_VariableCorrelationField->SetModel(model);
            });
    connect(ui->action_VariableDensity, &QAction::triggered, this, [&](bool checked) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        ui->dockWidget_VariableDensityField->show();
        ui->widget_VariableDensityField->SetModel(model);
    });

    connect(ui->widget_VariableDensityField, &igQtVariableDensityWidget::SIGNAL_RefreshDataClicked, this, [&]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        ui->widget_VariableDensityField->SetModel(model);
    });
    auto DataChangeFunc = [&](igQtMainWindow* mainWindow) {
        auto model = mainWindow->rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        mainWindow->ui->dockWidget_DataChangeField->show();
        mainWindow->ui->widget_DataChangeField->InitRadialStyle(
                mainWindow->rendererWidget->GetScene()->GetInteractor());
        auto name = mainWindow->rendererWidget->GetScene()->GetInteractor()->SetSpecialInteractor(
                mainWindow->ui->widget_DataChangeField->GetRadialStyle());
        mainWindow->ui->widget_DataChangeField->SetInteractorName(name);
        mainWindow->ui->widget_DataChangeField->SetModel(model);
    };
    connect(ui->action_DataChange, &QAction::triggered, this, [&](bool checked) { DataChangeFunc(this); });
    connect(ui->widget_DataChangeField, &igQtDataChangeWidget::SIGNAL_RefreshDataClicked, this,
            [&]() { DataChangeFunc(this); });

    ui->action_ContextPreserving->setVisible(false);
    connect(ui->action_ContextPreserving, &QAction::triggered, this, [&](bool checked) {
        if (checked && !ui->dockWidget_ContextPreservingShowField->isVisible()) {
            auto model = rendererWidget->GetScene()->GetCurrentModel();
            if (model == nullptr) return;
            ui->dockWidget_ContextPreservingShowField->show();
            ui->widget_ContextPreservingShowField->SetContextPreserving(model);
        } else if (!checked && ui->dockWidget_ContextPreservingShowField->isVisible())
            ui->dockWidget_ContextPreservingShowField->hide();
    });
    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [&]() {
        if (ui->dockWidget_ContextPreservingShowField->isHidden()) return;
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) {
            ui->dockWidget_ContextPreservingShowField->hide();
            return;
        }
        ui->widget_ContextPreservingShowField->SetContextPreserving(model);
    });
    connect(ui->widget_ContextPreservingShowField, &igQtContextPreservingShowWidget::DrawUpdated, this,
            [&]() { rendererWidget->update(); });
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
        auto dataIndex = dataObject->GetAttributeIndex();
        auto attrDimension = dataObject->GetAttributeDimension();
        if (dataIndex < 0) { return; }
        auto array = attributeSet->GetAttribute(dataIndex).pointer;
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


    connect(fileLoader, &igQtFileLoader::AddFileToModelList, ui->modelTreeView, &igQtModelListView::AddModel);


    connect(ui->widget_Animation, &igQtAnimationWidget::UpdateScene, this, &igQtMainWindow::UpdateRenderingWidget);


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
    connect(ui->widget_TensorField, &igQtTensorWidget::UpdateTensorGlyphs, this,
            [&](iGame::DataObject::Pointer res) { rendererWidget->update(); });
    connect(ui->widget_TensorField, &igQtTensorWidget::UpdateAttributes, this,
            [&](iGame::DataObject::Pointer res) { modelTreeWidget->updateAllAttriubute(res); });

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
    connect(ui->action_SelectView, &QAction::triggered, this, [&](bool checked) {
        if (checked && !ui->dockWidget_SelectionField->isVisible()) ui->dockWidget_SelectionField->show();
        else if (!checked && ui->dockWidget_SelectionField->isVisible())
            ui->dockWidget_SelectionField->hide();
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::Signal_SetSelectionStationChanged, this, [&]() {
        auto selectionStation = ui->widget_SelectionField->GetSelectionStation();
        switch (selectionStation) {
            case SelectionStation::NONE_SELECTION:
                ui->widget_SelectionField->SetVariableNames({});
                break;
            case SelectionStation::POINT_SELECTION: {
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) {
                    ui->widget_SelectionField->SetVariableNames({});
                } else {
                    auto attrs = model->GetDataObject()->GetAttributeSet()->GetAllAttributes();
                    auto variableNames = CtxPresObjData_Main::GenerateVariableNames(attrs, IG_POINT);
                    ui->widget_SelectionField->SetVariableNames(variableNames);
                }
            } break;
            case SelectionStation::CELL_SELECTION: {
                auto model = rendererWidget->GetScene()->GetCurrentModel();
                if (model == nullptr) {
                    ui->widget_SelectionField->SetVariableNames({});
                } else {
                    auto attrs = model->GetDataObject()->GetAttributeSet()->GetAllAttributes();
                    auto variableNames = CtxPresObjData_Main::GenerateVariableNames(attrs, IG_CELL);
                    ui->widget_SelectionField->SetVariableNames(variableNames);
                }
            } break;
            default:
                break;
        }
        switch (selectionStation) {
            case SelectionStation::NONE_SELECTION:
                rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
                break;
            case SelectionStation::POINT_SELECTION: {
                rendererWidget->ChangeInteractorStyle(Interactor::SinglePointSelectionStyle);
            } break;
            case SelectionStation::CELL_SELECTION: {
                rendererWidget->ChangeInteractorStyle(Interactor::SingleFaceSelectionStyle);
            } break;
            default:
                break;
        }
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::SetSelectionShow, this, [&](bool visiable) {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        auto& selectedItems = model->GetSelection()->GetSelectedItems();
        if (visiable) {
            for (auto& objsInType: selectedItems) {
                for (auto& eventsInObj: objsInType.second) {
                    for (auto& drawHandle: eventsInObj.second.drawHandles) { model->GetPainter3D()->Show(drawHandle); }
                }
            }
        } else {
            for (auto& objsInType: selectedItems) {
                for (auto& eventsInObj: objsInType.second) {
                    for (auto& drawHandle: eventsInObj.second.drawHandles) { model->GetPainter3D()->Hide(drawHandle); }
                }
            }
        }
        //if (visiable) model->GetPainter3D()->ShowAll();
        //else
        //    model->GetPainter3D()->HideAll();
        rendererWidget->update();
    });
    connect(ui->widget_SelectionField, &igQtSelectionWidget::SetClearSelection, this, [&]() {
        auto model = rendererWidget->GetScene()->GetCurrentModel();
        if (model == nullptr) return;
        model->GetSelection()->Reset();
        rendererWidget->update();
    });

    connect(ui->widget_SelectionField, &igQtSelectionWidget::Hided, this,
            [&]() { ui->action_SelectView->setChecked(false); });
    connect(modelTreeWidget, &igQtModelDialogWidget::CurrendModelChanged, this, [&]() {
        ui->widget_SelectionField->PreventSignalSend(true);
        ui->widget_SelectionField->SetDefaultSelectionButton();
        ui->widget_SelectionField->PreventSignalSend(false);
        return;
        //auto radius = ui->widget_SelectionField->GetSelectionRadius();
        //auto selectionStation = ui->widget_SelectionField->GetSelectionStation();
        //auto selectOrUnSelect = ui->widget_SelectionField->GetSelectOrUnSelect();
        //switch (selectionStation) {
        //    case SelectionStation::NONE_SELECTION:
        //        rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        //        break;
        //    case SelectionStation::POINT_SELECTION:
        //        rendererWidget->ChangeInteractorStyle(Interactor::SinglePointSelectionStyle, radius, selectOrUnSelect);
        //        break;
        //    case SelectionStation::CELL_SELECTION:
        //        rendererWidget->ChangeInteractorStyle(Interactor::SingleFaceSelectionStyle, radius, selectOrUnSelect);
        //        break;
        //    default:
        //        break;
        //}
        //auto visiable = ui->widget_SelectionField->GetSelectionShow();
        //auto model = rendererWidget->GetScene()->GetCurrentModel();
        //if (model == nullptr) return;
        //if (visiable) model->GetPainter3D()->ShowAll();
        //else
        //    model->GetPainter3D()->HideAll();
    });
    connect(ui->widget_ContextPreservingShowField, &igQtContextPreservingShowWidget::Hided, this,
            [&]() { ui->action_ContextPreserving->setChecked(false); });


    ui->action_select_point->setVisible(false);
    ui->action_select_face->setVisible(false);
    connect(ui->action_select_point, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_select_point->isChecked()) {
            if (ui->action_select_face->isChecked()) { ui->action_select_face->setChecked(false); }
            rendererWidget->ChangeInteractorStyle(Interactor::SinglePointSelectionStyle);
        } else {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
    });


    connect(ui->action_select_face, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_select_face->isChecked()) {
            if (ui->action_select_point->isChecked()) { ui->action_select_point->setChecked(false); }
            rendererWidget->ChangeInteractorStyle(Interactor::SingleFaceSelectionStyle);
        } else {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
    });
    connect(ui->action_drag_point, &QAction::triggered, this, [&](bool checked) {
        if (ui->action_drag_point->isChecked()) {
            rendererWidget->ChangeInteractorStyle(Interactor::DragPointStyle);
        } else {
            rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
        }
    });


}

void igQtMainWindow::UpdateRenderingWidget() { rendererWidget->update(); }
