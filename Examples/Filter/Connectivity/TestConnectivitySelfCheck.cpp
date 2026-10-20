// 迁移回归（待提交）：标准菜单原先缺少该功能；保留源仓库数值、输入保留和异常边界检查，
// 防止仅接入菜单或漏迁依赖导致行为退化。交互演示保留原启动方式。
// 连通区域过滤器回归自检（无 GUI、无外部模型依赖）。
// 覆盖：6 种抽取模式、RegionId 编号染色、按单元数重编号、
//       标量连通、共享点判连通、失败路径。
#include <Connectivity/iGameConnectivityFilter.h>

#include <iGameArrayObject.h>
#include <iGameAttributeSet.h>
#include <iGameCellArray.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>

#include <cstdio>
#include <limits>
#include <vector>

using namespace iGame;

namespace {

int g_checks = 0;
int g_failures = 0;

void Expect(bool ok, const char* what) {
    ++g_checks;
    if (!ok) ++g_failures;
    std::printf("  -> %s : %s\n", ok ? "PASS" : "FAIL", what);
}

// 三个互不连通的分量：区域 0 = 2 个三角形，区域 1 = 1 个三角形，区域 2 = 1 个三角形。
SurfaceMesh::Pointer MakeThreeIslands() {
    auto mesh = SurfaceMesh::New();
    auto pts = Points::New();
    // 区域 0：四边形 (0,1,2,3) 拆成 2 个三角形
    pts->AddPoint(Point(0, 0, 0));
    pts->AddPoint(Point(1, 0, 0));
    pts->AddPoint(Point(1, 1, 0));
    pts->AddPoint(Point(0, 1, 0));
    // 区域 1：三角形 (4,5,6)，远离区域 0
    pts->AddPoint(Point(5, 0, 0));
    pts->AddPoint(Point(6, 0, 0));
    pts->AddPoint(Point(5, 1, 0));
    // 区域 2：三角形 (7,8,9)，远离前两者
    pts->AddPoint(Point(10, 0, 0));
    pts->AddPoint(Point(11, 0, 0));
    pts->AddPoint(Point(10, 1, 0));
    mesh->SetPoints(pts);

    auto faces = CellArray::New();
    faces->AddCellId3(0, 1, 2); // f0 -> 区域 0
    faces->AddCellId3(0, 2, 3); // f1 -> 区域 0
    faces->AddCellId3(4, 5, 6); // f2 -> 区域 1
    faces->AddCellId3(7, 8, 9); // f3 -> 区域 2
    mesh->SetFaces(faces);
    return mesh;
}

// 链式三角带：f0=(0,1,2) 与 f1=(2,3,4) 仅共享点 2，用于标量连通测试。
SurfaceMesh::Pointer MakeChainWithScalar(const std::vector<double>& scalar) {
    auto mesh = SurfaceMesh::New();
    auto pts = Points::New();
    pts->AddPoint(Point(0, 0, 0));
    pts->AddPoint(Point(1, 0, 0));
    pts->AddPoint(Point(1, 1, 0));
    pts->AddPoint(Point(2, 1, 0));
    pts->AddPoint(Point(2, 2, 0));
    mesh->SetPoints(pts);
    auto faces = CellArray::New();
    faces->AddCellId3(0, 1, 2);
    faces->AddCellId3(2, 3, 4);
    mesh->SetFaces(faces);

    auto arr = FloatArray::New();
    arr->SetName("v");
    arr->SetDimension(1);
    arr->Resize(scalar.size());
    for (size_t i = 0; i < scalar.size(); ++i) {
        arr->RawPointer(static_cast<IGsize>(i))[0] = static_cast<float>(scalar[i]);
    }
    auto attrs = AttributeSet::New();
    attrs->AddAttribute(IG_SCALAR, IG_POINT, arr);
    mesh->SetAttributeSet(attrs);
    return mesh;
}

ArrayObject::Pointer FindRegionId(const SurfaceMesh::Pointer& mesh, IGenum attachment) {
    if (!mesh || !mesh->GetAttributeSet()) return nullptr;
    auto all = mesh->GetAttributeSet()->GetAllAttributes();
    if (!all) return nullptr;
    for (IGsize i = 0; i < all->GetNumberOfElements(); ++i) {
        auto& a = all->GetElement(i);
        if (a.isDeleted || a.pointer.IsNull()) continue;
        if (a.attachmentType == attachment && a.pointer->GetName() == ConnectivityFilter::RegionIdName) {
            return a.pointer;
        }
    }
    return nullptr;
}

void TestAllRegions() {
    std::printf("[case] ALL_REGIONS + 编号染色\n");
    auto src = MakeThreeIslands();
    auto filter = ConnectivityFilter::New();
    filter->SetInput(src);
    filter->SetExtractionMode(ConnectivityFilter::ALL_REGIONS);
    filter->SetColorRegions(true);
    Expect(filter->Execute(), "Execute returns true");

    auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
    Expect(out != nullptr, "output is a SurfaceMesh");
    if (!out) return;
    Expect(out->GetNumberOfFaces() == 4, "all 4 faces kept");
    Expect(filter->GetNumberOfExtractedRegions() == 3, "3 regions extracted");
    const auto& sizes = filter->GetRegionSizes();
    Expect(sizes.size() == 3 && sizes[0] == 2 && sizes[1] == 1 && sizes[2] == 1,
           "region sizes = {2,1,1}");

    auto cr = DynamicCast<IntArray>(FindRegionId(out, IG_CELL));
    Expect(cr != nullptr, "cell RegionId present");
    if (cr) {
        Expect(cr->GetValue(0) == 0 && cr->GetValue(1) == 0, "cells 0/1 -> region 0");
        Expect(cr->GetValue(2) == 1, "cell 2 -> region 1");
        Expect(cr->GetValue(3) == 2, "cell 3 -> region 2");
    }
    auto pr = DynamicCast<IntArray>(FindRegionId(out, IG_POINT));
    Expect(pr != nullptr, "point RegionId present");
    if (pr) { Expect(pr->GetNumberOfElements() == 10, "point RegionId size = 10"); }
}

void TestLargest() {
    std::printf("[case] LARGEST_REGION\n");
    auto src = MakeThreeIslands();
    auto filter = ConnectivityFilter::New();
    filter->SetInput(src);
    filter->SetExtractionMode(ConnectivityFilter::LARGEST_REGION);
    Expect(filter->Execute(), "Execute returns true");
    auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (out) {
        Expect(out->GetNumberOfFaces() == 2, "largest region has 2 faces");
        Expect(out->GetNumberOfPoints() == 4, "largest region has 4 points");
    }
}

void TestSpecified() {
    std::printf("[case] SPECIFIED_REGIONS\n");
    auto src = MakeThreeIslands();
    auto filter = ConnectivityFilter::New();
    filter->SetInput(src);
    filter->SetExtractionMode(ConnectivityFilter::SPECIFIED_REGIONS);
    filter->InitializeSpecifiedRegionList();
    filter->AddSpecifiedRegion(1);
    filter->AddSpecifiedRegion(2);
    Expect(filter->Execute(), "Execute returns true");
    auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (out) { Expect(out->GetNumberOfFaces() == 2, "regions 1+2 -> 2 faces"); }
}

void TestSeeded() {
    std::printf("[case] CELL/POINT/CLOSEST 种子模式\n");
    auto src = MakeThreeIslands();

    {
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src);
        filter->SetExtractionMode(ConnectivityFilter::CELL_SEEDED_REGIONS);
        filter->InitializeSeedList();
        filter->AddSeed(0);
        Expect(filter->Execute(), "cell-seeded Execute returns true");
        auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
        if (out) { Expect(out->GetNumberOfFaces() == 2, "cell 0 seed -> region of 2 faces"); }
    }
    {
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src);
        filter->SetExtractionMode(ConnectivityFilter::POINT_SEEDED_REGIONS);
        filter->InitializeSeedList();
        filter->AddSeed(7); // 仅属于 f3（区域 2）
        Expect(filter->Execute(), "point-seeded Execute returns true");
        auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
        if (out) { Expect(out->GetNumberOfFaces() == 1, "point 7 seed -> 1 face"); }
    }
    {
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src);
        filter->SetExtractionMode(ConnectivityFilter::CLOSEST_POINT_REGION);
        filter->SetClosestPoint(5.2, 0.1, 0.0); // 最近点 4（区域 1）
        Expect(filter->Execute(), "closest-point Execute returns true");
        auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
        if (out) { Expect(out->GetNumberOfFaces() == 1, "closest to (5.2,0.1,0) -> 1 face"); }
    }
}

