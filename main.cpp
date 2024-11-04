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


//    /* 创建场景*/
//    auto scene = iGame::Scene::New();
//    /* 读取文件测试并将其放入场景*/
//    const std::string fileName = "C:\\Users\\m_ky\\Desktop\\Resource\\xml\\sukong\\sukong.pvd";
//    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
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