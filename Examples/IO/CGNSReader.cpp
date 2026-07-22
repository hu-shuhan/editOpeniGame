//
// Created by m_ky on 2024/11/26.
//

/**
 * @class   TestCGNSReader
 * @brief   TestCGNSReader's brief
 */
#include <CGNS/iGameCGNSReader.h>
#include <iGameInteractor.h>
#include <iGameRenderWindow.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

void TestCGNSReader() {
    std::cerr << "[testCGNS] cwd=" << fs::current_path().string() << "\n"
              << std::flush;

    const std::string fileName = "./Models/F6-coarse-vol-v2.cgns";
    std::cerr << "[testCGNS] file=" << fileName
              << " exists=" << fs::exists(fileName) << "\n"
              << std::flush;
    if (!fs::exists(fileName)) {
        std::cerr << "[testCGNS] FAIL: model not found (check Working directory)\n"
                  << std::flush;
        return;
    }

    std::cerr << "[testCGNS] CGNS ReadFile begin\n" << std::flush;
    iGame::iGameCGNSReader::Pointer reader = iGame::iGameCGNSReader::New();
    auto obj = reader->ReadFile(fileName);
    if (obj == nullptr) {
        std::cerr << "[testCGNS] FAIL: ReadFile returned null\n" << std::flush;
        return;
    }
    std::cerr << "[testCGNS] ReadFile OK\n" << std::flush;

    std::cerr << "[testCGNS] Scene::New begin\n" << std::flush;
    auto scene = iGame::Scene::New();
    std::cerr << "[testCGNS] Scene::New OK\n" << std::flush;
    scene->AddModel(obj);
    std::cerr << "[testCGNS] AddModel OK\n" << std::flush;

    std::cerr << "[testCGNS] RenderWindow begin\n" << std::flush;
    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    std::cerr << "[testCGNS] SetScene begin\n" << std::flush;
    window->SetScene(scene);
    std::cerr << "[testCGNS] SetScene OK\n" << std::flush;

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    std::cerr << "[testCGNS] Show() — close window to exit\n" << std::flush;
    window->Show();
    std::cerr << "[testCGNS] Show() returned\n" << std::flush;
}

int main() {
    TestCGNSReader();
    return 0;
}
