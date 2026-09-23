// BUG (2026-09-19): PR #4's zero-cell point-cloud fallback was not in main.
// Coordinates without cells kept surface-only rendering and could be converted
// to an empty SurfaceMesh. Verify geometry, point colors, existing shells,
// hidden display modes, empty inputs and real/mixed topology separately.
// 修复提交：与本测试首次加入的提交相同，主题为：
// fix: port remaining stable-sdk fixes before closing PR #4
// 查询提交号：git log --diff-filter=A --format="%h %s" -- Examples/Rendering/ZeroCellPointCloudValidation.cpp
#include <iGameUnstructuredMesh.h>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using namespace iGame;
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Probe : public UnstructuredMesh {
public:
    I_OBJECT(Probe);
    static Pointer New() { return new Probe; }
    void SelectScalar() {
        m_AttributeIndex = 0;
        m_AttributeDimension = 0;
        m_AttributeChanged = true;
    }
    FloatArray::Pointer Colors() { return m_Colors; }
    bool HasShell() { return m_RenderableMesh.SurfaceMesh != nullptr; }
    void AddStaleShell() { m_RenderableMesh.SurfaceMesh = SurfaceMesh::New(); }
    bool HasSurfaceIndices() { return m_TriangleIndices->GetNumberOfValues() != 0; }

protected:
    Probe() = default;
};

Probe::Pointer PointsOnly() {
    auto mesh = Probe::New();
    auto points = Points::New();
    points->AddPoint(0, 0, 0);
    points->AddPoint(1, 0, 0);
    points->AddPoint(0, 1, 0);
    mesh->SetPoints(points);
    auto values = FloatArray::New();
    values->SetName("temperature");
    values->SetDimension(1);
    values->AddValue(1);
    values->AddValue(5);
    values->AddValue(10);
    mesh->GetAttributeSet()->AddScalar(IG_POINT, values);
    return mesh;
}
} // namespace

int main() {
    try {
        auto mesh = PointsOnly();
        Require(!mesh->TransferToSurfaceMesh(), "zero cells became a surface");
        mesh->AddStaleShell();
        mesh->SelectScalar();
        mesh->ConvertToDrawableData();
        Require(mesh->GetViewStyle() == IG_POINTS, "point cloud is still surface-only");
        Require(!mesh->GetShellRenderingOption() && !mesh->HasShell(), "point cloud retained a shell");
        Require(mesh->GetRenderPoints()->GetNumberOfElements() == 3, "render positions were lost");
        Require(mesh->GetNumberOfCells() == 0 && !mesh->HasSurfaceIndices(), "synthetic topology was added");
        auto colors = mesh->Colors();
        Require(colors && colors->GetNumberOfElements() == 3 && colors->GetDimension() == 4,
                "point attributes did not produce RGBA colors");
        bool different = false;
        for (int component = 0; component < 3; ++component)
            different |= std::abs(colors->GetValue(component) - colors->GetValue(8 + component)) > 1e-5;
        Require(different, "scalar extremes received the same color");
        mesh->SetViewStyle(0);
        mesh->ConvertToDrawableData();
        Require(mesh->GetViewStyle() == 0, "conversion re-enabled hidden points");

        auto empty = Probe::New();
        Require(!empty->TransferToSurfaceMesh(), "empty dataset became a surface");
        empty->ConvertToDrawableData();
        Require(empty->GetViewStyle() == IG_SURFACE, "empty dataset was classified as a point cloud");

        auto surface = PointsOnly();
        igIndex triangle[] = {0, 1, 2};
        surface->AddCell(triangle, 3, IG_TRIANGLE);
        Require(surface->TransferToSurfaceMesh() != nullptr, "valid surface conversion failed");
        surface->ConvertToDrawableData();
        Require(surface->GetViewStyle() == IG_SURFACE, "triangle forced to point mode");
        auto mixed = PointsOnly();
        mixed->AddCell(triangle, 1, IG_VERTEX);
        mixed->AddCell(triangle, 3, IG_TRIANGLE);
        mixed->SetShellRenderingOption(false);
        mixed->ConvertToDrawableData();
        Require(mixed->GetViewStyle() == IG_SURFACE && mixed->GetNumberOfCells() == 2,
                "mixed topology or style changed");
        std::cout << "PASS zero-cell point colors, stale shells, hidden/empty/surface/mixed inputs\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
