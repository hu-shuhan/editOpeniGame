#include <Fluent/iGameCASReader.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <iostream>

int main() {
    // std::string exePath = "../../ThirdParty/Python/pyFluentLib/cas_converter.exe";
    // std::string casPath = "./Models/room.cas";
    // std::string outputDir = "./Models";
    // fs::path inputPath(casPath);
    // fs::path outputFile = fs::path(outputDir) / (inputPath.stem().string() + ".vtk");
    //
    // std::string arguments = "--input \"" + casPath + "\" --output \"" + outputDir + "\"";
    // std::string fullCommand = "\"" + exePath + "\" " + arguments;
    //
    //
    // int returnCode = system(fullCommand.c_str());
    // if (returnCode == 0) {
    //     std::cout << "Success to  transfer CAS to VTK" << std::endl;
    // } else {
    //     std::cerr << "Error to transfer CAS to VTK" << std::endl;
    // }
    auto casReader = iGame::CASReader::New();
    auto scene = iGame::Scene::New();
    std::string filePath = "./Models/room.cas";
    casReader->SetFilePath(filePath);
    casReader->Execute();
    auto obj = casReader->GetOutput(0);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
    } else {
        scene->AddModel(obj);
    }

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