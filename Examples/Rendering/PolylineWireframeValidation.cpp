// BUG (2026-10-15): DecimatePolyline_Curve.vtk stores 151 points in one
// POLYDATA LINES cell. SurfaceMesh wireframe displayed only its first segment.
// Verify all adjacent segments, independent/closed/long lines, both clipping
// branches, ordinary polygon edges and lazy surface-only geometry. Degenerate
// and invalid segments must not emit invalid GPU indices.
// 修复提交：与本测试首次加入的提交相同，主题为：
// fix(rendering): connect all points in polyline wireframes
// 查询提交号：git log --diff-filter=A --format="%h %s" -- Examples/Rendering/PolylineWireframeValidation.cpp
#include <VTK/iGameVTKReader.h>
#include <iGameSurfaceMesh.h>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
using namespace iGame;
void Require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

void Check(SurfaceMesh::Pointer mesh, const std::vector<iguIndex>& expected) {
    auto positions = FloatArray::New();
    auto lines = UnsignedIntArray::New();
    auto triangles = UnsignedIntArray::New();
    auto masks = UnsignedCharArray::New();
    mesh->SetViewStyle(IG_WIREFRAME);
    for (bool clipping : {false, true}) {
        mesh->GetClipper()->m_Box.m_Use = clipping; // Unbounded box: all points visible.
        mesh->GetDrawableArray(positions, lines, triangles, masks);
        if (lines->GetNumberOfValues() != expected.size()) {
            throw std::runtime_error("wireframe has " +
                std::to_string(lines->GetNumberOfValues() / 2) + " segments, expected " +
                std::to_string(expected.size() / 2));
        }
        for (std::size_t i = 0; i < expected.size(); ++i)
            Require(lines->GetValue(i) == expected[i], "wireframe connected wrong endpoints");
    }
    mesh->GetClipper()->DisableAll();
    mesh->SetRemoteRenderingEnabled(true);
    mesh->SetViewStyle(IG_SURFACE);
    mesh->GetDrawableArray(positions, lines, triangles, masks);
    Require(lines->GetNumberOfValues() == 0, "remote surface-only mode built unnecessary wireframe");
    mesh->SetRemoteRenderingEnabled(false);
}

std::vector<iguIndex> Chain(int count) {
    std::vector<iguIndex> result;
    for (int i = 1; i < count; ++i) {
        result.push_back(i - 1);
        result.push_back(i);
    }
    return result;
}

SurfaceMesh::Pointer ReadCurve(const char* path) {
    auto reader = VTKReader::New();
    std::ostringstream vtk;
    if (path) {
        reader->SetFilePath(path);
    } else {
        vtk << "# vtk DataFile Version 3.0\nPolyline regression\nASCII\nDATASET POLYDATA\nPOINTS 151 float\n";
        for (int i = 0; i < 151; ++i) vtk << i << " 0 0\n";
        vtk << "LINES 1 152\n151";
        for (int i = 0; i < 151; ++i) vtk << ' ' << i;
        vtk << '\n';
    }
    const std::string contents = vtk.str();
    if (!path) reader->SetMemoryBuffer(contents.data(), contents.size());
    Require(reader->Execute(), "VTK reader failed");
    auto mesh = DynamicCast<SurfaceMesh>(reader->GetOutput());
    Require(mesh && mesh->GetNumberOfPoints() == 151, "curve points were lost");
    Require(mesh->GetEdges()->GetNumberOfCells() == 1 &&
            mesh->GetEdges()->GetCellSize(0) == 151, "reader split or truncated the polyline");
    return mesh;
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc > 1) Check(ReadCurve(argv[1]), Chain(151));
        Check(ReadCurve(nullptr), Chain(151));
        auto mesh = SurfaceMesh::New();
        auto points = Points::New();
        for (int i = 0; i < 600; ++i) points->AddPoint(i, 0, 0);
        mesh->SetPoints(points);
        mesh->SetFaces(CellArray::New());
        auto edges = CellArray::New();
        mesh->SetEdges(edges);
        std::vector<igIndex> longLine(600);
        std::iota(longLine.begin(), longLine.end(), 0);
        edges->AddCellIds(longLine.data(), static_cast<int>(longLine.size()));
        Check(mesh, Chain(600));

        edges = CellArray::New();
        mesh->SetEdges(edges);
        igIndex line[] = {0, 1}, open[] = {2, 3, 4}, closed[] = {5, 6, 7, 5};
        edges->AddCellIds(line, 2);
        edges->AddCellIds(open, 3);
        edges->AddCellIds(closed, 4);
        Check(mesh, {0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 7, 5});

        edges = CellArray::New();
        mesh->SetEdges(edges);
        edges->AddCellIds(line, 0);
        edges->AddCellIds(line, 1);
        igIndex invalid[] = {0, -1, 2, 600, 4, 5};
        edges->AddCellIds(invalid, 6);
        Check(mesh, {4, 5});

        mesh->SetEdges(nullptr);
        igIndex quad[] = {0, 1, 2, 3};
        mesh->GetFaces()->AddCellIds(quad, 4);
        Check(mesh, {0, 1, 1, 2, 2, 3, 3, 0});
        std::cout << "PASS: 151-point VTK curve -> 150 segments; long/multiple/closed/invalid lines, clipping and polygon edges\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
