#include <IQCore/igQtMainWindow.h>
#include <QApplication>
#if __linux__
#include <qtextcodec.h>
#else
#include <QtCore/Qtextcodec.h>
#endif


#include <Nastran/iGameNastranReader.h>
#include <iGameScene.h>
#include <iGameRenderWindow.h>
int main(int argc, char* argv[]) {
//        iGame::NastranReader::Pointer rd = iGame::NastranReader::New();
//        rd->SetBDFFileName("D:/Project/editOpeniGame/Examples/Models/ogs.bdf");
//        rd->SetFilePath("D:/Project/editOpeniGame/Examples/Models/ogs.bdf");
//        rd->SetOP2FileName("D:/Project/editOpeniGame/Examples/Models/ogs.op2");
//        rd->Execute();
//        auto obj = rd->GetOutput();
//        std::cout << "Attributes Num : " << obj->GetAttributeSet()->GetNumberOfAttributes() << std::endl;
//
//        if (obj == nullptr) {
//            std::cerr << "Error: Failed to load model" << std::endl;
//            return -1;
//        }
//
//        /* Launch window Settings */
//        auto scene = iGame::Scene::New();
//        iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
//        window->SetSize(1920, 1080);
//        window->SetScene(scene);
//
//        auto interactor = iGame::Interactor::New();
//        interactor->Initialize(scene);
//        interactor->CreateDefaultStyle();
//        window->SetInteractor(interactor);
//
//        scene->AddModel(obj);
//
//        /* show single window */
//        window->Show();
//

    Q_INIT_RESOURCE(iGameQtMainWindow);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling); // 窗口高分辨率支持
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);     // 图标高分辨率支持
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QApplication a(argc, argv);
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(4, 6); // Mac set to format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setRedBufferSize(8); // RGBA8
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    //format.setAlphaBufferSize(8); // This will cause the OpenGLWidget window to be transparent
    // If the depth buffer is set to 24, the line width can only be set to 1
    format.setDepthBufferSize(32);
    // If the template buffer is turned on, the line width can only be set to 1
    format.setStencilBufferSize(8);
    format.setSamples(1);
    QSurfaceFormat::setDefaultFormat(format);
    // Init Log System
    Log::Init();
    // IGAME_CORE_INFO("Start Core Log");
    // IGAME_RENDERING_INFO("Start Rendering Log");

    a.processEvents();
    igQtMainWindow w;
    QTextCodec* codec = QTextCodec::codecForName("GBK");
    w.setWindowTitle(codec->toUnicode("iGameVis 1.0"));
    w.show();
    w.showMaximized();
    w.initArgs(a.arguments());
    a.exec();
    return 0;
}
