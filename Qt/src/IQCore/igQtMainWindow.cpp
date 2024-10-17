#include "IQCore/igQtMainWindow.h"
//
// Created by m_ky on 2024/4/10.
//
#include "UndefinedFilters/iGameCurvatureFilter.h"
#include "UndefinedFilters/iGameGradientFilter.h"
#include "UndefinedFilters/iGameLaplacianFilter.h"
#include "UndefinedFilters/iGameVortexFilter.h"
#include "SurfaceMeshFilters/iGameSurfaceSimplification.h"

#include "Interactor/iGameInteractor.h"
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
#include <include/IQComponents/Dialog/igQtScreenShotOptionDialog.h>
#include <stdio.h>
#include <IQWidgets/igQtModelClipWidget.h>
#include <IQWidgets/igQtDeformationWidget.h>

#include <QMessageBox>
igQtMainWindow::igQtMainWindow(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);
	modelTreeWidget = new igQtModelDialogWidget(this);
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
	// Setup default GUI layout.
	this->setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);
	this->setTabPosition(Qt::RightDockWidgetArea, QTabWidget::North);
	this->setTabPosition(Qt::BottomDockWidgetArea, QTabWidget::North);
	// Set up the dock window corners to give the vertical docks more room.
	this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
	this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	modelTreeWidget->setFloating(false); // Make sure it's docked
	modelTreeWidget->setAllowedAreas(Qt::LeftDockWidgetArea |
		Qt::TopDockWidgetArea);
	modelTreeWidget->setFeatures(
		QDockWidget::NoDockWidgetFeatures); // Disable floating and moving
	this->addDockWidget(Qt::LeftDockWidgetArea, modelTreeWidget);
	initToolbarComponent();
	initAllComponents();
	initAllFilters();
	initAllSources();
	initAllInteractor();
	updateRecentFilePaths();

	connect(modelTreeWidget, &igQtModelDialogWidget::Update, rendererWidget,
            &igQtRenderWidget::update);
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

	// init ProgressBar
	progressBarWidget = new igQtProgressBarWidget(this);
	this->statusBar()->addPermanentWidget(progressBarWidget);

	// connect(ui->action_SaveScreenShot, &QAction::triggered, rendererWidget,
	// &igQtModelDrawWidget::SaveScreenShoot);
	connect(ui->action_LoadFile, &QAction::triggered, fileLoader,
		&igQtFileLoader::LoadFile);
	// connect(ui->action_SaveMesh, &QAction::triggered, fileLoader,
	// &igQtFileLoader::SaveFile);
	connect(ui->action_SaveMeshAs, &QAction::triggered, fileLoader,
		&igQtFileLoader::SaveFileAs);
	// connect(ui->action_CopyMesh, &QAction::triggered, this, [&]() {
	//	iGame::iGameManager::Instance()->CopyMesh();
	//	});
	// connect(ui->action_RecoverMesh, &QAction::triggered, this, [&]() {
	//	iGame::iGameManager::Instance()->RecoverMesh();
	//	});
	connect(ui->action_UseOrthographic, &QAction::triggered, this,
		[&](bool checked) {
			if (ui->action_UseOrthographic->isChecked()) {
				SceneManager::Instance()
					->GetCurrentScene()
					->ChangeCameraType(
						Camera::CameraType::ORTHOGRAPHIC);
			}
			else {
				SceneManager::Instance()
					->GetCurrentScene()
					->ChangeCameraType(Camera::CameraType::PERSPECTIVE);
			}
			rendererWidget->update();
		});
	connect(ui->action_ResetCenter, &QAction::triggered, this, [&]() {
		SceneManager::Instance()->GetCurrentScene()->ResetCenter();
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

	connect(ui->action_setViewToPositiveX, &QAction::triggered, this,
		[&](bool checked) {
			rendererWidget->GetScene()->lookAtPositiveX();
			rendererWidget->update();
		});
	connect(ui->action_setViewToNegativeX, &QAction::triggered, this,
		[&](bool checked) {
			rendererWidget->GetScene()->lookAtNegativeX();
			rendererWidget->update();
		});
	connect(ui->action_setViewToPositiveY, &QAction::triggered, this,
		[&](bool checked) {
			rendererWidget->GetScene()->lookAtPositiveY();
			rendererWidget->update();
		});
	connect(ui->action_setViewToNegativeY, &QAction::triggered, this,
		[&](bool checked) {
			rendererWidget->GetScene()->lookAtNegativeY();
			rendererWidget->update();
		});
	connect(ui->action_setViewToPositiveZ, &QAction::triggered, this,
		[&](bool checked) {
			rendererWidget->GetScene()->lookAtPositiveZ();
			rendererWidget->update();
		});
	connect(ui->action_setViewToNegativeZ, &QAction::triggered, this,
		[&](bool checked) {
			rendererWidget->GetScene()->lookAtNegativeZ();
			rendererWidget->update();
		});
	connect(ui->action_setViewToIsometric, &QAction::triggered, this,
		[&](bool checked) {
			rendererWidget->GetScene()->lookAtIsometric();
			rendererWidget->update();
		});
	connect(ui->action_rotateNinetyClockwise, &QAction::triggered, this,
		[&](bool checked) {
			rendererWidget->GetScene()->rotateNinetyClockwise();
			rendererWidget->update();
		});
	connect(ui->action_rotateNinetyCounterClockwise, &QAction::triggered, this,
		[&](bool checked) {
			rendererWidget->GetScene()->rotateNinetyCounterClockwise();
			rendererWidget->update();
		});
	connect(ui->action_SaveScreenShot, &QAction::triggered, this, [&]() {
		QString path = QFileDialog::getSaveFileName(
			nullptr, "Save Screen shot", "",
			"PNG Images(*.png);;BMP Images(*.bmp)");
		igQtScreenShotOptionDialog dialog(this);
		dialog.setWindowTitle("Save ScreenShot Option.");
		int oldwidth = rendererWidget->width(),
			oldheight = rendererWidget->height();
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
		}
		else {
			QMessageBox::information(this, "", "保存失败");
		}
		});

	connect(ui->action_SaveAnimation, &QAction::triggered, this,
		[&]() { ui->widget_Animation->saveAnimation(); });


	initAllDockWidgetConnectWithAction();
	initAllMySignalConnections();
}
igQtMainWindow::~igQtMainWindow() {}

