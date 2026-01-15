#include <iostream>
#include <iGameVolume.h>
#include <Core/iGameScene.h>
#include <iGameRenderWindow.h>
#include <iGameMultiRenderWindowManager.h>
#include <iGameInteractor.h>
#include <iGameFileIO.h>
#include <Clip/iGameModelClip.h>
#include <VectorView/iGameVectorBase.h>
int main() {
    /* ��������*/
    auto scene = iGame::Scene::New();
    /* ��ȡ�ļ����Բ�������볡��*/
    const std::string fileName = "./Models/StreamTest.vtk";
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
    if (VectorName.empty()) { return 0; }
    //set vectorBase
    m_VectorBase->SetInit(false);
    //set drawtype: 1.AllCell 2.CellInRange 3.EveryNth
    m_VectorBase->SetDrawMode(iGame::iGameVectorBase::DrawType::EveryNth);
    m_VectorBase->SetNth(5);
    m_VectorBase->DrawVector(VectorName[0], currentModel->GetDataObject());
    scene->AddModel(m_VectorBase);
    //Set the original model to be invisible
    scene->ChangeModelVisibility(0, false);

    /* ������������*/
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