//
// Created by m_ky on 2024/11/26.
//

/**
 * @class   TestVector
 * @brief   TestVector's brief
 */

#include <StreamView/iGameStreamBase.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iostream>
#include <string>
//#include<vld.h>
int main() {
    auto scene = iGame::Scene::New();
    const std::string fileName = "./Models/kit.vtk";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
    if (obj != nullptr) {
        scene->AddModel(obj);
    } else {
        std::cout << "Read ERROR!\n";
    }
    auto m_StreamBase = iGame::iGameStreamBase::New();
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

    auto streamtracer = m_StreamBase->streamFilter;
    streamtracer->initStreamTracer(_obj);
    //auto seeds=streamtracer->getModelSelect();//当实际已经选中了重点区域时直接调用该函数
    Vector3f boundMax = streamtracer->GetMesh()->GetBoundingBox().max;//包围盒区域
    Vector3f boundMin = streamtracer->GetMesh()->GetBoundingBox().min;
    Vector3f centerMax=(boundMax-boundMin)/5+boundMin;//模拟被选中重点区域
    auto seeds=streamtracer->getAllSubBlockCenters(boundMax,boundMin,centerMax,boundMin,2,4);//4，6为划分子块的数量
   // auto seeds=streamtracer->seedPCoordGenerate(boundMax,boundMin,1000);//均匀生成1000个种子点
    float lengthOfStreamLine=5;
    float lengthOfStep=0.3;
    float maxSteps=1000;
    float terminalSpeed=0.005;
    streamtracer->SetInput(seeds, VectorName[0], lengthOfStreamLine, lengthOfStep,
                                                 terminalSpeed, maxSteps);
    streamtracer->Execute();
    m_StreamBase->SetUpdate(true);
    auto output= streamtracer->GetOutput();//这就是输出的流线数据
    scene->AddModel(m_StreamBase);
    DynamicCast<iGame::DrawObject>(m_StreamBase)->AddViewStyle(IG_WIREFRAME);
    //Set the original model to be invisible
    scene->ChangeModelVisibility(1, false);
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    window->Show();
}