// 迁移回归（待提交）：覆盖标准过滤器的算法/界面行为及源测试中的边界条件，防止仅迁移菜单占位而遗漏实现。
// Regression: the old example used Tet_Plane and could not be checked against
// ParaView's Extract Cells By Region boundary behavior. The trigger is a cell
// crossing the Box/Sphere boundary, followed by a parameter change and Apply.
// These cases guarantee matching input cell IDs, compact output topology, and
// preservation of point/cell arrays. Boundary-only and empty regions matter
// because they expose strict/intersected and stale-preview mistakes.
// Fix and test first land together in the commit titled
// "feat: integrate region extraction with latest main". Find its real SHA with:
// git log --diff-filter=A --format="%h %s" -- Examples/Filter/Selection/TestExtractCellsByRegion.cpp
#include <Selection/iGameExtractCellsByRegionFilter.h>
#include <iGameAttributeSet.h>
#include <iGameFileIO.h>
#include <iostream>
#include <cmath>
#include <vector>

// Run with Examples (or the examples build directory containing Models) as cwd.
namespace {
bool CheckAttributes(iGame::UnstructuredMesh::Pointer output, const std::vector<int>& expectedIds) {
    auto attributes = output.IsNull() ? nullptr : output->GetAttributeSet();
    if (attributes == nullptr) return false;
    const auto& pointAttribute = attributes->GetScalar("PreviewPointValue");
    const auto& cellAttribute = attributes->GetScalar("MaterialId");
    if (pointAttribute.IsNone() || cellAttribute.IsNone() || pointAttribute.attachmentType != IG_POINT ||
        cellAttribute.attachmentType != IG_CELL || pointAttribute.pointer->GetArrayType() != IG_FloatArray ||
        cellAttribute.pointer->GetArrayType() != IG_IntArray) return false;
    if (pointAttribute.pointer->GetNumberOfElements() != output->GetNumberOfPoints() ||
        cellAttribute.pointer->GetNumberOfElements() != output->GetNumberOfCells()) return false;

    double value[1]{};
    for (int cell = 0; cell < static_cast<int>(expectedIds.size()); ++cell) {
        cellAttribute.pointer->GetElement(cell, value);
        if (value[0] != 1000 + expectedIds[cell]) return false;
        for (int point = 0; point < 4; ++point) {
            pointAttribute.pointer->GetElement(cell * 4 + point, value);
            if (value[0] != 100 + expectedIds[cell] * 4 + point) return false;
        }
    }
    return true;
}

bool Check(iGame::UnstructuredMesh::Pointer mesh, bool sphere, bool strict,
           const std::vector<int>& expectedIds, const char* label, bool empty = false) {
    auto filter = iGame::ExtractCellsByRegionFilter::New();
    if (sphere) filter->SetSphere(iGame::Vector3d(0, 0, 0), 1.0);
    else if (empty) filter->SetBox(iGame::Vector3d(10, 10, 10), iGame::Vector3d(11, 11, 11));
    else filter->SetBox(iGame::Vector3d(-1, -1, -1), iGame::Vector3d(1, 1, 1));
    filter->SetRequireAllPoints(strict);
    filter->SetInput(0, mesh);

    if (!filter->Preview() || filter->GetSelectedCellIds() != expectedIds) {
        std::cerr << "FAIL " << label << ": preview IDs do not match expected selection\n";
        return false;
    }
    if (!filter->Execute()) {
        std::cerr << "FAIL " << label << ": Execute failed\n";
        return false;
    }
    auto out = iGame::DynamicCast<iGame::UnstructuredMesh>(filter->GetOutput());
    // Each fixture cell has four private points; extraction must compact them.
    const int expected = static_cast<int>(expectedIds.size());
    const bool ok = !out.IsNull() && filter->GetSelectedCellIds() == expectedIds
                    && out->GetNumberOfCells() == expected && out->GetNumberOfPoints() == expected * 4;
    std::cout << (ok ? "PASS " : "FAIL ") << label << " expected IDs=";
    for (const int id : expectedIds) std::cout << id << ',';
    std::cout << " actual cells=" << (out.IsNull() ? -1 : static_cast<int>(out->GetNumberOfCells())) << '\n';
    const bool attributesOk = expectedIds.empty() || CheckAttributes(out, expectedIds);
    std::cout << (attributesOk ? "PASS " : "FAIL ") << label
              << (expectedIds.empty() ? " empty output has no attributes to copy\n" : " attributes preserved\n");
    return ok && attributesOk;
}

bool Run(const char* path, int inputCells, const std::vector<int>& boxStrict,
         const std::vector<int>& boxLoose) {
    auto obj = iGame::FileIO::ReadFile(path);
    auto mesh = iGame::DynamicCast<iGame::UnstructuredMesh>(obj);
    if (mesh.IsNull() || mesh->GetNumberOfCells() != inputCells
        || mesh->GetNumberOfPoints() != inputCells * 4) {
        std::cerr << "FAIL reading fixture " << path << ". Run from Examples.\n";
        return false;
    }
    if (mesh->GetAttributeSet()->GetScalar("PreviewPointValue").IsNone() ||
        mesh->GetAttributeSet()->GetScalar("MaterialId").IsNone()) {
        std::cerr << "FAIL fixture arrays are missing: " << path << '\n';
        return false;
    }
    std::cout << "Model: " << path << '\n';
    bool ok = true;
    ok = Check(mesh, false, true, boxStrict, "Box strict") && ok;
    ok = Check(mesh, false, false, boxLoose, "Box loose") && ok;
    ok = Check(mesh, true, true, {0}, "Sphere strict") && ok;
    ok = Check(mesh, true, false, {0, 1, 3}, "Sphere loose (includes boundary)") && ok;
    ok = Check(mesh, false, true, {}, "Empty region", true) && ok;

    // Reuse the same filter after changing the region: this is the code path used by the GUI's live preview.
    auto adjustable = iGame::ExtractCellsByRegionFilter::New();
    adjustable->SetInput(0, mesh);
    adjustable->SetBox(iGame::Vector3d(-1, -1, -1), iGame::Vector3d(1, 1, 1));
    adjustable->SetRequireAllPoints(false);
    const bool firstPreview = adjustable->Preview() && adjustable->GetSelectedCellIds() == boxLoose;
    adjustable->SetSphere(iGame::Vector3d(0, 0, 0), 1.0);
    adjustable->SetRequireAllPoints(true);
    const bool adjustedPreview = adjustable->Preview() && adjustable->GetSelectedCellIds() == std::vector<int>{0};
    const bool adjustedOutput = adjustable->Execute() &&
            adjustable->GetSelectedCellIds() == std::vector<int>{0} &&
            CheckAttributes(iGame::DynamicCast<iGame::UnstructuredMesh>(adjustable->GetOutput()), {0});
    std::cout << ((firstPreview && adjustedPreview && adjustedOutput) ? "PASS " : "FAIL ")
              << "Preview and Apply update after changing region parameters\n";
    ok = firstPreview && adjustedPreview && adjustedOutput && ok;
    adjustable->SetSphere(iGame::Vector3d(0, 0, 0), 0.0);
    const bool staleCleared = !adjustable->Preview() && adjustable->GetSelectedCellIds().empty();
    std::cout << (staleCleared ? "PASS " : "FAIL ") << "Invalid preview clears old cell IDs\n";
    ok = staleCleared && ok;

    auto invalid = iGame::ExtractCellsByRegionFilter::New();
    invalid->SetInput(0, mesh);
    invalid->SetBox(iGame::Vector3d(1, -1, -1), iGame::Vector3d(-1, 1, 1));
    bool rejected = !invalid->Execute();
    std::cout << (rejected ? "PASS " : "FAIL ") << "Invalid box rejected\n";
    ok = rejected && ok;
    for (double radius : {0.0, -1.0}) {
        invalid->SetSphere(iGame::Vector3d(0, 0, 0), radius);
        rejected = !invalid->Execute();
        std::cout << (rejected ? "PASS " : "FAIL ") << "Invalid sphere radius=" << radius << '\n';
        ok = rejected && ok;
    }
    return ok;
}
} // namespace

