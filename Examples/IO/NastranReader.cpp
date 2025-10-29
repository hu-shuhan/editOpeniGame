#include <iGameScene.h>
#include <iGameFileIO.h>
#include <iGameRenderWindow.h>
#include <Nastran/iGameNastranReader.h>
int main(){
    std::string bdfPath = "./Models/ogs.bdf";
    std::string op2Path = "./Models/ogs.op2";
    iGame::NastranReader::Pointer rd = iGame::NastranReader::New();
    rd->SetBDFFileName(bdfPath);
    /* Optional, the op2 file path can be set to read physical field data*/
    rd->SetOP2FileName(op2Path);
    rd->Execute();
    auto obj = rd->GetOutput();
    std::cout << "Attributes Num : " << obj->GetAttributeSet()->GetNumberOfAttributes() << std::endl;
    if (obj == nullptr) {
        std::cerr << "Error: Failed to load model" << std::endl;
        return -1;
    }
    /* Launch window Settings */
    auto scene = iGame::Scene::New();
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);
    scene->AddModel(obj);
    /* show single window */
    window->Show();
    return 0;
}