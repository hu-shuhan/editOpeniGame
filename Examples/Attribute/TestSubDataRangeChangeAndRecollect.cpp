//
// Created by m_ky on 2025/1/27.
//

/**
 * @class   TestSubDataRangeChangeAndRecollect
 * @brief   TestSubDataRangeChangeAndRecollect's brief
 */
#include <iostream>
#include <iGameVolume.h>
#include <Core/iGameScene.h>
#include <iGameRenderWindow.h>
#include <iGameInteractor.h>
#include <iGameFileIO.h>

#include <iGameRenderWindow.h>
#include <iGameInteractor.h>

int main(int argc, char* argv[]) {
    auto scene = iGame::Scene::New();
    /* Change the Path to your Model storage path. */
    const std::string fileName = "./Models/CAD11/_frames.pvd";
    iGame::DataObject::Pointer obj = iGame::FileIO::ReadFile(fileName);
//    auto obj = reader->ReadOdbFirstFrameMesh(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
    } else {
        scene->AddModel(obj);
    }
    /* Launch window Settings */
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    std::cout << "attr num : " << obj->GetAttributeSet()->GetNumberOfAttributes() << '\n';
    /* Reset DataObject's magnitude dimension to Default.*/
    obj->GetAttributeSet()->GetAttribute(0).GetDataRange()->SetValue(0, DBL_MIN);
    obj->GetAttributeSet()->GetAttribute(0).GetDataRange()->SetValue(1, DBL_MAX);
    /* Recollect subdata object's MIN/MAX DataRange to update the current dataObject's data range. */
    obj->ReCollectSubDataObjectDataRange();
    /* Update Sub data Object's data range to the parent data range. */
    obj->UpdateSubDataObjectDataRange();
    DynamicCast<iGame::DrawObject>(obj)->ViewCloudPicture(scene, 0);

    /* show single window */
    window->Show();

}
