#include <IQCore/igQtMainWindow.h>
#include <QApplication>

#if __linux__
#include <qtextcodec.h>
#else
#include <QtCore/Qtextcodec.h>
#endif

#include <iGameScene.h>
#include <iGameRenderWindow.h>
#include <iGameInteractor.h>
#include <iGameFileIO.h>

int main(int argc, char* argv[]) {


	///* 创建场景*/
	//auto scene = iGame::Scene::New();
	///* 读取文件测试并将其放入场景*/
	//const std::string fileName = "F:\\OpeniGame\\Model\\Common\\TestModel\\Tet_Plane.vtk";
	////const std::string fileName = "F:\\OpeniGame\\Model\\Common\\TestModel\\DrivAer_fastback_base_0.4_remesh_coarse_kw_CPU_test_P_V.cgns";  
	////const std::string fileName = "F:\\OpeniGame\\Model\\CGNS\\structured\\delta.cgns";
	//iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
	////auto drawObj = DynamicCast<iGame::DrawObject>(obj);
	////drawObj->ConvertToDrawableData();
	////drawObj->ViewCloudPicture(scene, 0);
	////if (obj != nullptr) {
	////	scene->AddModel(obj);
	////}
	////else {
	////	std::cout << "Read ERROR!\n";
	////}
	////drawObj->AddViewStyle(IG_WIREFRAME);
	///*  auto input=obj;
	//  auto filter = iGame::ModelClip::New();
	//  filter->SetInput(input);
	//  auto bound = input->GetBoundingBox();
	//  auto ori = (bound.min + bound.max) / 2;
	//  float n[3] = { 0, 1, 0 };
	//  float o[3] = { ori[0], ori[1], ori[2] };
	//  filter->SetPlane(o, n);
	//  filter->SetIsSlice(true);
	//  filter->Execute();
	//  auto res =filter->GetOutput();
	//  if (res != nullptr) {
	//	  scene->AddModel(res);
	//  }
	//  */
	//auto input = obj;
	//auto filter = iGame::ModelClip::New();
	//auto& attr = obj->GetAttributeSet()->GetAllPointAttributes()->GetElement(0);
	//auto range = attr.GetDataRange();
	//auto array = attr.pointer;
	//int dimension = 0;
	//double value = 0.0;
	//value = range->GetValue(dimension * 2 + 2)*2/3 + range->GetValue(dimension * 2 + 3)/3;
	//filter->SetInput(input);
	//filter->SetIsoScalarData(array, value, dimension);
	//filter->Execute();
	//auto res = filter->GetOutput();
	//if (res != nullptr) {
	//	scene->AddModel(res);
	//}
	//(DynamicCast<iGame::DrawObject>(res))->ConvertToDrawableData(); 
	//(DynamicCast<iGame::DrawObject>(res))->ViewCloudPicture(scene, 0, dimension);

	///* 启动窗口设置*/
	//iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
	//window->setSize(1920, 1080);
	//window->setScene(scene);
	//auto basicInteractor = iGame::Interactor::New();
	//basicInteractor->Initialize(scene);
	//window->setInteractor(basicInteractor);
	//window->show();


	Q_INIT_RESOURCE(iGameQtMainWindow);
	QCoreApplication::setAttribute(
	       Qt::AA_EnableHighDpiScaling); // 窗口高分辨率支持
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps); // 图标高分辨率支持
	QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
	QApplication a(argc, argv);

	QSurfaceFormat format;
	format.setRenderableType(QSurfaceFormat::OpenGL);
	format.setVersion(4, 6); // Mac set to format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);

	format.setRedBufferSize(8); // RGBA8
	format.setGreenBufferSize(8);
	format.setBlueBufferSize(8);
	// format.setAlphaBufferSize(8); // This will cause the OpenGLWidget window to be transparent

	// If the depth buffer is set to 24, the line width can only be set to 1
	format.setDepthBufferSize(32);

	// If the template buffer is turned on, the line width can only be set to 1
	// format.setStencilBufferSize(8);

	format.setSamples(1);
	QSurfaceFormat::setDefaultFormat(format);

	a.processEvents();
	igQtMainWindow w;
	QTextCodec* codec = QTextCodec::codecForName("GBK");
	w.setWindowTitle(codec->toUnicode("iGame-MeshView"));
	w.show();
	w.showMaximized();
	a.exec();
	return 0;
}