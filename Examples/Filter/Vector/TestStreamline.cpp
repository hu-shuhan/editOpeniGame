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
    Vector3f P1 = streamtracer->GetMesh()->GetBoundingBox().max;
    Vector3f P2 = streamtracer->GetMesh()->GetBoundingBox().min;
    auto seeds = streamtracer->seedPCoordGenerate(200, P1, P2);
    std::vector<std::vector<float>> streamlineColor;
    std::vector<std::vector<float>> streamline;
    float lengthOfStreamLine=5;
    float lengthOfStep=0.3;
    float maxSteps=1000;
    float terminalSpeed=0.005;
    streamline = streamtracer->showStreamLineMix(seeds, VectorName[0], streamlineColor, lengthOfStreamLine, lengthOfStep,
                                                 terminalSpeed, maxSteps);
    m_StreamBase->SetStreamLine(streamline, streamlineColor);
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