#include <IQCore/igQtMainWindow.h>
#include <QApplication>

#if __linux__
#include <qtextcodec.h>
#else
#include <QtCore/Qtextcodec.h>
#endif

int main(int argc, char* argv[]) {

    //	::testing::InitGoogleTest(&argc, argv);
    //	// 运行所有测试
    //	return RUN_ALL_TESTS();

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