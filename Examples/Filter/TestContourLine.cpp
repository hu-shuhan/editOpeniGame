#include <iostream>
#include <iGameVolume.h>
#include <Core/iGameScene.h>
#include <iGameRenderWindow.h>
#include <iGameInteractor.h>
#include <iGameFileIO.h>
#include <Clip/iGameModelClip.h>
#include <Contour/iGameContourFilter.h>

int main(){
    /* 创建场景*/
    auto scene = iGame::Scene::New();
    const std::string fileName = "F:/OpeniGame/Model/Common/dianfengshan.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
    }
    auto input = obj;
    //新建filter用于提取等值线等值面
    auto filter = iGame::ContourFilter::New();
    //数据的index
    int index = 0;
    auto pointAttibutes = obj->GetAttributeSet()->GetAllPointAttributes();
    if (pointAttibutes == nullptr|| index > pointAttibutes->GetNumberOfElements()) {
        std::cout<< "数据有误,无效点数据\n";
        return 0;
    }

    
    auto& attr = pointAttibutes->GetElement(index);
    auto range = attr.GetDataRange();
    auto array = attr.pointer;
    //数据的维度
    int dimension = 0;
    std::vector<double>values;
    double value = 0.0;
    //设定好想要提取的等值数据
    value = range->GetValue(dimension * 2 + 2) * 2 / 3 + range->GetValue(dimension * 2 + 3) / 3;
    values.push_back(value);
    value = range->GetValue(dimension * 2 + 2) / 3 + range->GetValue(dimension * 2 + 3) * 2 / 3;
    values.push_back(value);
    value = range->GetValue(dimension * 2 + 2) / 2 + range->GetValue(dimension * 2 + 3) / 2;
    values.push_back(value);
    //设置输入模型
    filter->SetInput(input);
    //设置对于的数据集，等值数据以及维度，不建议用矢量的长度模式，因为数据并不是线性的
    filter->SetIsoScalarData(array, values, dimension);
    //执行
    filter->Execute();
    auto res = filter->GetOutput();
    if (res != nullptr) {
        scene->AddModel(res);
    }
    (DynamicCast<iGame::DrawObject>(res))->ConvertToDrawableData();
    (DynamicCast<iGame::DrawObject>(res))->SetViewStyle(IG_SURFACE | IG_WIREFRAME);
    (DynamicCast<iGame::DrawObject>(res))->ViewCloudPicture(scene, index, dimension);
    /* 启动窗口设置*/
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
    return 0;
}