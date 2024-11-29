#include <Abaqus/iGameODBReader.h>
#include <iGameRenderWindow.h>
#include <iGameInteractor.h>
int main(){
    /* 创建场景*/
    auto scene = iGame::Scene::New();
    const std::string fileName = "C:\\Users\\m_ky\\Desktop\\Resource\\machineHand\\otherOdb\\CP10_L6_DP1_new.odb";
    iGame::ODBReader::Pointer reader = iGame::ODBReader::New();
    auto obj = reader->ReadOdbFirstFrameMesh(fileName);
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

    return 0;
}