// Regression: remapping polyhedron counts as point IDs corrupts topology;
// streams longer than 256 also overflowed the fixed buffer. This case forces
// point compaction and checks both Box/Sphere with an 80-sided prism.
// Fix: "feat: finalize region controls and polyhedron extraction" (same commit).
// A polyhedron face stream contains counts as well as point IDs. Its stream
// also commonly exceeds the fixed size used by tetrahedra/hexahedra.
bool CheckPolyhedronFaceStream() {
    using namespace iGame;
    auto mesh = UnstructuredMesh::New();
    auto points = Points::New();
    points->AddPoint(Vector3d(99,99,99)); // unused point forces ID compaction
    constexpr int sides = 80;
    for (int layer = 0; layer < 2; ++layer) {
        for (int i = 0; i < sides; ++i) {
            const double angle = i * 6.283185307179586 / sides;
            points->AddPoint(Vector3d(std::cos(angle),std::sin(angle),layer));
        }
    }
    mesh->SetPoints(points);
    std::vector<igIndex> stream{sides+2, sides};
    for (int i = sides-1; i >= 0; --i) stream.push_back(i+1);
    stream.push_back(sides);
    for (int i = 0; i < sides; ++i) stream.push_back(sides+i+1);
    for (int i = 0; i < sides; ++i) {
        const int next = (i+1)%sides;
        for (int value : {4,i+1,next+1,sides+next+1,sides+i+1}) stream.push_back(value);
    }
    mesh->AddCell(stream.data(),static_cast<int>(stream.size()),IG_POLYHEDRON);
    auto filter = ExtractCellsByRegionFilter::New();
    filter->SetInput(0,mesh);
    bool ok = stream.size()>IGAME_CELL_MAX_SIZE;
    for (bool sphere : {false,true}) {
        if (sphere) filter->SetSphere(Vector3d(0,0,.5),2);
        else filter->SetBox(Vector3d(-2,-2,-2),Vector3d(2,2,2));
        ok = filter->Execute() && ok;
        auto out = DynamicCast<UnstructuredMesh>(filter->GetOutput());
        if (out.IsNull() || out->GetNumberOfCells()!=1 || out->GetNumberOfPoints()!=2*sides) return false;
        const igIndex* result = nullptr;
        const int n = out->GetCellPointIds(0,result);
        if (n!=stream.size() || result[0]!=sides+2) return false;
        int offset = 1;
        for (int face = 0; face < sides+2; ++face) {
            const int count = stream[offset];
            if (result[offset++]!=count) return false;
            for (int j = 0; j < count; ++j,++offset) {
                if (result[offset]<0 || result[offset]>=out->GetNumberOfPoints()) return false;
                if ((points->GetPoint(stream[offset])-out->GetPoints()->GetPoint(result[offset])).squaredNorm()>1e-12) return false;
            }
        }
    }
    std::cout << (ok ? "PASS " : "FAIL ") << "Polyhedron face counts and large stream preserved\n";
    return ok;
}

int main() {
    bool ok = Run("./Models/ExtractCellsByRegion_BoxCases.vtk", 4, {0}, {0, 1, 3});
    ok = Run("./Models/ExtractCellsByRegion_SphereCases.vtk", 5, {0, 4}, {0, 1, 3, 4}) && ok;
    ok = CheckPolyhedronFaceStream() && ok;
    std::cout << (ok ? "ALL TESTS PASSED\n" : "TESTS FAILED\n");
    return ok ? 0 : 1;
}
