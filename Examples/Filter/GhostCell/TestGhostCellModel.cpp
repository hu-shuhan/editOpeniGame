// 迁移回归（待提交）：防止标准过滤器仅有菜单占位，验证本文件记录的算法结果、输入保持不变及边界情况。
// Existing example updated for the partition-generating Ghost Cells behavior.
// Regression: the former filter only wrote custom GhostCells flags and could
// not exchange cells or feed Remove Ghost Information. Fix commit: 待提交.
#include "GhostCell/iGameGhostCellFilter.h"
#include "RemoveGhostInformation/iGameRemoveGhostInformationFilter.h"
#include "Selection/iGameExtractBlockFilter.h"
#include "iGameUnstructuredMesh.h"
#include <iostream>
using namespace iGame;
int main() {
    auto root = DrawObject::New();
    for (int part = 0; part < 2; ++part) {
        auto mesh = UnstructuredMesh::New();
        mesh->SetName("partition_" + std::to_string(part));
        for (auto p : {Point(0,0,0), Point(1,0,0), Point(0,1,0), Point(0,0,part ? -1 : 1)})
            mesh->GetPoints()->AddPoint(p);
        igIndex ids[]{0,1,2,3}; mesh->AddCell(ids,4,IG_TETRA);
        root->AddSubDataObject(mesh);
    }
    auto ghost = GhostCellFilter::New(); ghost->SetInput(root); ghost->SetNumberOfGhostLayers(1);
    if (!ghost->Execute()) { std::cerr << ghost->GetLastError() << '\n'; return 1; }
    auto blocks = ExtractBlockFilter::GetBlocks(ghost->GetOutput());
    if (blocks.size() != 2) return 2;
    for (auto block : blocks) {
        auto mesh = DynamicCast<UnstructuredMesh>(block);
        if (!mesh || mesh->GetNumberOfCells() != 2 || mesh->GetNumberOfPoints() != 5) return 3;
    }
    auto remove = RemoveGhostInformationFilter::New(); remove->SetInput(ghost->GetOutput());
    if (!remove->Execute()) return 4;
    for (auto block : ExtractBlockFilter::GetBlocks(remove->GetOutput())) {
        auto mesh = DynamicCast<UnstructuredMesh>(block);
        if (!mesh || mesh->GetNumberOfCells() != 1 || mesh->GetNumberOfPoints() != 4) return 5;
    }
    std::cout << "PASS: two tetrahedral partitions exchange ghosts and remove back to native cells\n";
    return 0;
}
