#include <iGameDrawObject.h>
#include <iGameScene.h>
#include <iGameSurfaceMesh.h>

#include <iostream>

int main() {
    auto scene = iGame::Scene::New();
    int updateCount = 0;
    scene->SetUpdateFunctor([&updateCount]() { ++updateCount; });

    auto root = iGame::DrawObject::New();
    auto surface = iGame::SurfaceMesh::New();
    auto values = iGame::FloatArray::New();
    values->SetName("scalar");
    values->SetDimension(1);
    values->AddValue(1.0f);
    root->GetAttributeSet()->AddScalar(IG_POINT, values);
    surface->GetAttributeSet()->AddScalar(IG_POINT, values);
    root->SetRenderableObject(surface);

    if (!root->ViewCloudPicture(scene, 0, 0) || updateCount != 1) {
        std::cerr << "attribute view must flush the scene once\n";
        return 1;
    }
    if (root->ViewCloudPicture(scene, 0, 0) || updateCount != 1) {
        std::cerr << "repeated attribute view must not flush the scene\n";
        return 2;
    }
    if (!root->ViewCloudPicture(scene, -1, -1, false) || updateCount != 1) {
        std::cerr << "deferred attribute view must not flush the scene\n";
        return 3;
    }
    scene->Update();
    if (updateCount != 2) {
        std::cerr << "deferred attribute view must allow one outer flush\n";
        return 4;
    }

    std::cout << "attribute view update batching passed\n";
    return 0;
}
