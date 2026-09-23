#include <MyFilter/iGameCellCenterFilter.h>
#include <iGameFileIO.h>
#include <iGameInteractor.h>
#include <iGameAttributeSet.h>
#include <iGamePointSet.h>
#include <iGameRenderWindow.h>
#include <iGameScene.h>
#include <cmath>
#include <string>

int main() {
    // Read a mesh file and run the CellCenterFilter on it.
    // CellCenter_hexa_grid.vtk：2 层 3x3 格点 → 4 个六面体，
    // 带 float/double 点属性和 double 单元属性。
    // 单元中心坐标已知：(0.5/1.5, 0.5/1.5, 0.5)，可精确校验几何中心；
    // double 属性用于校验输出数组类型/精度保留（createLikeArray）。
    const std::string fileName = "./Models/CellCenter_hexa_grid.vtk";

    auto obj = iGame::FileIO::ReadFile(fileName);
    if (obj == nullptr) {
        std::cout << "Read ERROR: cannot open " << fileName << std::endl;
        return 1;
    }

    // 记录输入的单元数、属性数，用于和输出对账
    auto inPoints = obj->GetPoints();
    auto inCells  = obj->GetCellArray();
    if (inPoints == nullptr || inCells == nullptr) {
        std::cout << "Input data has no points/cells" << std::endl;
        return 1;
    }
    const IGsize inCellNum = inCells->GetNumberOfCells();
    const IGsize inPointNum = inPoints->GetNumberOfPoints();
    const IGsize inAttrNum =
            obj->GetAttributeSet() ? obj->GetAttributeSet()->GetNumberOfAttributes() : 0;

    auto filter = iGame::CellCenterFilter::New();
    filter->SetInput(obj);
    if (!filter->Execute()) {
        std::cout << "CellCenterFilter Execute FAILED" << std::endl;
        return 1;
    }

    auto out = filter->GetOutput();
    auto centerSet = iGame::DynamicCast<iGame::PointSet>(out);
    if (centerSet == nullptr) {
        std::cout << "Output is not a PointSet" << std::endl;
        return 1;
    }

    // Check: one output point per input cell
    const IGsize outPointNum = centerSet->GetNumberOfPoints();
    std::cout << "Input cells:  " << inCellNum << std::endl;
    std::cout << "Output points: " << outPointNum << std::endl;
    if (outPointNum != inCellNum) {
        std::cout << "FAIL: output point count != input cell count" << std::endl;
        return 1;
    }
    std::cout << "PASS: one output point per input cell" << std::endl;

    // 校验几何中心坐标：4 个六面体中心应为
    // (0.5,0.5,0.5) (1.5,0.5,0.5) (0.5,1.5,0.5) (1.5,1.5,0.5)
    if (inCellNum == 4) {
        const double expect[4][3] = {
            {0.5, 0.5, 0.5}, {1.5, 0.5, 0.5},
            {0.5, 1.5, 0.5}, {1.5, 1.5, 0.5}};
        for (IGsize c = 0; c < 4; c++) {
            auto p = centerSet->GetPoint(c);
            for (int d = 0; d < 3; d++) {
                if (std::fabs(p[d] - expect[c][d]) > 1e-5) {
                    std::cout << "FAIL: center[" << c << "] coord[" << d
                              << "]=" << p[d] << " expected " << expect[c][d] << std::endl;
                    return 1;
                }
            }
        }
        std::cout << "PASS: cell centers match expected coordinates" << std::endl;
    }

    // 校验属性：属性总数应被保留（点属性插值后仍在，单元属性原样保留）
    const IGsize outAttrNum =
            out->GetAttributeSet() ? out->GetAttributeSet()->GetNumberOfAttributes() : 0;
    std::cout << "Input attributes:  " << inAttrNum << std::endl;
    std::cout << "Output attributes: " << outAttrNum << std::endl;
    if (outAttrNum != inAttrNum) {
        std::cout << "FAIL: attribute count changed" << std::endl;
        return 1;
    }

    // 校验每个输出属性的数组长度：
    //   点属性 → 插值后长度 = 单元数（中心点数）
    //   单元属性 → 原样保留，长度 = 单元数
    auto allAttrs = out->GetAttributeSet()->GetAllAttributes();
    for (IGsize i = 0; i < allAttrs->GetNumberOfElements(); i++) {
        auto& attr = allAttrs->GetElement(i);
        const IGsize elemNum = attr.pointer->GetNumberOfElements();
        std::cout << "  attr[" << i << "] name=" << attr.pointer->GetName()
                  << " attachment=" << attr.attachmentType
                  << " arrayType=" << attr.pointer->GetArrayType()
                  << " elements=" << elemNum << std::endl;
        if (elemNum != outPointNum) {
            std::cout << "FAIL: attribute length != output point count" << std::endl;
            return 1;
        }
        // double 输入属性（pid_double / cid）输出后仍应为 DoubleArray，精度不丢失
        std::string an = attr.pointer->GetName();
        if ((an == "pid_double" || an == "cid") && attr.pointer->GetArrayType() != IG_DoubleArray) {
            std::cout << "FAIL: double attribute " << an
                      << " lost its type after filter" << std::endl;
            return 1;
        }
    }

    std::cout << "PASS: all attribute lengths match output points" << std::endl;
    std::cout << "PASS: CellCenterFilter test finished" << std::endl;

    // Show the result in a render window
    // PointSet 默认视图样式是"填充面"，但没有面单元，必须切到 IG_POINTS 才能显示点云；
    // 顺带把点调大一点，方便观察。
    auto drawObj = iGame::DynamicCast<iGame::DrawObject>(out);
    if (drawObj) {
        drawObj->SetViewStyle(IG_POINTS);
        drawObj->SetPointSize(3.0f);
    }

    auto scene = iGame::Scene::New();
    scene->AddModel(out);

    iGame::RenderWindow::Pointer window = iGame::RenderWindow::New();
    window->SetSize(1280, 720);
    window->SetScene(scene);

    auto interactor = iGame::Interactor::New();
    interactor->Initialize(scene);
    interactor->CreateDefaultStyle();
    window->SetInteractor(interactor);

    window->Show();
    return 0;
}