void TestReassignment() {
    std::printf("[case] RegionIdAssignmentMode\n");
    auto src = MakeThreeIslands();
    auto filter = ConnectivityFilter::New();
    filter->SetInput(src);
    filter->SetExtractionMode(ConnectivityFilter::ALL_REGIONS);
    filter->SetRegionIdAssignmentMode(ConnectivityFilter::CELL_COUNT_ASCENDING);
    Expect(filter->Execute(), "Execute returns true");
    auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
    auto cr = DynamicCast<IntArray>(FindRegionId(out, IG_CELL));
    Expect(cr != nullptr, "cell RegionId present");
    if (cr) {
        // 升序：两个 1-face 区域得 0/1，2-face 区域得 2。
        Expect(cr->GetValue(2) == 0, "1-face region reordered to id 0");
        Expect(cr->GetValue(3) == 1, "1-face region reordered to id 1");
        Expect(cr->GetValue(0) == 2, "2-face region reordered to id 2");
    }
    const auto& sizes = filter->GetRegionSizes();
    Expect(sizes.size() == 3 && sizes[0] == 1 && sizes[1] == 1 && sizes[2] == 2,
           "region sizes reordered to {1,1,2}");
}

void TestScalarConnectivity() {
    std::printf("[case] ScalarConnectivity\n");
    // 点 2 的值远在区间外，隔断 f0/f1；点 3/4 在区间内。
    auto src = MakeChainWithScalar({0.0, 0.0, 0.0, 0.0, 0.0});

    {
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src);
        filter->SetExtractionMode(ConnectivityFilter::ALL_REGIONS);
        Expect(filter->Execute(), "geometric-only Execute returns true");
        Expect(filter->GetNumberOfExtractedRegions() == 1, "no scalar split -> 1 region");
    }
    {
        auto src2 = MakeChainWithScalar({0.0, 0.0, 10.0, 10.0, 10.0});
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src2);
        filter->SetScalarConnectivity(true);
        filter->SetScalarArrayName("v");
        filter->SetScalarRange(0.0, 1.0);
        filter->SetExtractionMode(ConnectivityFilter::ALL_REGIONS);
        Expect(filter->Execute(), "scalar Execute returns true");
        Expect(filter->GetNumberOfExtractedRegions() == 2, "scalar range splits into 2 regions");
    }
}

