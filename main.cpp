#include <IQCore/igQtMainWindow.h>
#include <QApplication>

#if __linux__
#include <qtextcodec.h>
#else
#include <QtCore/Qtextcodec.h>
#endif

#include <Deformation/iGameStressDeformationFilterCode.h>
#include "Core/Interactor/iGameInteractor.h"
#include "Core/RenderWindow/iGameMultiRenderWindowManager.h"
#include "Core/RenderWindow/iGameRenderWindow.h"
#include "iGameFileIO.h"
#include <iostream>
int main(int argc, char* argv[]) {
//    iGame::StressDeformationCodeFilter::Pointer filter = iGame::StressDeformationCodeFilter::New();
//    //    const std::string fileName = "./Models/sukong_Step-1_2.vtu";
//    // Any Model with Vector Attribute.
////    const std::string fileName = "D:\\dev\\TestModel\\sukong_Step-1_10.vtu";
//    const std::string fileName = "D:\\dev\\TestModel\\sukong_10_binary2.vtu";
//    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
//
////    obj->GetDeformationData()->SetAttributeName("UVW");
//    obj->GetDeformationData()->SetAttributeName("U");
//    filter->SetInput(obj);
//
////    filter->CalculateIdealDSF();
//    filter->SetScaleFactorX(70497564.91641039);
//    filter->SetScaleFactorY(70497564.91641039);
//    filter->SetScaleFactorZ(70497564.91641039);
//    filter->Execute();
//
//    auto res = filter->GetOutput(0);
//    //    std::cout << DynamicCast<iGame::PointSet>(res)->GetNumberOfPoints() << std::endl;
//    auto pointset_raw = DynamicCast<iGame::PointSet>(obj);
//    auto attribute_set = pointset_raw->GetAttributeSet()->GetAttribute(obj->GetDeformationData()->GetDeformationAttributeName()).pointer;
//    auto pointset_new = DynamicCast<iGame::PointSet>(res);
////    for(int i= 0, j = 0; i < 10; i ++, j += 3){
////        std::cout << "==================" << i << std::endl;
////
////        //  Deformation value
////        std::cout << pointset_raw->GetDeformationData()->GetScaleFactorX() * attribute_set->GetValue(j + 0) << ' '
////                  << pointset_raw->GetDeformationData()->GetScaleFactorY() * attribute_set->GetValue(j + 1) << ' '
////                  << pointset_raw->GetDeformationData()->GetScaleFactorZ() * attribute_set->GetValue(j + 2) << std:: endl;
////        // Before Deformation Point Position
////        std:: cout << pointset_raw->GetPoint(i)[0] << ' ' << pointset_raw->GetPoint(i)[1] << ' ' << pointset_raw->GetPoint(i)[2] << '\n';
////        // After Deformation Point Position
////        std:: cout << pointset_new->GetPoint(i)[0] << ' ' << pointset_new->GetPoint(i)[1] << ' ' << pointset_new->GetPoint(i)[2] << '\n';
////
////    }
//
////        iGame::DynamicCast<iGame::DrawObject>(res)->AddViewStyle(IG_WIREFRAME);
////        iGame::DynamicCast<iGame::DrawObject>(res)->AddViewStyle(IG_POINTS);
////        iGame::DynamicCast<iGame::DrawObject>(res)->AddViewStyle(IG_SURFACE);
////        iGame::DynamicCast<iGame::DrawObject>(res)->ConvertToDrawableData();
//    /* 创建场景*/
//    auto scene = iGame::Scene::New();
//    if (obj != nullptr) {
//        scene->AddModel(res);
//    } else {
//        std::cout << "error\n";
//    }
//
//    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
//    window->SetSize(1920, 1080);
//    window->SetScene(scene);
//
//    auto interactor = iGame::Interactor::New();
//    interactor->Initialize(scene);
//    interactor->CreateDefaultStyle();
//    window->SetInteractor(interactor);
//    window->Show();

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

    a.processEvents();
    igQtMainWindow w;
    QTextCodec* codec = QTextCodec::codecForName("GBK");
    w.setWindowTitle(codec->toUnicode("iGameVis 2.0"));
    w.show();
    w.showMaximized();
    w.initArgs(a.arguments());
    a.exec();
    return 0;
}
