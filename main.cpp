#include <IQCore/igQtMainWindow.h>
#include <QApplication>

#if __linux__
#include <qtextcodec.h>
#else
#include <QtCore/Qtextcodec.h>
#endif

#include <iGameScene.h>
#include <Core/RenderWindow/iGameRenderWindow.h>
#include <iGameMultiRenderWindowManager.h>
#include <iGameInteractor.h>
#include <iGameFileIO.h>

#include <Spline XML/iGameSplineSurfaceReader.h>
#include <Spline XML/iGameSplineVolumeReader.h>
int main(int argc, char* argv[]) {
    /* 创建场景*/
    auto scene = iGame::Scene::New();
    /* 读取文件测试并将其放入场景*/
    //const std::string fileName = "D:\\lab\\build\\StreamTest.vtk";
    const std::string fileName = "D:\\lab\\build\\DrivAer_fastback_base_0.4_remesh_coarse_kw_CPU_test_P_V.cgns";
    //const std::string fileName = "D:\\lab\\model\\CAD11\\_frames.pvd";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj != nullptr) {
        scene->AddModel(obj);
    } else {
        std::cout << "Read ERROR!\n";
    }


    auto m_VectorBase = iGame::iGameVectorBase::New();
    //set Arrow parameter
    m_VectorBase->SetArrow(0.01, 0.03, 0.005, 0.04);
    auto currentModel = scene->GetCurrentModel();
    if (!currentModel) {
        std::cout << "haven''t model" << std::endl;
        return 0;
    }
    auto _obj = currentModel->GetDataObject();
    iGame::AttributeSet* _AttributeSet;
    std::vector<std::string> VectorName;
    // find vector and its name
    if (_obj->HasSubDataObject()) {
        auto it = _obj->SubDataObjectIteratorBegin();
        _AttributeSet = it->second->GetAttributeSet();
    } else {
        _AttributeSet = _obj->GetAttributeSet();
    }
    if (!_AttributeSet) return 0;
    _AttributeSet->TransformScalars2VectorArray();
    auto allAttributes = _AttributeSet->GetAllAttributes();
    if (!allAttributes) return 0;

    for (int i = 0; i < allAttributes->GetNumberOfElements(); i++) {
        auto attribute = allAttributes->GetElement(i);
        if (attribute.type == IG_VECTOR) {
            if (attribute.pointer) {
                VectorName.emplace_back(attribute.pointer->GetName());
                std::cout << attribute.pointer->GetName() << std::endl;
            }
        }
    }

    //set vectorBase
    m_VectorBase->SetInit(false);
    //set drawtype: 1.AllCell 2.CellInRange 3.EveryNth
    m_VectorBase->SetDrawMode(iGameVectorBase::DrawType::CellInRange);
    m_VectorBase->SetCellRange(0, 600);
    // m_VectorBase->SetNth(5);
    m_VectorBase->DrawVector(VectorName[0], currentModel->GetDataObject());
    scene->AddModel(m_VectorBase);
    //Set the original model to be invisible
    scene->ChangeModelVisibility(0, false);

    /* 启动窗口设置*/
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->setSize(1920, 1080);
    window->setScene(scene);
    auto basicInteractor = iGame::Interactor::New();
    basicInteractor->Initialize(scene);
    window->setInteractor(basicInteractor);
    window->show();


//    /* 创建场景*/
//    auto scene = iGame::Scene::New();
//    /* 读取文件测试并将其放入场景*/
//    const std::string fileName = "C:\\Users\\m_ky\\Desktop\\Resource\\Model\\Armadillo.obj";
////    const std::string fileName = "C:\\Users\\m_ky\\Desktop\\Resource\\xml\\sukong\\sukong.pvd";
////    const std::string fileName = "C:\\Users\\m_ky\\Desktop\\Resource\\xml\\pvd\\CAD11\\_frames.pvd";
////    const std::string fileName = "C:\\Users\\m_ky\\Desktop\\Resource\\machineHand\\sukong.odb";
//    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
//
////    iGame::SplineSurfaceReader::Pointer reader = iGame::SplineSurfaceReader::New();
//
////    iGame::SplineVolumeReader::Pointer reader = iGame::SplineVolumeReader::New();
////    reader->SetFilePath("C:\\Users\\m_ky\\Desktop\\Resource\\xml\\out\\O.xml");
////    reader->Execute();
////    auto obj = reader->GetOutput();
//
//    DynamicCast<iGame::DrawObject>(obj)->AddViewStyle(IG_WIREFRAME);
//    if(obj != nullptr){
//        scene->AddModel(obj);
//    } else {
//        std::cout << "Read ERROR!\n";
//    }
//
//    /* 启动窗口设置*/
//    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
//    window->setSize(1920, 1080);
//    window->setScene(scene);
//    auto basicInteractor = iGame::Interactor::New();
//    basicInteractor->Initialize(scene);
//    window->setInteractor(basicInteractor);
//    window->show();
//
//    /* 多窗口测试 */
//    iGame::RenderWindow::Pointer window_2 = iGame::RenderWindow::New();
//    auto scene2 = iGame::Scene::New();
//    window_2->setScene(scene2);
//    auto basicInteractor2 = iGame::Interactor::New();
//    basicInteractor2->Initialize(scene2);
//    window_2->setInteractor(basicInteractor2);
//    iGame::DataObject::Pointer obj2 = iGame::FileIO::ReadFile("C:\\Users\\m_ky\\Desktop\\Resource\\Model\\bunny.obj");
//    scene2->AddModel(obj2);

    /* 多窗口同时运行需在MultiRenderWindowManager中注册 */
//    iGame::MultiRenderWindowManager::Instance()->Register(window);
//    iGame::MultiRenderWindowManager::Instance()->Register(window_2);
//    iGame::MultiRenderWindowManager::Instance()->ShowAllRegisterWindow();


//    Q_INIT_RESOURCE(iGameQtMainWindow);
//    QCoreApplication::setAttribute(
//           Qt::AA_EnableHighDpiScaling); // 窗口高分辨率支持
//    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps); // 图标高分辨率支持
//    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
//    QApplication a(argc, argv);
//
//    QSurfaceFormat format;
//    format.setRenderableType(QSurfaceFormat::OpenGL);
//    format.setVersion(4, 6); // Mac set to format.setVersion(3, 3);
//    format.setProfile(QSurfaceFormat::CoreProfile);
//
//    format.setRedBufferSize(8); // RGBA8
//    format.setGreenBufferSize(8);
//    format.setBlueBufferSize(8);
//    // format.setAlphaBufferSize(8); // This will cause the OpenGLWidget window to be transparent
//
//    // If the depth buffer is set to 24, the line width can only be set to 1
//    format.setDepthBufferSize(32);
//
//    // If the template buffer is turned on, the line width can only be set to 1
//    // format.setStencilBufferSize(8);
//
//    format.setSamples(1);
//    QSurfaceFormat::setDefaultFormat(format);
//
//    a.processEvents();
//    igQtMainWindow w;
//    QTextCodec* codec = QTextCodec::codecForName("GBK");
//    w.setWindowTitle(codec->toUnicode("iGame-MeshView"));
//    w.show();
//    w.showMaximized();
//    a.exec();
//    return 0;
}