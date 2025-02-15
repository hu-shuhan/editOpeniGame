#include <Abaqus/iGameODBReader.h>
#include <iGameMultiRenderWindowManager.h>
#include <iGameRenderWindow.h>
#include <iGameInteractor.h>
#include <iGameScene.h>
/* Read Only mesh Without Field data*/
void TestReadOnlyMesh(){
    /* 创建场景*/
    auto scene = iGame::Scene::New();
    const std::string fileName = "./Models/CP10_L6_DP1_new.odb";
    iGame::ODBReader::Pointer reader = iGame::ODBReader::New();
    /* Read */
    auto obj = reader->ReadOdbRawMesh(fileName);
    /* Check Attribute num is ZERO. */
    std::cout << "======Attribute num  : " << obj->GetAttributeSet()->GetNumberOfAttributes() << '\n';
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

    /* show single window */
    window->Show();

}
void TestTargetFrameFieldData(){

}


void TestFirstFrameMeshWithFieldData(){
    /* 创建场景*/
    auto scene_0 = iGame::Scene::New();
    auto scene_1 = iGame::Scene::New();
//    const std::string fileName = "./Models/CP10_L6_DP1_new.odb";
    const std::string fileName = "./Models/Job-1.odb";
    iGame::ODBReader::Pointer reader = iGame::ODBReader::New();
    /* Read */
    auto obj_0 = reader->ReadOdbFirstFrameMesh(fileName);
    auto obj_1 = reader->ReadOdbFirstFrameMesh(fileName, "Step-1");
    /* Check Attribute num is ZERO. */
    scene_0->AddModel(obj_0);
    scene_1->AddModel(obj_1);

    iGame::DynamicCast<iGame::DrawObject>(obj_0)->ViewCloudPicture(scene_0, 0);
    /* Launch window Settings */
    iGame::RenderWindow::Pointer window_0 = iGame::RenderWindow::New();
    window_0->SetScene(scene_0);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene_0);
    interactor->CreateDefaultStyle();
    window_0->SetInteractor(interactor);

    iGame::RenderWindow::Pointer window_1 = iGame::RenderWindow::New();
    window_1->SetScene(scene_1);
    auto interactor_1 = iGame::Interactor::New();
    interactor_1->Initialize(scene_1);
    interactor_1->CreateDefaultStyle();
    window_1->SetInteractor(interactor_1);

    /* show Multi window */
    iGame::MultiRenderWindowManager* manager = iGame::MultiRenderWindowManager::Instance();
    manager->Register(window_0);
    manager->Register(window_1);

    manager->ShowAllRegisterWindow();
}



int main(){
//    TestReadOnlyMesh();
    TestFirstFrameMeshWithFieldData();
    return 0;
}