void igQtMainWindow::initAllFilters() {
	connect(ui->action_test_01, &QAction::triggered, this, [&](bool checked) {
		SurfaceSimplification::Pointer filter = SurfaceSimplification::New();

		filter->SetInput(
			rendererWidget->GetScene()->GetCurrentModel()->GetDataObject());

		//auto data =
		//        rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
		//iGameModelGeometryFilter::Pointer ext =
		//iGameModelGeometryFilter::New();
		//ext->SetInput(data);
		//ext->Execute();

		//auto mesh = ext->GetExtractMesh();
		//filter->SetInput(mesh);

		filter->Execute();
		modelTreeWidget->addDataObjectToModelTree(filter->GetOutput(), Algorithm);
		rendererWidget->update();
		});

	connect(ui->action_test_02, &QAction::triggered, this, [&](bool checked) {
		FilterPoints::Pointer fp = FilterPoints::New();
		fp->SetInput(
			rendererWidget->GetScene()->GetCurrentModel()->GetDataObject());
		fp->SetFilterRate(0.5);
		fp->Execute();
		rendererWidget->update();
		});


	connect(ui->action_test_03, &QAction::triggered, this, [&](bool checked) {
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
		SurfaceMeshFilterTest::Pointer fp = SurfaceMeshFilterTest::New();
		fp->SetInput(
			rendererWidget->GetScene()->GetCurrentModel()->GetDataObject());
		fp->Execute();
		rendererWidget->update();
		});

	connect(ui->action_test_05, &QAction::triggered, this, [&](bool checked) {
		// VolumeMeshFilterTest::Pointer fp = VolumeMeshFilterTest::New();
		// fp->SetInput(rendererWidget->GetScene()->GetCurrentModel()->GetDataObject());
		// fp->Execute();
		// rendererWidget->update();

		igQtFilterDialogDockWidget* dialog =
			new igQtFilterDialogDockWidget(this);
		int targetId =
			dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"Target number of faces", "1000");
		int reductionId =
			dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"Reduction (0..1)", "0");
		int thresholdId =
			dialog->addParameter(igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"Quality threshold", "0.1");
		int preserveId =
			dialog->addParameter(igQtFilterDialogDockWidget::QT_CHECK_BOX,
				"Preserve Boundary of the mesh", "false");
		dialog->show();

		bool ok;
		std::cout << dialog->getInt(targetId, ok) << std::endl;
		std::cout << dialog->getDouble(reductionId, ok) << std::endl;
		std::cout << dialog->getDouble(thresholdId, ok) << std::endl;
		std::cout << (dialog->getChecked(preserveId, ok) ? "true" : "false")
			<< std::endl;

		dialog->setApplyFunctor([&]() { std::cout << "123\n"; });
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

	connect(ui->menuTest->addAction("surfaceExtractTest"), &QAction::triggered,
		this, [&](bool checked) {
			auto fp = iGameModelGeometryFilter::New();
			auto input = rendererWidget->GetScene()
				->GetCurrentModel()
				->GetDataObject();
			/*	fp->SetCellIndexExtent(100, 100000);*/
			// fp->SetPointIndexExtent(0, 100);
			auto bound = input->GetBoundingBox();
			auto a = (bound.max + bound.min * 4) / 5;
			auto b = (bound.max * 4 + bound.min) / 5;
			double extent[6] = { a[0], b[0], a[1], b[1], a[2], b[2] };

			for (int i = 0; i < 3; i++) {
				std::cout << a[i] << ' ' << b[i] << '\n';
			}
			fp->SetExtent(extent);

			fp->Execute(input);
			SceneManager::Instance()
				->GetCurrentScene()
				->ChangeModelVisibility(0, false);
			auto mesh = fp->GetExtractMesh();
			mesh->SetName("SURFACE");
			modelTreeWidget->addDataObjectToModelTree(mesh,
				ItemSource::File);
		});

	auto action_subdivision = ui->menuTest->addAction("Subdivision");
	connect(action_subdivision, &QAction::triggered, this, [&](bool checked) {
		/*	auto filter = HexhedronSubdivision::New();
						VolumeMesh::Pointer mesh =
		   DynamicCast<VolumeMesh>(rendererWidget->GetScene()->GetCurrentModel()->GetDataObject());*/
		   // auto filter = QuadSubdivision::New();
		   // SurfaceMesh::Pointer mesh = DynamicCast<SurfaceMesh>(
		   //     rendererWidget->GetScene()->GetCurrentModel()->GetDataObject());
		   // filter->SetMesh(mesh);
		   // filter->Execute();
		   // auto ControlPoints = filter->GetOutput();
		   // ControlPoints->SetName("ControlPoints");

		   // modelTreeWidget->addDataObjectToModelTree(ControlPoints,
		   // ItemSource::File);
		auto obj =
			rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
		std::cout << obj->GetName() << "KB\n";
		// MyTestFilter::Pointer aaa = MyTestFilter::New();
		// aaa->SetMesh(obj);
		// aaa->Execute();
		});

	auto action_tensorview = ui->menu_help->addAction("tensorview");
	connect(action_tensorview, &QAction::triggered, this, [&](bool checked) {
		auto chart = new igQtCharts;
		auto dataarray = rendererWidget->GetScene()
			->GetCurrentModel()
			->GetDataObject()
			->GetAttributeSet()
			->GetAttribute(0)
			.pointer;
		//auto dataarray=DoubleArray::New();
		//dataarray->AddValue(1.0);
		//dataarray->AddValue(2.0);
		//dataarray->AddValue(13.0);
		//dataarray->AddValue(4.0);
		//dataarray->AddValue(6.0);
		chart->draw(dataarray);
		chart->exec();
		});
	auto action_loadtest = ui->menu_help->addAction("loadtest");
	connect(action_loadtest, &QAction::triggered, this, [&](bool checked) {
		// std::string filePath = "F:\\OpeniGame\\Model\\vtk\\Spherical001.vtk";
		std::string filePath =
			"F:\\OpeniGame\\Model\\secrecy\\DrivAer_fastback_"
			"base_0.4_remesh_coarse_kw_CPU_test_P_V.cgns";
		// clock_t time_1 = clock();
		// auto writer = VTKWriter::New();
		// auto mesh =
		// SceneManager::Instance()->GetCurrentScene()->GetCurrentModel()->GetDataObject();
		// writer->WriteToFile(mesh, filePath);
		// clock_t time_2 = clock();
		// std::cout << time_2 - time_1 << "ms\n";
		// return;

		CharArray::Pointer m_Buffer = CharArray::New();
		size_t m_FileSize;
		clock_t time1 = clock();

		// auto file_ = fopen(filePath.c_str(), "rb");
		// if (fseek(file_, SEEK_SET, SEEK_END) != 0) {
		//	return false;
		// }
		// m_FileSize = static_cast<size_t>(_ftelli64(file_));
		// rewind(file_);
		// if (m_FileSize == 0) {
		//	return false;
		// }
		// m_Buffer->Resize(m_FileSize);
		// fread(m_Buffer->RawPointer(), 1, m_FileSize, file_) == m_FileSize;
		// fclose(file_);

		// auto m_File = std::make_unique<std::ifstream>(filePath,
		// std::ios::binary); m_File->seekg(0, std::ios::end); m_FileSize =
		// m_File->tellg(); m_File->seekg(0, std::ios::beg); if (m_FileSize == 0) {
		//	return false;
		// }
		// m_Buffer->Resize(m_FileSize);
		// m_File->read(m_Buffer->RawPointer(), m_FileSize);

		clock_t time2 = clock();
		std::cout << "Read file to buffer Cost " << time2 - time1 << "ms\n";
		});

	auto action_savetest_fwrite = ui->menu_help->addAction("savetest_fwrite");
	connect(action_savetest_fwrite, &QAction::triggered, this,
		[&](bool checked) {
			std::string filePath = "F:\\OpeniGame\\Model\\common\\test.txt";
			// 创建多个长度为一亿的 char 数组
			const int num_arrays = 3;
			const size_t array_size = 200000000;
			char* data[num_arrays];
			for (int i = 0; i < num_arrays; ++i) {
				data[i] = (char*)malloc(array_size * sizeof(char));
				if (data[i] == NULL) {
					_tprintf(_T("Memory allocation failed.\n"));
					return;
				}

				// 初始化数组
				for (size_t j = 0; j < array_size; ++j) {
					data[i][j] = 'A' + (j % 26);
				}
			}
			clock_t time1 = clock();
			// 打开文件用于写入
			FILE* file = fopen(filePath.data(), "wb");
			if (file == NULL) {
				_tprintf(_T("File opening failed.\n"));
				return;
			}

			// 将每个数组依次写入文件
			for (int i = 0; i < num_arrays; ++i) {
				size_t elements_written =
					fwrite(data[i], sizeof(char), array_size, file);
				if (elements_written != array_size) {
					_tprintf(_T("Error writing to file.\n"));
					fclose(file);
					return;
				}
			}

			// 关闭文件
			fclose(file);

			// 释放内存
			for (int i = 0; i < num_arrays; ++i) { free(data[i]); }
			clock_t time2 = clock();
			std::cout << "Read file to buffer Cost " << time2 - time1
				<< "ms\n";
		});

	connect(ui->menuTest->addAction("addTetra"), &QAction::triggered, this,
		[&](bool checked) {
			VolumeMesh::Pointer mesh = VolumeMesh::New();
			Points::Pointer points = Points::New();
			CellArray::Pointer cells = CellArray::New();

			points->AddPoint(-1, -1, 0);
			points->AddPoint(1, -1, 0);
			points->AddPoint(1, 1, 0);
			points->AddPoint(-1, 1, 0);

			points->AddPoint(-1, -1, 2);
			points->AddPoint(1, -1, 2);
			points->AddPoint(1, 1, 2);
			points->AddPoint(-1, 1, 2);

			cells->AddCellId4(0, 1, 2, 4);
			cells->AddCellId4(2, 3, 0, 4);

			mesh->SetPoints(points);
			mesh->SetVolumes(cells);
			mesh->SetName("undefined_unstructured_mesh");
			mesh->RequestEditStatus();
			mesh->PrintSelf();
			modelTreeWidget->addDataObjectToModelTree(mesh,
				ItemSource::File);
		});

	connect(ui->menuTest->addAction("addVolume"), &QAction::triggered, this,
		[&](bool checked) {
			VolumeMesh::Pointer mesh = VolumeMesh::New();
			Points::Pointer points = Points::New();
			CellArray::Pointer cells = CellArray::New();

			points->AddPoint(-1, -1, 0);
			points->AddPoint(1, -1, 0);
			points->AddPoint(1, 1, 0);
			points->AddPoint(-1, 1, 0);

			points->AddPoint(-1, -1, 2);
			points->AddPoint(1, -1, 2);
			points->AddPoint(1, 1, 2);
			points->AddPoint(-1, 1, 2);

			cells->AddCellId4(0, 1, 2, 4);
			mesh->SetPoints(points);
			mesh->SetVolumes(cells);
			mesh->SetName("undefined_unstructured_mesh");

			mesh->RequestEditStatus();
			igIndex v[4]{ 2, 3, 0, 4 };
			mesh->AddVolume(v, 4);
			mesh->GarbageCollection();

			mesh->RequestEditStatus();
			mesh->PrintSelf();
			modelTreeWidget->addDataObjectToModelTree(mesh,
				ItemSource::File);
		});


	connect(ui->menuTest->addAction("pfTest"), &QAction::triggered, this,
		[&](bool checked) {
			auto obj = rendererWidget->GetScene()
				->GetCurrentModel()
				->GetDataObject();
			VolumeMesh::Pointer mesh = DynamicCast<UnstructuredMesh>(obj)
				->TransferToVolumeMesh();
			PointFinder::Pointer finder = PointFinder::New();
			//finder->SetPoints(mesh->GetPoints());
			finder->Initialize();
			Point p = Point(0.234, 0.678987, 0.765);
			int id;
			clock_t start = clock();
			for (int i = 0; i < 100000; i++) {
				finder->FindClosestPoint(p);
			}
			std::cout << clock() - start << std::endl;

			start = clock();
			for (int i = 0; i < 100000; i++) {
				id = -1;
				double dist = std::numeric_limits<double>::max();
				for (int i = 0; i < mesh->GetNumberOfPoints(); i++) {
					auto pp = mesh->GetPoint(i);
					double d = (p - pp).length();
					if (d < dist) {
						id = i;
						dist = d;
					}
				}
			}
			std::cout << clock() - start << std::endl;
		});

	connect(ui->menuTest->addAction("tetraSimplTest"), &QAction::triggered,
		this, [&](bool checked) {
			auto td = TetraDecimation::New();
			auto obj = rendererWidget->GetScene()
				->GetCurrentModel()
				->GetDataObject();
			DataObject::Pointer input;
			if (DynamicCast<UnstructuredMesh>(obj)) {
				input = DynamicCast<UnstructuredMesh>(obj)
					->TransferToVolumeMesh();
			}
			else if (DynamicCast<VolumeMesh>(obj)) {
				input = DynamicCast<VolumeMesh>(obj);
			}

			td->SetModel(rendererWidget->GetScene()->GetCurrentModel());
			td->SetInput(input);
			td->Execute();
			input->SetName(obj->GetName() + "*");
			modelTreeWidget->addDataObjectToModelTree(
				input, ItemSource::Algorithm);
			modelTreeWidget->addDataObjectToModelTree(
				td->GetOutput(), ItemSource::Algorithm);
			rendererWidget->update();
		});

	connect(ui->menuTest->addAction("initARAP"), &QAction::triggered, this,
		[&](bool checked) {
			auto test = ARAPTest::New();
			auto model = rendererWidget->GetScene()->GetCurrentModel();
			test->SetModel(model);
			test->SetInput(model->GetDataObject());
			model->SetModelFilter(test);
			test->Initialize();

			Vector3f a;
			a.maxCoeff();

			modelTreeWidget->updateAllAttriubute(model->GetDataObject());


			rendererWidget->update();
		});

	connect(ui->menuTest->addAction("executeARAP"), &QAction::triggered, this,
		[&](bool checked) {
			auto model = rendererWidget->GetScene()->GetCurrentModel();
			ARAPTest* test =
				dynamic_cast<ARAPTest*>(model->GetModelFilter());
			test->Begin();
			rendererWidget->update();
		});

	connect(ui->menuTest->addAction("cancelARAP"), &QAction::triggered, this,
		[&](bool checked) {
			auto model = rendererWidget->GetScene()->GetCurrentModel();
			model->DeleteModelFilter();
			rendererWidget->ChangeInteractorStyle(Interactor::BasicStyle);
		});

	// add test VolumeMesh
	connect(ui->menuTest->addAction("test VolumeMesh"), &QAction::triggered,
		this, [&](bool checked) {
			VolumeMesh::Pointer mesh = VolumeMesh::New();
			Points::Pointer points = Points::New();
			CellArray::Pointer cells = CellArray::New();

			const int subdivision = 10;
			const int subdivision1 = subdivision + 1;
			const int subdivision1_2 = subdivision1 * subdivision1;
			const float step = 1.0 / subdivision;
			auto getIdx = [&](int i, int j, int k) -> int {
				return i * subdivision1_2 + j * subdivision1 + k;
			};
			points->Resize(subdivision1 * subdivision1 * subdivision1);
			for (int i = 0; i <= subdivision; ++i) {
				for (int j = 0; j <= subdivision; ++j) {
					for (int k = 0; k <= subdivision; ++k) {
						points->SetPoint(getIdx(i, j, k), i * step,
							j * step, k * step);
					}
				}
			}
			igIndex cell[8];
			for (int i = 0; i < subdivision; ++i) {
				for (int j = 0; j < subdivision; ++j) {
					for (int k = 0; k < subdivision; ++k) {
						cell[0] = getIdx(i, j, k);
						cell[1] = getIdx(i + 1, j, k);
						cell[2] = getIdx(i + 1, j + 1, k);
						cell[3] = getIdx(i, j + 1, k);
						cell[4] = getIdx(i, j, k + 1);
						cell[5] = getIdx(i + 1, j, k + 1);
						cell[6] = getIdx(i + 1, j + 1, k + 1);
						cell[7] = getIdx(i, j + 1, k + 1);
						cells->AddCellIds(cell, 8);
					}
				}
			}
			mesh->SetPoints(points);
			mesh->SetVolumes(cells);
			mesh->SetName("Test_VolumeMesh");
			modelTreeWidget->addDataObjectToModelTree(
				mesh, ItemSource::Algorithm);
			rendererWidget->update();
		});


	// add test UnstructuredMesh
	connect(ui->menuTest->addAction("test UnstructuredMesh"),
		&QAction::triggered, this, [&](bool checked) {
			UnstructuredMesh::Pointer mesh = UnstructuredMesh::New();
			Points::Pointer points = Points::New();
			CellArray::Pointer cells = CellArray::New();
			UnsignedIntArray::Pointer types = UnsignedIntArray::New();

			const int subdivision = 10;
			const int subdivision1 = subdivision + 1;
			const int subdivision1_2 = subdivision1 * subdivision1;
			const float step = 1.0 / subdivision;
			auto getIdx = [&](int i, int j, int k) -> int {
				return i * subdivision1_2 + j * subdivision1 + k;
			};
			points->Resize(subdivision1 * subdivision1 * subdivision1);
			for (int i = 0; i <= subdivision; ++i) {
				for (int j = 0; j <= subdivision; ++j) {
					for (int k = 0; k <= subdivision; ++k) {
						points->SetPoint(getIdx(i, j, k), i * step,
							j * step, k * step);
					}
				}
			}
			igIndex cell[8];
			for (int i = 0; i < subdivision; ++i) {
				for (int j = 0; j < subdivision; ++j) {
					for (int k = 0; k < subdivision; ++k) {
						cell[0] = getIdx(i, j, k);
						cell[1] = getIdx(i + 1, j, k);
						cell[2] = getIdx(i + 1, j + 1, k);
						cell[3] = getIdx(i, j + 1, k);
						cell[4] = getIdx(i, j, k + 1);
						cell[5] = getIdx(i + 1, j, k + 1);
						cell[6] = getIdx(i + 1, j + 1, k + 1);
						cell[7] = getIdx(i, j + 1, k + 1);
						cells->AddCellIds(cell, 8);
						types->AddValue(IG_HEXAHEDRON);
					}
				}
			}
			mesh->SetPoints(points);
			mesh->SetCells(cells, types);
			mesh->SetName("Test_UnstructuredMesh");
			modelTreeWidget->addDataObjectToModelTree(
				mesh, ItemSource::Algorithm);

			rendererWidget->update();
		});

	connect(ui->menuTest->addAction("executeMarchingCubes"),
		&QAction::triggered, this, [&](bool checked) {
			auto obj = rendererWidget->GetScene()
				->GetCurrentModel()
				->GetDataObject();
            auto filter=ModelClip::New();
            auto pointData= obj->GetAttributeSet()->GetAllPointAttributes();
            filter->SetIsoScalarData(pointData->GetElement(0).pointer,0.58,0);
			filter->SetInput(0, obj);
			filter->Execute();
			modelTreeWidget->addDataObjectToModelTree(
				filter->GetOutput(), ItemSource::Algorithm);
		});
	connect(ui->menuTest->addAction("abc"),
		&QAction::triggered, this, [&](bool checked) {
			auto obj = rendererWidget->GetScene()
				->GetCurrentModel()
				->GetDataObject();
		auto res=SurfaceMesh::New();
		auto filter=iGameModelGeometryFilter::New();
		filter->Execute(obj,res);
		modelTreeWidget->addDataObjectToModelTree(
			res, ItemSource::Algorithm);
		});

	connect(ui->menuTest->addAction("executeClip"), &QAction::triggered, this,
		[&](bool checked) {
			auto obj = rendererWidget->GetScene()
				->GetCurrentModel()
				->GetDataObject();
			auto box = obj->GetBoundingBox();
			auto center = (box.min + box.max) * 0.5;
			float n[3] = { 1,0, 0 };
			float o[3] = { (float)center[0], (float)center[1],
						  (float)center[2] };
			auto filter = ModelClip::New();
			filter->SetIsSlice(true);
			filter->SetPlane(o , n);
			filter->SetInput(0, obj);
			filter->Execute();
			auto result = filter->GetOutput(0);
			modelTreeWidget->addDataObjectToModelTree(
				result, ItemSource::Algorithm);
			rendererWidget->update();
		});


	QMenu* view = ui->menu_filters->addMenu("viewTest");
	QAction* curvature = view->addAction("Get Curvature");
	connect(curvature, &QAction::triggered, this, [&](bool checked) {
		CurvatureFilter::Pointer filter = CurvatureFilter::New();
		auto data = rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
		filter->SetInput(data);
		filter->Execute();
		modelTreeWidget->updateAllAttriubute(data);
		});

	QAction* gradient = view->addAction("Get Gradient");
	connect(gradient, &QAction::triggered, this, [&](bool checked) {
		GradientFilter::Pointer filter = GradientFilter::New();
		auto data =
			rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
		filter->SetInput(data);
		filter->Execute();
		modelTreeWidget->updateAllAttriubute(data);
		});

	QAction* laplacian = view->addAction("Get Laplacian");
	connect(laplacian, &QAction::triggered, this, [&](bool checked) {
		LaplacianFilter::Pointer filter = LaplacianFilter::New();
		auto data =
			rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
		filter->SetInput(data);
		filter->Execute();
		modelTreeWidget->updateAllAttriubute(data);
		});

	QAction* vortex = view->addAction("Get Vortex");
	connect(vortex, &QAction::triggered, this, [&](bool checked) {
		VortexFilter::Pointer filter = VortexFilter::New();
		UnstructuredMesh::Pointer data = DynamicCast<UnstructuredMesh>(
			rendererWidget->GetScene()->GetCurrentModel()->GetDataObject());
		//iGameModelGeometryFilter::Pointer ext =
		//        iGameModelGeometryFilter::New();
		//ext->SetInput(data);
		//ext->Execute();

		//auto mesh = ext->GetExtractMesh();
		//filter->SetInput(mesh);
		//filter->Execute();
		//modelTreeWidget->addDataObjectToModelTree(mesh, ItemSource::Algorithm);


		filter->SetInput(data);
		filter->Execute();
		modelTreeWidget->updateAllAttriubute(data);
		});

}

