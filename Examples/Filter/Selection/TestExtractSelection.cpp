// 迁移回归（待提交）：防止标准过滤器仅有菜单占位，验证本文件记录的算法结果、输入保持不变及边界情况。
#include "Selection/igameextractselectionfilter.h"
#include "iGameSelection.h"
#include "iGameUnstructuredMesh.h"
#include <iostream>
#include <stdexcept>

using namespace iGame;

static void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static UnstructuredMesh::Pointer MakeMesh() {
    auto mesh = UnstructuredMesh::New();
    mesh->SetName("selection_test");
    mesh->GetPoints()->AddPoint(Point(0, 0, 0));
    mesh->GetPoints()->AddPoint(Point(1, 0, 0));
    mesh->GetPoints()->AddPoint(Point(0, 1, 0));
    mesh->GetPoints()->AddPoint(Point(1, 1, 0));
    mesh->GetPoints()->AddPoint(Point(2, 1, 0));
    igIndex first[]{0, 1, 2};
    igIndex second[]{2, 1, 3};
    mesh->AddCell(first, 3, IG_TRIANGLE);
    mesh->AddCell(second, 3, IG_TRIANGLE);
    auto values = DoubleArray::New();
    values->SetName("temperature");
    for (int i = 0; i < 5; ++i) values->AddValue(10 + i);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, values);
    auto vectors = FloatArray::New();
    vectors->SetName("velocity");
    vectors->SetDimension(3);
    for (int i = 0; i < 5; ++i) vectors->AddElement3(i, i + 1, i + 2);
    mesh->GetAttributeSet()->AddAttribute(IG_VECTOR, IG_POINT, vectors);
    auto ids = LongLongArray::New();
    ids->SetName("large_ids");
    for (int i = 0; i < 5; ++i) ids->AddValue(9007199254740993LL + i);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, ids);
    auto cellValues = IntArray::New();
    cellValues->SetName("material");
    cellValues->AddValue(7);
    cellValues->AddValue(9);
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, cellValues);
    return mesh;
}

