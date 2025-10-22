#include <iGameScene.h>
#include <iGameFileIO.h>
#include <iGameRenderWindow.h>

int main(){
    std::string exePath = "nastran_to_vtk_cli.exe";
    std::string outputPath = "./Models/NastranToVtu_ogs.vtu";
    std::string bdfPath = "./Models/ogs.bdf";
    std::string op2Path = "./Models/ogs.op2";

    std::string arguments = "--bdf " + bdfPath + " --op2 " + op2Path + " --output " + outputPath;



    std::string fullCommand = "\"" + exePath + "\" " + arguments;

    int returnCode = system(fullCommand.c_str());

    if (returnCode == 0) {
        std::cout << "Success to  transfer Nastran to VTK��" << std::endl;
    } else {
        std::cerr << "Error to transfer Nastran to VTK" << std::endl;
    }


    auto scene = iGame::Scene::New();
    /* Test CGNS File Reader's output. Add it into Scene*/
    const std::string fileName = outputPath;
    auto obj = iGame::FileIO::ReadFile(fileName);

    std::cout << "Attributes Num : " << obj->GetAttributeSet()->GetNumberOfAttributes() << std::endl;

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

    return 0;
}