void igQtMainWindow::initAllDockWidgetConnectWithAction() {
    // connect(ui->action_SearchInfo, &QAction::triggered, this, [&](bool checked)
    // { 	ui->dockWidget_SearchInfo->sh
    //  ow();
    //	});
    connect(ui->action_IsShowColorBar, &QAction::triggered, this,
            &igQtMainWindow::updateColorBarShow);
    connect(ui->action_ExportAnimation, &QAction::triggered, this,
            [&](bool checked) { ui->dockWidget_Animation->show(); });
    connect(ui->action_Scalar, &QAction::triggered, this,
            [&](bool checked) { ui->dockWidget_ScalarField->show(); });
    connect(ui->action_Vector, &QAction::triggered, this, [&](bool checked) {
        ui->dockWidget_VectorField->show();
        ui->widget_VectorField->updateVectorNameList();
    });
	connect(ui->action_Scalar, &QAction::triggered, this,
		[&](bool checked) { ui->dockWidget_ScalarField->show(); });
	connect(ui->action_Glyph, &QAction::triggered, this, [&](bool checked) {
		ui->dockWidget_VectorField->show();
		ui->widget_VectorField->updateVectorNameList();
		});
    connect(ui->action_Tensor, &QAction::triggered, this, [&](bool checked) {
        ui->dockWidget_TensorField->show();
        ui->widget_TensorField->UpdateScalarsNameList();
        ui->widget_TensorField->UpdateTensorsNameList();
    });
    connect(ui->action_FlowField, &QAction::triggered, this,
            [&](bool checked) { ui->dockWidget_FlowField->show(); });
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

	auto DrawSurfaceMeshByPointer = [](SurfaceMesh::Pointer m,
		Painter3D* painter,
		const float color[3]) -> void {
			// 1. draw faces
			painter->SetPen(Color::None);
			painter->SetBrush(color[0], color[1], color[2]);
			igIndex cell[32]{};
			for (int i = 0; i < m->GetNumberOfFaces(); i++) {
				int ncell = m->GetFacePointIds(i, cell);
				for (int j = 2; j < ncell; j++) {
					painter->DrawTriangle(m->GetPoint(cell[0]),
						m->GetPoint(cell[j - 1]),
						m->GetPoint(cell[j]));
				}
			}
			// 2. draw lines
			painter->SetPen(Color::Black);
			painter->SetBrush(Color::None);
			if (m->GetEdges() == nullptr) { m->BuildEdges(); }
			for (int i = 0; i < m->GetNumberOfEdges(); i++) {
				int ncell = m->GetEdgePointIds(i, cell);
				if (cell[0] < 0 || cell[1] < 0) {
					throw std::runtime_error("The index of the edge is negative.");
				}
				else {
					painter->DrawLine(m->GetPoint(cell[0]), m->GetPoint(cell[1]));
				}
			}
			painter->Modified();
	};

	auto AddClippingMeshToScene =
		[DrawSurfaceMeshByPointer](
			const std::string& mainName, SurfaceMesh::Pointer OV,
			SurfaceMesh::Pointer t_IV, SurfaceMesh::Pointer OIV,
			igQtModelDialogWidget* modelTreeWidget) {
				auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

				const std::string OVName = "__" + mainName + "_OV"; // 临时模型
				const std::string IVName = "__" + mainName + "_IV"; // 临时模型
				const std::string OIVName =
					"__" + mainName + "_OIV"; // 临时模型
				const float OVColor[3]{ 1.f, 1.f, 1.f };
				const float IVColor[3]{ 1.f, 1.f, 0.f };
				const float OIVColor[3]{ 1.f, 1.f, 1.f };
				const float OIVAlpha = 0.2f;

				SurfaceMesh::Pointer IV = SurfaceMesh::New();
				OV->SetName(OVName);
				IV->SetName(IVName);
				OIV->SetName(OIVName);

				Model* IVModel{ nullptr };
				bool exist[3]{ false, false, false };
				for (auto& [id, model] : scene->GetModelList()) {
					if (model->GetDataObject()->GetName() == OVName) {
						auto model = scene->GetModelById(id);
						model->SetDataObject(OV);
						exist[0] = true;
					}
					else if (model->GetDataObject()->GetName() == IVName) {
						auto model = scene->GetModelById(id);
						model->SetDataObject(IV);
						exist[1] = true;
						IVModel = model;
					}
					else if (model->GetDataObject()->GetName() == OIVName) {
						auto model = scene->GetModelById(id);
						model->SetDataObject(OIV);
						exist[2] = true;
					}
				}
				if (!exist[0])
					modelTreeWidget->addDataObjectToModelTree(
						OV, ItemSource::Algorithm);
				if (!exist[1]) {
					int id = modelTreeWidget->addDataObjectToModelTree(
						IV, ItemSource::Algorithm);
					IVModel = scene->GetModelById(id);
				}
				if (!exist[2])
					modelTreeWidget->addDataObjectToModelTree(
						OIV, ItemSource::Algorithm);

				DrawSurfaceMeshByPointer(t_IV, IVModel->GetPainter(), IVColor);

				//OV->SetFaceColor(OVColor);
				OV->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
				//IV->SetFaceColor(IVColor);
				//IV->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
				//OIV->SetFaceColor(OIVColor);
				OIV->SetTransparency(OIVAlpha);
				OIV->SetViewStyle(IG_SURFACE);
	};

	connect(ui->action_BoxClipping_Better, &QAction::triggered, this,
		[&](bool checked) {
			if (!rendererWidget->GetScene()->GetCurrentModel()) {
				std::cout << "Need Input" << std::endl;
				return;
			}
			bool ok;
			auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
			auto oldCurrentModel = scene->GetCurrentModel();
			auto dataObject = scene->GetCurrentModel()->GetDataObject();
			auto drawObject = DynamicCast<DrawObject>(dataObject);
			static std::vector<int> supportTypes = { IG_VOLUME_MESH,
													IG_UNSTRUCTURED_MESH,
													IG_STRUCTURED_MESH };
			if (std::find(supportTypes.begin(), supportTypes.end(),
				dataObject->GetDataObjectType()) ==
				supportTypes.end()) {
				std::cout << "This type of mesh can't be clipped"
					<< std::endl;
				return;
			}

			auto inputMesh = DynamicCast<iGame::DataObject>(dataObject);
			auto box = inputMesh->GetBoundingBox();
			auto center = (box.min + box.max) * 0.5;
			auto size = box.max - box.min;

			igQtFilterDialogDockWidget* dialog =
				new igQtFilterDialogDockWidget(this);
			int x_min_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_min(0..1)",
				"0.0");
			int y_min_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_min(0..1)",
				"0.0");
			int z_min_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_min(0..1)",
				"0.0");
			int x_max_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_max(0..1)",
				"0.5");
			int y_max_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_max(0..1)",
				"1.0");
			int z_max_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_max(0..1)",
				"1.0");
			int flip_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip",
				"false");
			dialog->show();

			dialog->setApplyFunctor([=]() {
				bool ok;
				auto Clamp = [](double x, double l, double r) -> double {
					if (x < l) return l;
					if (x > r) return r;
					return x;
				};

				double x_min =
					box.min[0] +
					size[0] * Clamp(dialog->getDouble(x_min_id, ok), 0.,
						1.);
				double y_min =
					box.min[1] +
					size[1] * Clamp(dialog->getDouble(y_min_id, ok), 0.,
						1.);
				double z_min =
					box.min[2] +
					size[2] * Clamp(dialog->getDouble(z_min_id, ok), 0.,
						1.);
				double x_max =
					box.min[0] +
					size[0] * Clamp(dialog->getDouble(x_max_id, ok), 0.,
						1.);
				double y_max =
					box.min[1] +
					size[1] * Clamp(dialog->getDouble(y_max_id, ok), 0.,
						1.);
				double z_max =
					box.min[2] +
					size[2] * Clamp(dialog->getDouble(z_max_id, ok), 0.,
						1.);
				bool flip = dialog->getChecked(flip_id, ok);

				auto clipper = iGameVolumeMeshClipper::New();
				clipper->SetInput(0, dataObject);
				clipper->SetExtent(x_min, x_max, y_min, y_max, z_min, z_max,
					flip);
				clipper->Execute();
				auto OV = DynamicCast<SurfaceMesh>(clipper->GetOutput(0));
				auto IV = DynamicCast<SurfaceMesh>(clipper->GetOutput(1));
				auto OIV = DynamicCast<SurfaceMesh>(clipper->GetOutput(2));
				AddClippingMeshToScene(dataObject->GetName(), OV, IV, OIV,
					modelTreeWidget);
				drawObject->SetVisibility(false);
				scene->SetCurrentModel(oldCurrentModel);
				rendererWidget->update();
				});
		});

	connect(ui->action_PlaneClipping_Better, &QAction::triggered, this,
		[&](bool checked) {
			if (!rendererWidget->GetScene()->GetCurrentModel()) {
				std::cout << "Need Input" << std::endl;
				return;
			}
			bool ok;
			auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
			auto oldCurrentModel = scene->GetCurrentModel();
			auto dataObject = scene->GetCurrentModel()->GetDataObject();
			auto drawObject = DynamicCast<DrawObject>(dataObject);
			static std::vector<int> supportTypes = { IG_VOLUME_MESH,
													IG_UNSTRUCTURED_MESH,
													IG_STRUCTURED_MESH };
			if (std::find(supportTypes.begin(), supportTypes.end(),
				dataObject->GetDataObjectType()) ==
				supportTypes.end()) {
				std::cout << "This type of mesh can't be clipped"
					<< std::endl;
				return;
			}

			auto inputMesh = DynamicCast<iGame::DataObject>(dataObject);
			auto box = inputMesh->GetBoundingBox();
			auto center = (box.min + box.max) * 0.5;
			auto size = box.max - box.min;

			igQtFilterDialogDockWidget* dialog =
				new igQtFilterDialogDockWidget(this);
			int origin_x_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"origin_x(0..1)", "0.5");
			int origin_y_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"origin_y(0..1)", "0.5");
			int origin_z_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"origin_z(0..1)", "0.5");
			int normal_x_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"normal_x(-1..1)", "1.0");
			int normal_y_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"normal_y(-1..1)", "0.0");
			int normal_z_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"normal_z(-1..1)", "0.0");
			int flip_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip",
				"false");
			dialog->show();

			dialog->setApplyFunctor([=]() {
				bool ok;
				auto Clamp = [](double x, double l, double r) -> double {
					if (x < l) return l;
					if (x > r) return r;
					return x;
				};

				double origin_x =
					box.min[0] +
					size[0] * Clamp(dialog->getDouble(origin_x_id, ok),
						0., 1.);
				double origin_y =
					box.min[1] +
					size[1] * Clamp(dialog->getDouble(origin_y_id, ok),
						0., 1.);
				double origin_z =
					box.min[2] +
					size[2] * Clamp(dialog->getDouble(origin_z_id, ok),
						0., 1.);
				double normal_x =
					Clamp(dialog->getDouble(normal_x_id, ok), -1., 1.);
				double normal_y =
					Clamp(dialog->getDouble(normal_y_id, ok), -1., 1.);
				double normal_z =
					Clamp(dialog->getDouble(normal_z_id, ok), -1., 1.);
				bool flip = dialog->getChecked(flip_id, ok);
				if (normal_x == 0. && normal_y == 0. && normal_z == 0.) {
					std::cout << "Normal is a vector of zero" << std::endl;
					return;
				}

				auto clipper = iGameVolumeMeshClipper::New();
				clipper->SetInput(0, dataObject);
				clipper->SetPlane(origin_x, origin_y, origin_z, normal_x,
					normal_y, normal_z, flip);
				clipper->Execute();
				auto OV = DynamicCast<SurfaceMesh>(clipper->GetOutput(0));
				auto IV = DynamicCast<SurfaceMesh>(clipper->GetOutput(1));
				auto OIV = DynamicCast<SurfaceMesh>(clipper->GetOutput(2));
				AddClippingMeshToScene(dataObject->GetName(), OV, IV, OIV,
					modelTreeWidget);
				drawObject->SetVisibility(false);
				scene->SetCurrentModel(oldCurrentModel);
				rendererWidget->update();
				});
		});


	SliceDockWidget = new QDockWidget(this);
	SliceDockWidget->setWindowTitle("网格切割");
	SliceWidget = new igQtModelClipWidget(SliceDockWidget);
	SliceDockWidget->setWidget(SliceWidget);
	SliceDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea);
	SliceDockWidget->hide();
	this->addDockWidget(Qt::LeftDockWidgetArea, SliceDockWidget);
	connect(ui->action_slice, &QAction::triggered, this, [&](bool checked) {


		SliceDockWidget->show();
		auto obj =
			rendererWidget->GetScene()->GetCurrentModel()->GetDataObject();
		SliceWidget->SetOriginDataObject(obj);
        
        rendererWidget->getInteractor()->SetDataObject(obj);
        rendererWidget->getInteractor()->SetPainter(
                rendererWidget->GetScene()->GetCurrentModel()->GetPainter());

		if (rendererWidget->GetScene()->GetInteractor()) {
            rendererWidget->GetScene()->GetInteractor()->SetCallBack(
                    &igQtModelClipWidget::FilterSignal, SliceWidget);
        }

        rendererWidget->getInteractor()->RequestSlicingStyle();

		});
	connect(SliceWidget, &igQtModelClipWidget::DrawClipModel, this,
		[&](SurfaceMesh::Pointer mesh) {
			modelTreeWidget->addDataObjectToModelTree(
				mesh, ItemSource::Algorithm);
		});
	connect(SliceWidget, &igQtModelClipWidget::UpdateClipModel, this,
		[&](SurfaceMesh::Pointer mesh) {
			rendererWidget->update();
		});
}
void igQtMainWindow::initAllMySignalConnections() {
	// connect(rendererWidget, &igQtModelDrawWidget::insertToModelListView,
	// ui->modelTreeView, &igQtModelListView::InsertModel);

	connect(fileLoader, &igQtFileLoader::NewModel, modelTreeWidget,
		&igQtModelDialogWidget::addDataObjectToModelTree);
	connect(fileLoader, &igQtFileLoader::FinishReading, this,
		&igQtMainWindow::updateRecentFilePaths);
	connect(ui->action_DeleteMesh, &QAction::triggered, modelTreeWidget,
		&igQtModelDialogWidget::deleteCurrentModel);


	// connect(fileLoader, &igQtFileLoader::FinishReading, this,
	// &igQtMainWindow::updateViewStyleAndCloudPicture); connect(fileLoader,
	// &igQtFileLoader::FinishReading, this,
	// &igQtMainWindow::updateCurrentSceneWidget);
	connect(fileLoader, &igQtFileLoader::FinishReading, ui->widget_Animation,
		&igQtAnimationWidget::initAnimationComponents);
	//connect(fileLoader, &igQtFileLoader::FinishReading, DeformationWidget, &igQtDeformationWidget::updateInfo);



	connect(ui->widget_FlowField, &igQtStreamTracerWidget::AddStreamObject,
		this, [&](iGame::DataObject::Pointer res) {
			modelTreeWidget->addDataObjectToModelTree(
				res, ItemSource::Algorithm);
		});
	connect(ui->widget_FlowField, &igQtStreamTracerWidget::UpdateStreamObject,
		this, [&](iGame::DataObject::Pointer res) {
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

	connect(fileLoader, &igQtFileLoader::AddFileToModelList, ui->modelTreeView,
		&igQtModelListView::AddModel);
	// connect(rendererWidget, &igQtRenderWidget::AddDataObjectToModelList,
	// ui->modelTreeView, &igQtModelListView::AddModel); connect(rendererWidget,
	// &igQtRenderWidget::UpdateCurrentDataObject, this,
	// &igQtMainWindow::updateCurrentDataObject);

	// connect(fileLoader, &igQtFileLoader::LoadAnimationFile,
	// ui->widget_Animation, &igQtAnimationWidget::initAnimationComponents);

	connect(ui->widget_Animation, &igQtAnimationWidget::UpdateScene, this,
		&igQtMainWindow::UpdateRenderingWidget);

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
	connect(ui->widget_ScalarField, &igQtScalarViewWidget::changeColorBarShow,
		this, &igQtMainWindow::updateColorBarShow);
	connect(this->modelTreeWidget, &igQtModelDialogWidget::CloudPictureChanged,
		ui->widget_ScalarField, &igQtScalarViewWidget::showScalarView);
	connect(ui->widget_ScalarField,
		&igQtScalarViewWidget::ChangeShowColorManager, this, [&]() {
			if (this->ColorManagerWidget->isHidden()) {
				this->ColorManagerWidget->resetColorRange();
				this->ColorManagerWidget->show();
			}
			else {
				this->ColorManagerWidget->hide();
			}
		});
	connect(this->ColorManagerWidget,
		&igQtColorManagerWidget::UpdateColorBarFinished, this, [&]() {
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
	connect(ui->widget_VectorField, &igQtVectorWidget::DrawDireVector, this,
		[&](iGame::DataObject::Pointer res) {
			modelTreeWidget->addDataObjectToModelTree(
				res, ItemSource::Algorithm);
		});
	connect(ui->widget_VectorField, &igQtVectorWidget::UpdateDireVector, this,
		[&](iGame::DataObject::Pointer res) {
			res->Modified();
                modelTreeWidget->updateItemName(res);
			rendererWidget->update();
		});
	connect(ui->widget_TensorField, &igQtTensorWidget::DrawTensorGlyphs, this,
		[&](iGame::DataObject::Pointer res) {
			modelTreeWidget->addDataObjectToModelTree(
				res, ItemSource::Algorithm);
		});
	connect(ui->widget_TensorField, &igQtTensorWidget::UpdateTensorGlyphs, this,
		[&](iGame::DataObject::Pointer res) {
			//res->Modified();
			rendererWidget->update();
		});
	connect(ui->widget_TensorField, &igQtTensorWidget::UpdateAttributes, this,
		[&](iGame::DataObject::Pointer res) {
			modelTreeWidget->updateAllAttriubute(res);
			// modelTreeWidget->addDataObjectToModelTree(res,
			// ItemSource::Algorithm);
		});


	// reset clipping
	connect(ui->action_ResetClipping, &QAction::triggered, this,
		[&](bool checked) {
			if (!rendererWidget->GetScene()->GetCurrentModel()) {
				std::cout << "Need Input" << std::endl;
				return;
			}
			auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
			auto inputMesh = DynamicCast<iGame::DrawObject>(
				scene->GetCurrentModel()->GetDataObject());
			if (!inputMesh->GetClipped()) {
				std::cout << "This type of mesh can't be clipped"
					<< std::endl;
				return;
			}

			inputMesh->GetClipper()->DisableAll();
			inputMesh->SetVisibility(true);

			const std::string OVName =
				"__" + inputMesh->GetName() + "_OV"; // 临时模型
			const std::string IVName =
				"__" + inputMesh->GetName() + "_IV"; // 临时模型
			const std::string OIVName =
				"__" + inputMesh->GetName() + "_OIV"; // 临时模型
			bool exist[3]{ false, false, false };
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
			for (auto& [id, model] : scene->GetModelList()) {
				auto drawObject =
					DynamicCast<DrawObject>(model->GetDataObject());

				if (drawObject->GetName() == OVName) {
					drawObject->SetVisibility(false);
				}
				else if (drawObject->GetName() == IVName) {
					drawObject->SetVisibility(false);
				}
				else if (drawObject->GetName() == OIVName) {
					drawObject->SetVisibility(false);
				}
			}

			rendererWidget->update();
		});


	// box clipping
	connect(ui->action_BoxClipping, &QAction::triggered, this,
		[&](bool checked) {
			if (!rendererWidget->GetScene()->GetCurrentModel()) {
				std::cout << "Need Input" << std::endl;
				return;
			}
			bool ok;

			auto scene = iGame::SceneManager::Instance()->GetCurrentScene();
			auto inputMesh = DynamicCast<iGame::DrawObject>(
				scene->GetCurrentModel()->GetDataObject());
			if (!inputMesh->GetClipped()) {
				std::cout << "This type of mesh can't be clipped"
					<< std::endl;
				return;
			}

			auto box = inputMesh->GetBoundingBox();
			auto center = (box.min + box.max) * 0.5;
			auto size = box.max - box.min;

			igQtFilterDialogDockWidget* dialog =
				new igQtFilterDialogDockWidget(this);
			int x_min_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_min(0..1)",
				"0.0");
			int y_min_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_min(0..1)",
				"0.0");
			int z_min_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_min(0..1)",
				"0.0");
			int x_max_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT, "x_max(0..1)",
				"0.5");
			int y_max_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT, "y_max(0..1)",
				"1.0");
			int z_max_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT, "z_max(0..1)",
				"1.0");
			int flip_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip",
				"false");
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

				cbox.m_Bmin[0] =
					box.min[0] +
					size[0] * Clamp(dialog->getDouble(x_min_id, ok), 0.,
						1.);
				cbox.m_Bmin[1] =
					box.min[1] +
					size[1] * Clamp(dialog->getDouble(y_min_id, ok), 0.,
						1.);
				cbox.m_Bmin[2] =
					box.min[2] +
					size[2] * Clamp(dialog->getDouble(z_min_id, ok), 0.,
						1.);
				cbox.m_Bmax[0] =
					box.min[0] +
					size[0] * Clamp(dialog->getDouble(x_max_id, ok), 0.,
						1.);
				cbox.m_Bmax[1] =
					box.min[1] +
					size[1] * Clamp(dialog->getDouble(y_max_id, ok), 0.,
						1.);
				cbox.m_Bmax[2] =
					box.min[2] +
					size[2] * Clamp(dialog->getDouble(z_max_id, ok), 0.,
						1.);
				cbox.m_Flip = dialog->getChecked(flip_id, ok);

				clipper->Modified();

				rendererWidget->update();
				});
		});


	// plane clipping
	connect(ui->action_PlaneClipping, &QAction::triggered, this,
		[&](bool checked) {
			if (!rendererWidget->GetScene()->GetCurrentModel()) {
				std::cout << "Need Input" << std::endl;
				return;
			}
			bool ok;

			auto scene = iGame::SceneManager::Instance()->GetCurrentScene();

			auto inputMesh = DynamicCast<iGame::DrawObject>(
				scene->GetCurrentModel()->GetDataObject());
			if (!inputMesh->GetClipped()) {
				std::cout << "This type of mesh can't be clipped"
					<< std::endl;
				return;
			}

			auto box = inputMesh->GetBoundingBox();
			auto center = (box.min + box.max) * 0.5;
			auto size = box.max - box.min;

			igQtFilterDialogDockWidget* dialog =
				new igQtFilterDialogDockWidget(this);
			int origin_x_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"origin_x(0..1)", "0.5");
			int origin_y_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"origin_y(0..1)", "0.5");
			int origin_z_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"origin_z(0..1)", "0.5");
			int normal_x_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"normal_x(-1..1)", "1.0");
			int normal_y_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"normal_y(-1..1)", "0.0");
			int normal_z_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_LINE_EDIT,
				"normal_z(-1..1)", "0.0");
			int flip_id = dialog->addParameter(
				igQtFilterDialogDockWidget::QT_CHECK_BOX, "flip",
				"false");
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

				cplane.m_Origin[0] =
					box.min[0] +
					size[0] * Clamp(dialog->getDouble(origin_x_id, ok),
						0., 1.);
				cplane.m_Origin[1] =
					box.min[1] +
					size[1] * Clamp(dialog->getDouble(origin_y_id, ok),
						0., 1.);
				cplane.m_Origin[2] =
					box.min[2] +
					size[2] * Clamp(dialog->getDouble(origin_z_id, ok),
						0., 1.);
				cplane.m_Normal[0] =
					Clamp(dialog->getDouble(normal_x_id, ok), -1., 1.);
				cplane.m_Normal[1] =
					Clamp(dialog->getDouble(normal_y_id, ok), -1., 1.);
				cplane.m_Normal[2] =
					Clamp(dialog->getDouble(normal_z_id, ok), -1., 1.);
				cplane.m_Flip = dialog->getChecked(flip_id, ok);
				if (cplane.m_Normal[0] == 0. && cplane.m_Normal[1] == 0. &&
					cplane.m_Normal[2] == 0.) {
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
	}
	else {
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
	connect(ui->action_select_point, &QAction::triggered, this,
		[&](bool checked) {
			if (ui->action_select_point->isChecked()) {
				if (ui->action_select_points->isChecked()) {
					ui->action_select_points->setChecked(false);
				}
				rendererWidget->ChangeInteractorStyle(
					Interactor::SinglePointSelectionStyle);
			}
			else {
				rendererWidget->ChangeInteractorStyle(
					Interactor::BasicStyle);
			}
		});
	connect(ui->action_select_points, &QAction::triggered, this,
		[&](bool checked) {
			if (ui->action_select_points->isChecked()) {
				if (ui->action_select_point->isChecked()) {
					ui->action_select_point->setChecked(false);
				}
				rendererWidget->ChangeInteractorStyle(
					Interactor::MultiPointSelectionStyle);
			}
			else {
				rendererWidget->ChangeInteractorStyle(
					Interactor::BasicStyle);
			}
		});

	connect(ui->action_select_face, &QAction::triggered, this,
		[&](bool checked) {
			if (ui->action_select_face->isChecked()) {
				if (ui->action_select_faces->isChecked()) {
					ui->action_select_faces->setChecked(false);
				}
				rendererWidget->ChangeInteractorStyle(
					Interactor::SingleFaceSelectionStyle);
			}
			else {
				rendererWidget->ChangeInteractorStyle(
					Interactor::BasicStyle);
			}
		});
	connect(ui->action_select_faces, &QAction::triggered, this,
		[&](bool checked) {
			if (ui->action_select_faces->isChecked()) {
				if (ui->action_select_face->isChecked()) {
					ui->action_select_face->setChecked(false);
				}
				rendererWidget->ChangeInteractorStyle(
					Interactor::MultiFaceSelectionStyle);
			}
			else {
				rendererWidget->ChangeInteractorStyle(
					Interactor::BasicStyle);
			}
		});
	connect(ui->action_drag_point, &QAction::triggered, this,
		[&](bool checked) {
			if (ui->action_drag_point->isChecked()) {
				rendererWidget->ChangeInteractorStyle(
					Interactor::DragPointStyle);
			}
			else {
				rendererWidget->ChangeInteractorStyle(
					Interactor::BasicStyle);
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
