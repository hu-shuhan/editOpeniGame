// 迁移回归（待提交）：防止标准过滤器仅有菜单占位，验证本文件记录的算法结果、输入保持不变及边界情况。
#include "Selection/iGameExtractBlockFilter.h"
#include "iGameFileIO.h"
#include "iGameUnstructuredMesh.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace iGame;

static void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
        std::filesystem::create_directories("ExtractionResults");
        for (const auto* file : {"ExtractBlockFlat.vtm", "ExtractBlockNested.vtm"}) {
            auto input = FileIO::ReadFile(std::string("Models/") + file);
            Check(input != nullptr, "read multiblock fixture");
            auto filter = ExtractBlockFilter::New();
            filter->SetInput(input);
            if (std::string(file) == "ExtractBlockFlat.vtm") filter->SetBlockIndex(1);
            else filter->SetBlockPath({1, 0});
            if (!filter->Execute()) throw std::runtime_error(filter->GetLastError());
            auto output = DynamicCast<UnstructuredMesh>(filter->GetOutput());
            Check(output && output->GetNumberOfPoints() == 5 && output->GetNumberOfCells() == 2,
                  "tetrahedron block counts");
            Check(output->GetCellType(0) == IG_TETRA && output->GetCellType(1) == IG_TETRA,
                  "tetrahedron block types");
            Check(FileIO::WriteFile(std::string("ExtractionResults/") + file + ".vtk", output),
                  "write block result");
            filter->SetBlockName("ExtractSelectionTetrahedra");
            if (!filter->Execute()) throw std::runtime_error(filter->GetLastError());
            auto named = DynamicCast<UnstructuredMesh>(filter->GetOutput());
            Check(named && named->GetNumberOfCells() == 2, "name lookup");
            Check(ExtractBlockFilter::GetBlocks(input).size() == 2, "input hierarchy unchanged");
            std::cout << file << ": index/path and name extraction passed; 5 points, 2 cells\n";
        }
        auto mixed = FileIO::ReadFile("Models/ExtractBlockThreeTypes.vtm");
        Check(mixed != nullptr, "read three-type fixture");
        const char* names[]{"Triangles", "Tetrahedra", "Hexahedron"};
        const std::vector<std::vector<int>> paths{{0}, {1, 0}, {1, 1}};
        const IGsize points[]{5, 5, 8}, cells[]{2, 2, 1};
        const IGenum types[]{IG_TRIANGLE, IG_TETRA, IG_HEXAHEDRON};
        for (int i = 0; i < 3; ++i) {
            for (int mode = 0; mode < 2; ++mode) {
                auto filter = ExtractBlockFilter::New();
                filter->SetInput(mixed);
                if (mode == 0) filter->SetBlockPath(paths[i]);
                else filter->SetBlockName(names[i]);
                if (!filter->Execute()) throw std::runtime_error(filter->GetLastError());
                auto result = DynamicCast<UnstructuredMesh>(filter->GetOutput());
                Check(result && result->GetNumberOfPoints() == points[i] &&
                      result->GetNumberOfCells() == cells[i], "three-type counts");
                Check(result->GetCellType(0) == types[i], "three-type topology");
                Check(FileIO::WriteFile(std::string("ExtractionResults/") + names[i] +
                    (mode == 0 ? "-path.vtk" : "-name.vtk"), result), "write three-type output");
            }
        }
        auto deep = FileIO::ReadFile("Models/ExtractBlockDeep.vtm");
        auto deepFilter = ExtractBlockFilter::New();
        deepFilter->SetInput(deep);
        deepFilter->SetBlockPath({1, 0, 0});
        Check(deepFilter->Execute() && deepFilter->GetOutput()->GetName() == "DeepHexahedron_extracted_block",
              "recursive reader preserves XML indices and names");
        deepFilter->SetBlockName("DeepHexahedron");
        Check(deepFilter->Execute(), "recursive named extraction from loaded VTM");
        std::cout << "BlockExtractionExamples: all cases passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "BlockExtractionExamples FAILED: " << error.what() << '\n';
        return 1;
    }
}
