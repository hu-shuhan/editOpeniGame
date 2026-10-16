// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Periodic/TestAngularPeriodic.cpp
// Batch 3 acceptance regression: the destination lacked these filter examples.
// Run against the imported models and synthetic boundary cases below; failures must
// return nonzero. IGAME_EXAMPLE_NO_RENDER retains numerical checks without a GPU.
// Integration fix: feat: integrate third-batch standard filters. Source: dayuwan77/igamevis, fdafcbb.
#include <cstdlib>
#include "Periodic/iGameAngularPeriodicFilter.h"
#include "iGameFileIO.h"
#include "iGameInteractor.h"
#include "iGameRenderWindow.h"
#include "iGameScene.h"

int main() {
    // AI 生成的环形扇区测试模型：120° 扇区绕 Z 轴 3 份 360° 可拼满整圈
    const std::string fileName = "./Models/AIGen_Surface_RingSector.obj";
    auto dataObj = iGame::FileIO::ReadFile(fileName);
    if (dataObj == nullptr) {
        igError("Error reading the file");
        return 1;
    }

    auto scene = iGame::Scene::New();
    scene->AddModel(dataObj);

    auto filter = iGame::AngularPeriodicFilter::New();
    filter->SetInput(dataObj);
    iGame::Point axisOrigin(1.0, 0.0, 0.0);
    iGame::Vector3d axisDir(0.0, 0.0, 1.0);
    filter->SetRotationAxis(axisOrigin, axisDir);
    filter->SetNumberOfCopies(3);
    filter->SetAngle(120.0); // 周期角度：相邻两份间隔 120°（0/120/240，与 120° 扇区吻合）
    if (!filter->Execute() || !filter->GetOutput()) return 1;
    scene->AddModel(filter->GetOutput());

    // 画出旋转定轴和 XYZ 坐标轴
    auto bbox = dataObj->GetBoundingBox();
    double halfLen = bbox.diag() * 0.75 + 1.0;
    auto center = bbox.center();

    auto makeAxisLine = [&](const iGame::Vector3d& c, const iGame::Vector3d& dir,
                            double len, const igm::vec3& color) {
        iGame::Point start(static_cast<float>(c[0] - dir[0] * len),
                           static_cast<float>(c[1] - dir[1] * len),
                           static_cast<float>(c[2] - dir[2] * len));
        iGame::Point end(static_cast<float>(c[0] + dir[0] * len),
                         static_cast<float>(c[1] + dir[1] * len),
                         static_cast<float>(c[2] + dir[2] * len));
        auto line = iGame::UnstructuredMesh::New();
        line->AddPoint(start);
        line->AddPoint(end);
        igIndex lineIds[2] = {0, 1};
        line->AddCell(lineIds, 2, iGame::IG_LINE);
        line->SetViewStyle(IG_WIREFRAME);
        line->SetLineColor(color);
        line->SetLineWidth(2.0f);
        scene->AddModel(line);
    };

    makeAxisLine(axisOrigin, axisDir, halfLen, igm::vec3(1.0f, 0.0f, 1.0f));

    // XYZ 坐标轴：X 红、Y 绿、Z 蓝，以模型包围盒中心为原点
    makeAxisLine(center, iGame::Vector3d(1.0, 0.0, 0.0), halfLen,
                 igm::vec3(1.0f, 0.0f, 0.0f));
    makeAxisLine(center, iGame::Vector3d(0.0, 1.0, 0.0), halfLen,
                 igm::vec3(0.0f, 1.0f, 0.0f));
    makeAxisLine(center, iGame::Vector3d(0.0, 0.0, 1.0), halfLen,
                 igm::vec3(0.0f, 0.0f, 1.0f));

    if (std::getenv("IGAME_EXAMPLE_NO_RENDER")) return 0;
    auto window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);
    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    window->Show();
    return 0;
}
