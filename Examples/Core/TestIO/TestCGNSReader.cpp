//
// Created by m_ky on 2024/11/26.
//

/**
 * @class   TestCGNSReader
 * @brief   TestCGNSReader's brief
 */
#include <CGNS/iGameCGNSReader.h>
#include <iGameRenderWindow.h>
#include <iGameInteractor.h>
void TestCGNSReader(){

    /* 创建场景*/
    auto scene = iGame::Scene::New();
    /* Test CGNS File Reader's output. Add it into Scene*/
    const std::string fileName = "./Models/bump.cgns";
    iGame::iGameCGNSReader::Pointer reader = iGame::iGameCGNSReader::New();
    auto obj = reader->ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR!\n";
    } else {
        scene->AddModel(obj);
    }
    /* Launch window Settings */
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1920, 1080);
    window->SetScene(scene);
    auto basicInteractor = iGame::Interactor::New();
    basicInteractor->Initialize(scene);
    window->SetInteractor(basicInteractor);

    /* show single window */
    window->Show();
}

int main(){
    TestCGNSReader();

    return 0;
}