void TestFailures() {
    std::printf("[case] 失败路径\n");
    {
        auto filter = ConnectivityFilter::New();
        Expect(!filter->Execute(), "null input fails");
    }
    {
        auto um = UnstructuredMesh::New();
        um->GetPoints()->AddPoint(Point(0, 0, 0));
        um->GetPoints()->AddPoint(Point(1, 0, 0));
        um->GetPoints()->AddPoint(Point(0, 1, 0));
        igIndex ids[3] = {0, 1, 2};
        um->AddCell(ids, 3, IG_TRIANGLE);
        auto filter = ConnectivityFilter::New();
        filter->SetInput(um);
        Expect(!filter->Execute(), "UnstructuredMesh input rejected");
    }
    {
        auto src = MakeThreeIslands();
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src);
        filter->SetExtractionMode(ConnectivityFilter::CELL_SEEDED_REGIONS);
        // 不给种子
        Expect(!filter->Execute(), "seeded mode without seeds fails");
    }
    {
        auto src = MakeThreeIslands();
        auto filter = ConnectivityFilter::New();
        filter->SetInput(src);
        filter->SetExtractionMode(999);
        Expect(!filter->Execute(), "invalid extraction mode fails");
    }
}

// BUG: attributes went through double and silently changed 64-bit IDs above 2^53.
// Trigger: multi-component point/cell arrays, including signed and unsigned extrema, then copy/extract.
// Expected: exact native values, original type and independent buffers for every supported numeric type.
// Fix commit: 3947ff2d86205e3b2205b24bc73876d076d8dbee.
template<class Array, class Value>
void CheckExactAttributeCopy(Value first, Value second) {
    auto src = MakeThreeIslands();
    auto filter = ConnectivityFilter::New();
    filter->SetExtractionMode(ConnectivityFilter::SPECIFIED_REGIONS);
    filter->AddSpecifiedRegion(2);
    auto point = Array::New(); point->SetName("ExactPoint"); point->SetDimension(2);
    point->Resize(src->GetNumberOfPoints());
    auto cell = Array::New(); cell->SetName("ExactCell"); cell->SetDimension(2);
    cell->Resize(src->GetNumberOfFaces());
    for (IGsize t = 0; t < point->GetNumberOfElements(); ++t) {
        point->RawPointer(t)[0] = t % 2 ? second : first;
        point->RawPointer(t)[1] = t % 2 ? first : second;
    }
    for (IGsize t = 0; t < cell->GetNumberOfElements(); ++t) {
        cell->RawPointer(t)[0] = t % 2 ? second : first;
        cell->RawPointer(t)[1] = t % 2 ? first : second;
    }
    src->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, point);
    src->GetAttributeSet()->AddAttribute(IG_VECTOR, IG_CELL, cell);
    filter->SetInput(src);
    Expect(filter->Execute(), "typed attribute Execute succeeds");
    auto out = DynamicCast<SurfaceMesh>(filter->GetOutput());
    if (!out) { Expect(false, "typed copy output exists"); return; }
    auto p = DynamicCast<Array>(out->GetAttributeSet()->GetAttribute("ExactPoint").pointer);
    auto c = DynamicCast<Array>(out->GetAttributeSet()->GetAttribute("ExactCell").pointer);
    bool exact = p && c && p->GetNumberOfElements() == 3 && c->GetNumberOfElements() == 1;
    for (IGsize t = 0; exact && t < p->GetNumberOfElements(); ++t)
        for (int d = 0; d < 2; ++d) exact = exact && p->RawPointer(t)[d] == point->RawPointer(t + 7)[d];
    for (IGsize t = 0; exact && t < c->GetNumberOfElements(); ++t)
        for (int d = 0; d < 2; ++d) exact = exact && c->RawPointer(t)[d] == cell->RawPointer(3)[d];
    Expect(exact, "point/cell values and array types are exact after copy/extraction");
    if (p && c) {
        const auto originalPoint = point->RawPointer(7)[0];
        const auto originalCell = cell->RawPointer(3)[0];
        p->RawPointer(0)[0] = originalPoint == first ? second : first;
        c->RawPointer(0)[0] = originalCell == first ? second : first;
        Expect(point->RawPointer(7)[0] == originalPoint && cell->RawPointer(3)[0] == originalCell,
               "changing the output does not change input attribute buffers");
    }
}

