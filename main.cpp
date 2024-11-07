#include <IQCore/igQtMainWindow.h>
#include <QApplication>

#if __linux__
#include <qtextcodec.h>
#else
#include <QtCore/Qtextcodec.h>
#endif

#include <VectorView/iGameVectorBase.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
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

    //m_VectorBase->SetViewStyle(IG_SURFACE);

    //scene->ChangeModelVisibility(0, false);
    // 
    // 



    //Q_INIT_RESOURCE(iGameQtMainWindow);
    //QCoreApplication::setAttribute(
    //       Qt::AA_EnableHighDpiScaling); // 窗口高分辨率支持
    //QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps); // 图标高分辨率支持
    //QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    //QApplication a(argc, argv);

    //QSurfaceFormat format;
    //format.setRenderableType(QSurfaceFormat::OpenGL);
    //format.setVersion(4, 6); // Mac set to format.setVersion(3, 3);
    //format.setProfile(QSurfaceFormat::CoreProfile);

    //format.setRedBufferSize(8); // RGBA8
    //format.setGreenBufferSize(8);
    //format.setBlueBufferSize(8);
    // format.setAlphaBufferSize(8); // This will cause the OpenGLWidget window to be transparent

    // If the depth buffer is set to 24, the line width can only be set to 1
    //format.setDepthBufferSize(32);

    // If the template buffer is turned on, the line width can only be set to 1
    // format.setStencilBufferSize(8);

    //format.setSamples(1);
    //QSurfaceFormat::setDefaultFormat(format);

    //a.processEvents();
    //igQtMainWindow w;
    //QTextCodec* codec = QTextCodec::codecForName("GBK");
    //w.setWindowTitle(codec->toUnicode("iGame-MeshView"));
    //w.show();
    //w.showMaximized();
    //a.exec();
    //return 0;
}