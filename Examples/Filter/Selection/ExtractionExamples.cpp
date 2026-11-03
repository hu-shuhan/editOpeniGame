// 迁移回归（待提交）：防止标准过滤器仅有菜单占位，验证本文件记录的算法结果、输入保持不变及边界情况。
#include "Selection/igameextractselectionfilter.h"
#include "iGameFileIO.h"
#include "iGameSelection.h"
#include "iGameUnstructuredMesh.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace iGame;
static void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
int main() {
    try {
        auto triangles = DynamicCast<PointSet>(FileIO::ReadFile("Models/ExtractSelectionTriangles.vtk"));
        Check(triangles && triangles->GetNumberOfPoints() == 5, "read triangle fixture");
        triangles->GetSelection()->SelectionCallBackEvent(IG_CELL, std::vector<igIndex>{0}, Selection::Add);
        auto selection = ExtractSelectionFilter::New();
        selection->SetInput(triangles);
        selection->SetSelectionType(IG_CELL);
        Check(selection->Execute(), "extract one triangle");
        auto selected = DynamicCast<UnstructuredMesh>(selection->GetOutput());
        Check(selected && selected->GetNumberOfCells() == 1 && selected->GetNumberOfPoints() == 3,
              "one triangle has three points");
        std::filesystem::create_directories("ExtractionResults");
        Check(FileIO::WriteFile("ExtractionResults/selected_triangle.vtk", selected), "write triangle result");
        auto tetrahedra = DynamicCast<PointSet>(FileIO::ReadFile("Models/ExtractSelectionTetrahedra.vtk"));
        Check(tetrahedra && tetrahedra->GetNumberOfPoints() == 5, "read tetrahedra fixture");
        tetrahedra->GetSelection()->SelectionCallBackEvent(IG_CELL, std::vector<igIndex>{0, 1}, Selection::Add);
        selection->SetInput(tetrahedra);
        Check(selection->Execute(), "extract tetrahedra");
        selected = DynamicCast<UnstructuredMesh>(selection->GetOutput());
        Check(selected && selected->GetNumberOfCells() == 2 && selected->GetNumberOfPoints() == 5, "shared tetrahedron points");
        Check(FileIO::WriteFile("ExtractionResults/selected_tetrahedra.vtk", selected), "write tetrahedra result");
        std::cout << "ExtractionExamples: selection cases passed; results in ExtractionResults\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ExtractionExamples FAILED: " << e.what() << '\n';
        return 1;
    }
}