void TestExactAttributes() {
    std::printf("[case] native numeric attributes retain exact values\n");
    CheckExactAttributeCopy<FloatArray>(1.25f, -2.5f);
    CheckExactAttributeCopy<DoubleArray>(1.0 / 3.0, -2.25);
    CheckExactAttributeCopy<IntArray>(std::numeric_limits<int>::max(), std::numeric_limits<int>::lowest());
    CheckExactAttributeCopy<UnsignedIntArray>(std::numeric_limits<unsigned int>::max(), 0u);
    CheckExactAttributeCopy<CharArray>(char(3), char(0));
    CheckExactAttributeCopy<UnsignedCharArray>(static_cast<unsigned char>(255), static_cast<unsigned char>(0));
    CheckExactAttributeCopy<ShortArray>(std::numeric_limits<short>::max(), std::numeric_limits<short>::lowest());
    CheckExactAttributeCopy<UnsignedShortArray>(std::numeric_limits<unsigned short>::max(), static_cast<unsigned short>(0));
    CheckExactAttributeCopy<LongLongArray>(9007199254740993LL, std::numeric_limits<long long>::lowest());
    CheckExactAttributeCopy<UnsignedLongLongArray>(std::numeric_limits<unsigned long long>::max(), 9007199254740993ULL);
}