int main() {
    try {
        auto mesh = MakeMesh();
        auto filter = ExtractSelectionFilter::New();
        Check(!filter->Execute() && !filter->GetOutput(), "null input");
        filter->SetInput(mesh);
        Check(!filter->Execute(), "empty selection");
        mesh->GetSelection()->SelectionCallBackEvent(IG_POINT, std::vector<igIndex>{4, 1, 4}, Selection::Add);
        Check(filter->Execute(), "point extraction");
        auto points = DynamicCast<PointSet>(filter->GetOutput());
        Check(points && points->GetNumberOfPoints() == 2, "point count and deduplication");
        auto vertices = DynamicCast<UnstructuredMesh>(points);
        Check(vertices && vertices->GetNumberOfCells() == 2, "one vertex cell per selected point");
        Check(vertices->GetViewStyle() == IG_POINTS, "selected vertices default to visible point rendering");
        Check(!vertices->GetShellRenderingOption(), "vertex results do not attempt surface extraction");
        for (igIndex i = 0; i < 2; ++i) {
            const igIndex* ids = nullptr;
            Check(vertices->GetCellType(i) == IG_VERTEX &&
                  vertices->GetCellPointIds(i, ids) == 1 && ids[0] == i,
                  "vertex connectivity follows output point IDs");
        }
        Check(points->GetPoint(0)[0] == 1 && points->GetPoint(1)[0] == 2, "point ordering");
        auto attrs = points->GetAttributeSet();
        Check(attrs->GetNumberOfAttributes() == 3, "point attributes only");
        Check(attrs->GetAttribute(0).pointer->GetValue(0) == 11, "scalar remapping");
        Check(attrs->GetAttribute(0).dataRange->GetValue(2) == 11 &&
              attrs->GetAttribute(0).dataRange->GetValue(3) == 14, "attribute range recomputed");
        Check(attrs->GetAttribute(1).pointer->GetDimension() == 3 &&
              attrs->GetAttribute(1).pointer->GetValue(3) == 4, "vector remapping");
        auto large = DynamicCast<LongLongArray>(attrs->GetAttribute(2).pointer);
        Check(large && large->RawPointer()[0] == 9007199254740994LL, "integer precision and type");
        attrs->GetAttribute(0).pointer->SetValue(0, 999);
        Check(mesh->GetAttributeSet()->GetAttribute(0).pointer->GetValue(1) == 11, "independent attributes");
        Check(points->GetPoints() != mesh->GetPoints(), "independent geometry");

        mesh->GetSelection()->SelectionCallBackEvent(IG_CELL, std::vector<igIndex>{0, 1}, Selection::Add);
        filter->SetSelectionType(IG_CELL);
        Check(filter->Execute(), "cell extraction");
        auto cells = DynamicCast<UnstructuredMesh>(filter->GetOutput());
        Check(cells && cells->GetNumberOfCells() == 2 && cells->GetNumberOfPoints() == 4, "shared vertices");
        const igIndex* connectivity = nullptr;
        Check(cells->GetCellPointIds(1, connectivity) == 3 && connectivity[0] == 2 &&
              connectivity[1] == 1 && connectivity[2] == 3, "connectivity remapping");
        Check(cells->GetCellType(1) == IG_TRIANGLE, "cell type");
        Check(cells->GetAttributeSet()->GetAttribute(3).pointer->GetValue(1) == 9, "cell attributes");
        Check(mesh->GetNumberOfPoints() == 5 && mesh->GetNumberOfCells() == 2 &&
              mesh->GetSelection()->GetSelectedCells().size() == 2, "input and selection unchanged");

        mesh->GetSelection()->SelectionCallBackEvent(IG_CELL, std::vector<igIndex>{0}, Selection::Remove);
        Check(filter->Execute(), "single cell extraction");
        cells = DynamicCast<UnstructuredMesh>(filter->GetOutput());
        Check(cells->GetNumberOfPoints() == 3 && cells->GetAttributeSet()->GetAttribute(0).pointer->GetValue(0) == 12,
              "single cell point attributes follow remap");

        auto surface = mesh->TransferToSurfaceMesh();
        surface->GetSelection()->SelectionCallBackEvent(IG_CELL, std::vector<igIndex>{1}, Selection::Add);
        filter->SetInput(surface);
        Check(filter->Execute(), "surface mesh cells");

        auto tetrahedra = MakeMesh();
        auto volumes = CellArray::New();
        igIndex tetra[]{0, 1, 2, 3};
        volumes->AddCellIds(tetra, 4);
        auto types = UnsignedIntArray::New();
        types->AddValue(IG_TETRA);
        tetrahedra->SetCells(volumes, types);
        auto volume = tetrahedra->TransferToVolumeMesh();
        volume->GetSelection()->SelectionCallBackEvent(IG_CELL, std::vector<igIndex>{0}, Selection::Add);
        filter->SetInput(volume);
        if (!filter->Execute()) throw std::runtime_error("volume mesh cells: " + filter->GetLastError());
        cells = DynamicCast<UnstructuredMesh>(filter->GetOutput());
        Check(cells->GetNumberOfPoints() == 4 && cells->GetCellType(0) == IG_TETRA, "tetrahedron type");

        auto poly = MakeMesh();
        igIndex faces[]{4, 3, 0, 2, 1, 3, 0, 1, 3, 3, 1, 2, 3, 3, 2, 0, 3};
        poly->AddCell(faces, 17, IG_POLYHEDRON);
        poly->GetAttributeSet()->GetAttribute(3).pointer->Resize(3);
        poly->GetSelection()->SelectionCallBackEvent(IG_CELL, std::vector<igIndex>{2}, Selection::Add);
        filter->SetInput(poly);
        Check(filter->Execute(), "polyhedron extraction");
        cells = DynamicCast<UnstructuredMesh>(filter->GetOutput());
        Check(cells->GetCellPointIds(0, connectivity) == 17 && connectivity[0] == 4 &&
              connectivity[1] == 3 && connectivity[3] == 1, "polyhedron counts preserved and IDs remapped");
        auto malformed = MakeMesh();
        igIndex badFace[]{1, 7, 0, 1, 2};
        malformed->AddCell(badFace, 5, IG_POLYHEDRON);
        malformed->GetSelection()->SelectionCallBackEvent(IG_CELL, std::vector<igIndex>{2}, Selection::Add);
        filter->SetInput(malformed);
        Check(!filter->Execute() && !filter->GetOutput(), "malformed polyhedron rejected");

        filter->SetInput(mesh);
        mesh->GetSelection()->SelectionCallBackEvent(IG_CELL, std::vector<igIndex>{99}, Selection::Add);
        Check(!filter->Execute() && !filter->GetOutput(), "invalid ID clears stale output");
        filter->SetSelectionType(IG_NONE);
        Check(!filter->Execute(), "invalid mode");
        filter->SetSelectionType(IG_POINT);
        auto broken = MakeMesh();
        broken->GetSelection()->SelectionCallBackEvent(IG_POINT, std::vector<igIndex>{4}, Selection::Add);
        broken->GetAttributeSet()->GetAttribute(0).pointer->Resize(1);
        filter->SetInput(broken);
        Check(!filter->Execute() && !filter->GetOutput(), "short attribute rejected");
        broken->GetSelection()->SelectionCallBackEvent(IG_POINT, std::vector<igIndex>{-1}, Selection::Add);
        Check(!filter->Execute(), "negative ID rejected");
        std::cout << "ExtractSelection regression tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
