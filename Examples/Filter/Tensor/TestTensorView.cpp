//
// Created by OpeniGame on 25-3-31.
//
/**
 * @class   TestTensor
 * @brief   TestTensor's brief
 */

#include <TensorViewFilter/iGameTensorFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iostream>
#include <string>


int main() {
    /* 创建场景*/
    auto scene = iGame::Scene::New();
    const std::string fileName = "./Models/Quad_Plane_Tensor.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) { std::cout << "Read ERROR!\n"; }
    auto mesh = iGame::DynamicCast<iGame::PointSet>(obj);
    if (mesh == nullptr) { std::cout << "Mesh ERROR!\n"; }
    auto allAttributes = mesh->GetAttributeSet()->GetAllAttributes();
    if (!allAttributes) {
        std::cout << "No Tensor Data\n";
        return 0;
    }
    iGame::ArrayObject::Pointer tensorData = nullptr;
    for (int i = 0; i < allAttributes->GetNumberOfElements(); i++) {
        auto attribute = allAttributes->GetElement(i);
        if (attribute.type == IG_TENSOR && attribute.attachmentType == IG_POINT) { tensorData = attribute.pointer; }
    }
    if (!tensorData) {
        std::cout << "No Tensor Data\n";
        return 0;
    }
    auto m_TensorFilter = iGame::iGameTensorFilter::New();
    //设置输入
    m_TensorFilter->SetInput(mesh);
    //设置张量场数据，是3*3张量场,如果没有输入则会默认找网格的第一个张量场
    m_TensorFilter->SetTensorAttributes(tensorData);
    //设置显示图元类型，目前支持ELLIPSOID和CUBOID两种
    m_TensorFilter->SetGlyphType(iGame::iGameTensorRepresentation::CUBOID);
    //设置绘制精度
    m_TensorFilter->SetSliceNum(5);
    //设置图元缩放比例
    m_TensorFilter->SetGlyphScale(0.02);

    if (m_TensorFilter->Execute()) {
        auto res = DynamicCast<DrawObject>(m_TensorFilter->GetOutput());
        scene->AddModel(res);
        int num = res->GetAttributeSet()->GetNumberOfAttributes();
        if (num > 0) {
            res->ViewCloudPicture(scene, 0);
        }
    }
    else {
        std::cout << "Generate Tensor Filed\n";
        return 0;
    }

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
