#include <iGameScene.h>
#include <iGameFileIO.h>
#include <iGameRenderWindow.h>
#include <Nastran/iGameNastranReader.h>
int main(int argc, char* argv[]){
    std::string bdfPath = argc > 1 ? argv[1] : "./Models/ogs.bdf";
    std::string op2Path = argc > 2 ? argv[2] : "./Models/ogs.op2";
    const bool noWindow = argc > 3 && std::string(argv[3]) == "--no-window";
    iGame::NastranReader::Pointer rd = iGame::NastranReader::New();
    /* It is necessary, equivalent to SetFilePath */
    rd->SetBDFFileName(bdfPath);
//    rd->SetFilePath(bdfPath);
    /* Optional, the op2 file path can be set to read physical field data*/
    rd->SetOP2FileName(op2Path);
    if (!rd->Execute()) {
        std::cerr << "Error: NastranReader::Execute failed" << std::endl;
        return -1;
    }
    auto obj = rd->GetOutput();
    if (obj == nullptr) {
        std::cerr << "Error: Failed to load model" << std::endl;
        return -1;
    }
    auto attributes = obj->GetAttributeSet();
    std::cout << "Attributes Num : " << (attributes ? attributes->GetNumberOfAttributes() : 0) << std::endl;
    if (noWindow) {
        return 0;
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
