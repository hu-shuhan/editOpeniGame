//
// Created by OpeniGame on 25-3-31.
//
/**
 * @class   TestTensor
 * @brief   TestTensor's brief
 */

#include <TensorViewFilter/iGameTensorBase.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iostream>
#include <string>
//#include<vld.h>
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
    auto m_TensorObject = iGame::iGameTensorBase::New();
    //设置顶点数据
    m_TensorObject->SetPoints(mesh->GetPoints());
    //设置显示图元类型，目前支持ELLIPSOID和CUBOID两种
    m_TensorObject->SetGlyphType(iGame::iGameTensorRepresentation::CUBOID);
    //设置张量场数据，是3*3张量场
    m_TensorObject->SetTensorAttributes(tensorData);
    //设置图元颜色，如果是SetPositionsScalarArray，则是按照标量数据影射
    m_TensorObject->SetPositionsScalarArray(tensorData, 0);
    /*//设置图元颜色，如果是SetPositionColors，则需要直接给入颜色数据
    m_TensorObject->SetPositionColors(colors);*/
    //显示张量场
    m_TensorObject->ShowTensorField();
    scene->AddModel(m_TensorObject);

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