// BUG: old RegionId arrays shadowed freshly generated arrays on repeated execution.
// Trigger: rerun a colored output with the opposite numbering order, and extract sorted IDs.
// Expected: one point and one cell RegionId; sorted IDs select the same region shown by All Regions.
// Equal-sized regions retain traversal order in either sort direction. Fix commit: 3947ff2d86205e3b2205b24bc73876d076d8dbee.
void TestRepeatAndSortedSelection() {
    auto first = ConnectivityFilter::New(); first->SetInput(MakeThreeIslands());
    first->SetExtractionMode(ConnectivityFilter::ALL_REGIONS);
    first->SetRegionIdAssignmentMode(ConnectivityFilter::CELL_COUNT_ASCENDING);
    Expect(first->Execute(), "initial ascending numbering succeeds");
    auto original = DynamicCast<SurfaceMesh>(first->GetOutput());
    auto next = ConnectivityFilter::New(); next->SetInput(original);
    next->SetExtractionMode(ConnectivityFilter::ALL_REGIONS);
    next->SetRegionIdAssignmentMode(ConnectivityFilter::CELL_COUNT_DESCENDING);
    Expect(next->Execute(), "rerun descending numbering succeeds");
    auto output = DynamicCast<SurfaceMesh>(next->GetOutput());
    if (!output) return;
    int pointIds = 0, cellIds = 0;
    auto attrs = output->GetAttributeSet()->GetAllAttributes();
    for (IGsize i = 0; i < attrs->GetNumberOfElements(); ++i) {
        auto& attr = attrs->GetElement(i);
        if (attr.pointer && attr.pointer->GetName() == "RegionId") {
            pointIds += attr.attachmentType == IG_POINT; cellIds += attr.attachmentType == IG_CELL;
        }
    }
    Expect(pointIds == 1 && cellIds == 1, "rerun replaces old point/cell RegionId arrays");
    auto ids = FindRegionId(output, IG_CELL);
    Expect(ids && ids->GetValue(0) == 0 && ids->GetValue(2) == 1 && ids->GetValue(3) == 2,
           "descending IDs are current and ties retain traversal order");
    Expect(FindRegionId(original, IG_CELL)->GetValue(0) == 2, "rerun retains original IDs in input");
    for (int assignment : {ConnectivityFilter::UNSPECIFIED, ConnectivityFilter::CELL_COUNT_ASCENDING,
                           ConnectivityFilter::CELL_COUNT_DESCENDING}) {
        auto extract = ConnectivityFilter::New(); extract->SetInput(MakeThreeIslands());
        extract->SetExtractionMode(ConnectivityFilter::SPECIFIED_REGIONS);
        extract->SetRegionIdAssignmentMode(assignment); extract->AddSpecifiedRegion(0);
        Expect(extract->Execute(), "sorted specified region succeeds");
        auto part = DynamicCast<SurfaceMesh>(extract->GetOutput());
        const int count = assignment == ConnectivityFilter::CELL_COUNT_ASCENDING ? 1 : 2;
        Expect(part && part->GetNumberOfFaces() == count && FindRegionId(part, IG_CELL)->GetValue(0) == 0,
               "specified id 0 selects the sorted region and retains id 0");
        extract->SetColorRegions(false);
        Expect(extract->Execute(), "sorted extraction also succeeds without generated IDs");
        part = DynamicCast<SurfaceMesh>(extract->GetOutput());
        Expect(part && part->GetNumberOfFaces() == count && !FindRegionId(part, IG_CELL),
               "coloring switch does not alter sorted selection");
    }
    next->SetColorRegions(false);
    Expect(next->Execute(), "disabling new RegionId generation succeeds");
    output = DynamicCast<SurfaceMesh>(next->GetOutput());
    Expect(output && FindRegionId(output, IG_CELL)->GetValue(0) == 2,
           "without generation, existing IDs are copied unchanged");
}

// BUG: an ineligible start face could absorb an eligible neighbor, changing regions when faces reordered.
// Trigger: mixed scalar eligibility, swapped face order, strict/full scalar mode and seeded extraction.
// Expected: ineligible faces stay separate in All Regions; seeds must be eligible; any means a vertex hits.
// Fix commit: 3947ff2d86205e3b2205b24bc73876d076d8dbee.
void TestScalarOrderAndBoundaries() {
    auto mesh = MakeChainWithScalar({10,10,10,0,0});
    auto f = ConnectivityFilter::New(); f->SetInput(mesh); f->SetScalarConnectivity(true);
    f->SetScalarArrayName("v"); f->SetScalarRange(0,1); f->SetExtractionMode(ConnectivityFilter::ALL_REGIONS);
    Expect(f->Execute() && f->GetNumberOfExtractedRegions() == 2,
           "ineligible start does not absorb eligible neighbor");
    auto faces = CellArray::New(); faces->AddCellId3(2,3,4); faces->AddCellId3(0,1,2); mesh->SetFaces(faces);
    Expect(f->Execute() && f->GetNumberOfExtractedRegions() == 2,
           "face-order swap retains region count");
    f->SetExtractionMode(ConnectivityFilter::CELL_SEEDED_REGIONS); f->AddSeed(1);
    Expect(!f->Execute() && !f->GetOutput(), "ineligible seed fails without stale output");
    f->InitializeSeedList(); f->AddSeed(0);
    Expect(f->Execute(), "eligible seed succeeds");
    auto out = DynamicCast<SurfaceMesh>(f->GetOutput());
    Expect(out && out->GetNumberOfFaces() == 1, "eligible seed does not expand into ineligible face");
    f->SetExtractionMode(ConnectivityFilter::ALL_REGIONS); f->SetFullScalarConnectivity(true);
    Expect(f->Execute() && f->GetNumberOfExtractedRegions() == 2, "full scalar mode rejects partially eligible faces");
    f->SetFullScalarConnectivity(false); f->SetInput(MakeChainWithScalar({0,2,2,0,2})); f->SetScalarRange(0.5,1.5);
    Expect(f->Execute() && f->GetNumberOfExtractedRegions() == 2,
           "overlapping min/max does not count as an actual vertex hit");
    f->SetScalarRange(2,0);
    Expect(f->Execute() && f->GetNumberOfExtractedRegions() == 1, "reversed finite range works");
    f->SetScalarRange(0,std::numeric_limits<double>::quiet_NaN());
    Expect(!f->Execute() && !f->GetOutput(), "nonfinite scalar range is rejected");
    f->SetScalarRange(0,1); f->SetInput(MakeChainWithScalar({0,0}));
    Expect(!f->Execute(), "short scalar array is rejected before indexing");
}

// BUG: name-only lookup stopped at a cell/vector/multicomponent attribute named "s",
// even when a later complete one-component point scalar named "s" was available.
// Trigger: duplicate names with different attachments/types, in either insertion order.
// Expected: select the first live, complete point scalar with the exact requested name;
// skip unsuitable candidates, and never fall back to a differently named scalar.
// Fix commit: 3940540ca75ee4a8ee3d139e25013aab115a6bb3.
void TestNamedPointScalarLookup() {
    auto makeArray = [](const char* name, IGsize tuples, int dimension, float value) {
        auto array = FloatArray::New();
        array->SetName(name);
        array->SetDimension(dimension);
        array->Resize(tuples);
        for (IGsize i = 0; i < tuples; ++i) {
            for (int component = 0; component < dimension; ++component) {
                array->RawPointer(i)[component] = value;
            }
        }
        return array;
    };
    auto mesh = MakeChainWithScalar({1,1,1,1,1});
    auto filter = ConnectivityFilter::New();
    filter->SetInput(mesh);
    filter->SetScalarConnectivity(true);
    filter->SetFullScalarConnectivity(true);
    filter->SetScalarRange(1,1);
    filter->SetScalarArrayName("s");
    filter->SetExtractionMode(ConnectivityFilter::CELL_SEEDED_REGIONS);
    filter->AddSeed(0);
    const char* cases[] = {
        "cell scalar before same-name point scalar",
        "point scalar before same-name cell scalar",
        "vector before same-name point scalar",
        "multicomponent scalar before same-name point scalar",
        "deleted attribute before same-name point scalar",
        "null attribute before same-name point scalar"
    };
    for (int scenario = 0; scenario < 6; ++scenario) {
        auto attrs = AttributeSet::New();
        auto point = makeArray("s", 5, 1, 1);
        if (scenario == 1) attrs->AddAttribute(IG_SCALAR, IG_POINT, point);
        if (scenario <= 1) attrs->AddAttribute(IG_SCALAR, IG_CELL, makeArray("s", 2, 1, 0));
        if (scenario == 2) attrs->AddAttribute(IG_VECTOR, IG_POINT, makeArray("s", 5, 3, 0));
        if (scenario == 3) attrs->AddAttribute(IG_SCALAR, IG_POINT, makeArray("s", 5, 2, 0));
        if (scenario == 4) {
            const IGsize id = attrs->AddAttribute(IG_SCALAR, IG_POINT, makeArray("s", 5, 1, 0));
            attrs->DeleteAttribute(id);
        }
        if (scenario == 5) attrs->GetAllAttributes()->AddElement(AttributeSet::Attribute::None());
        if (scenario != 1) attrs->AddAttribute(IG_SCALAR, IG_POINT, point);
        mesh->SetAttributeSet(attrs);
        Expect(filter->Execute(), cases[scenario]);
        auto output = DynamicCast<SurfaceMesh>(filter->GetOutput());
        Expect(output && output->GetNumberOfFaces() == 2 && filter->GetNumberOfExtractedRegions() == 1,
               "named point scalar controls eligibility and keeps both connected faces");
        Expect(point->GetValue(0) == 1 && point->GetNumberOfElements() == 5,
               "scalar lookup retains the input point array");
    }

    for (int scenario = 0; scenario < 5; ++scenario) {
        auto attrs = AttributeSet::New();
        attrs->AddAttribute(IG_SCALAR, IG_POINT, makeArray("other", 5, 1, 1));
        if (scenario == 0) attrs->AddAttribute(IG_SCALAR, IG_CELL, makeArray("s", 2, 1, 1));
        if (scenario == 1) attrs->AddAttribute(IG_VECTOR, IG_POINT, makeArray("s", 5, 3, 1));
        if (scenario == 2) attrs->AddAttribute(IG_SCALAR, IG_POINT, makeArray("s", 5, 2, 1));
        if (scenario == 3) attrs->AddAttribute(IG_SCALAR, IG_POINT, makeArray("s", 2, 1, 1));
        mesh->SetAttributeSet(attrs);
        Expect(!filter->Execute() && !filter->GetOutput(),
               "missing or unsuitable named scalar fails without fallback or stale output");
        Expect(!filter->GetMessage().empty(), "named scalar failure provides a diagnostic");
    }
}

// BUG: an empty face was counted as a region but skipped when building output,
// so region sizes/count no longer described the output faces.
// Trigger: a valid surface followed by a zero-vertex face.
// Expected: reject the malformed topology, report why and clear the prior output.
// Fix commit: 3940540ca75ee4a8ee3d139e25013aab115a6bb3.
void TestEmptyFace() {
    auto filter = ConnectivityFilter::New();
    filter->SetExtractionMode(ConnectivityFilter::ALL_REGIONS);
    auto mesh = MakeThreeIslands();
    filter->SetInput(mesh);
    Expect(filter->Execute(), "empty-face reuse begins with valid output");
    const igIndex dummy = 0;
    mesh->GetFaces()->AddCellIds(&dummy, 0);
    Expect(!filter->Execute() && !filter->GetOutput(), "empty face is rejected without a phantom region or stale output");
    Expect(!filter->GetMessage().empty(), "empty face provides a diagnostic");
}

// BUG: invalid parameters could silently select a region or leave previous outputs accessible.
// Expected: reject nonfinite coordinates/invalid sorted IDs; keep failures clear of stale output.
// Fix commit: 3947ff2d86205e3b2205b24bc73876d076d8dbee.
void TestAdditionalFailuresAndFan() {
    auto f = ConnectivityFilter::New(); f->SetInput(MakeThreeIslands());
    f->SetExtractionMode(ConnectivityFilter::ALL_REGIONS); Expect(f->Execute(), "reuse begins with a valid output");
    f->SetExtractionMode(ConnectivityFilter::CLOSEST_POINT_REGION);
    f->SetClosestPoint(std::numeric_limits<double>::quiet_NaN(),0,0);
    Expect(!f->Execute() && !f->GetOutput(), "nonfinite closest point fails and clears output");
    f->SetExtractionMode(ConnectivityFilter::SPECIFIED_REGIONS); f->AddSpecifiedRegion(3);
    Expect(!f->Execute(), "out-of-range region ID fails");
    f->SetExtractionMode(ConnectivityFilter::ALL_REGIONS); f->SetRegionIdAssignmentMode(999);
    Expect(!f->Execute(), "invalid numbering mode fails");
    auto fan = SurfaceMesh::New(); auto pts = Points::New(); auto faces = CellArray::New(); pts->AddPoint(Point(0,0,0));
    for (int i=0; i<4000; ++i) {
        pts->AddPoint(Point(i+1,0,0)); pts->AddPoint(Point(i+1,1,0)); faces->AddCellId3(0,2*i+1,2*i+2);
    }
    fan->SetPoints(pts); fan->SetFaces(faces);
    f->SetInput(fan); f->SetRegionIdAssignmentMode(ConnectivityFilter::UNSPECIFIED);
    Expect(f->Execute() && f->GetNumberOfExtractedRegions() == 1,
           "high-valence vertex traversal retains one region without duplicate expansion");
    auto out = DynamicCast<SurfaceMesh>(f->GetOutput());
    Expect(out && out->GetNumberOfFaces() == 4000, "high-valence traversal retains all faces");
}

} // namespace

int main() {
    TestAllRegions();
    TestLargest();
    TestSpecified();
    TestSeeded();
    TestReassignment();
    TestScalarConnectivity();
    TestFailures();
    TestExactAttributes();
    TestRepeatAndSortedSelection();
    TestScalarOrderAndBoundaries();
    TestNamedPointScalarLookup();
    TestEmptyFace();
    TestAdditionalFailuresAndFan();

    std::printf("total checks: %d, failures: %d\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
