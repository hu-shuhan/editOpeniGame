// Find the integration commit: git log --diff-filter=A --format="%h %s" -- Examples/Filter/Convert/TestResampleOctreeChecks.cpp
// Regression/example imported from dayuwan77/igamevis at
// eccac729b57aeacbe9312d7d5189f6990bb4eebd (same relative path).
// Integration regression: these filters and their example assets were missing
// from iGameVis-multiFilter. The checks below cover their output/attribute and
// geometry contracts; visual examples retain an interactive default mode.
// Local integration fix: feat: integrate second-batch standard filters.
// ============================================================================
// ResampleToImage / PointSetToOctree 数值与逻辑一致性回归检查（无窗口，可直接进 CI）
//
// 覆盖内容：
//   A. 四面体基线回归：与 VTK vtkResampleToImage 对比时的已知基线数值
//      （64^3 → 262144 格点 / 250047 单元 / 229376 有效格点 / spacing = 1/63）
//   B. 三角形单元：三角形**平面外**的格点必须判为无效（点到平面距离判定）
//   C. 线段单元：偏离轴线的格点必须判为无效（点到直线垂直距离判定）
//   D. 四边形单元：平面内与**倾斜平面**均按三维局部坐标判定（Gauss–Newton 残差）
//   E. 不受支持的单元类型必须显式提示或拒绝，不静默产出不完整结果
//   F/G. 属性策略：保留存储类型与分量数、ID 类数组禁止线性插值、同名点/单元冲突时点数据优先
//   H. 点集转八叉树：按 VTK 公式独立复算每个点的体素归属并自行统计，与过滤器输出逐项比对
//   I. 线性场再现（四面体）：f = 3x+5y+7z 在有效格点上必须被精确再现（同时验证单元搜索与插值权重）
//   J. 线性场再现（六面体）：同上，与 VTK vtkHexahedron 的权重路径对照
//
// 失败时返回非 0，成功返回 0。
// 运行方式（工作目录为 Examples 构建目录，Models 已自动拷贝到 ./Models）：
//     ./testResampleOctreeChecks
// ============================================================================
#include <Convert/iGamePointSetToOctreeFilter.h>
#include <Convert/iGameResampleToImageFilter.h>
#include <iGameAttributeSet.h>
#include <iGameCellType.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace iGame; // 单元类型枚举位于 iGame 命名空间

namespace {

int g_failed = 0;
int g_total = 0;

void Check(bool condition, const std::string& what) {
    ++g_total;
    if (!condition) ++g_failed;
    std::cout << (condition ? "  [ OK ] " : "  [FAIL] ") << what << "\n";
}

struct CellSpec {
    std::vector<igIndex> ids;
    IGenum type;
};

/// 程序化构造网格：可选地点属性数组（值随点位置变化）与单元属性数组。
UnstructuredMesh::Pointer MakeMesh(const std::vector<Point>& pts, std::vector<CellSpec>& cells,
                                   const std::string& pointFieldName, int pointFieldDim,
                                   const std::string& cellFieldName = std::string()) {
    auto mesh = UnstructuredMesh::New();
    for (const auto& p : pts) mesh->AddPoint(p);
    for (auto& c : cells) mesh->AddCell(c.ids.data(), static_cast<int>(c.ids.size()), c.type);

    if (pointFieldDim > 0 && !pointFieldName.empty()) {
        auto arr = FloatArray::New();
        arr->SetName(pointFieldName);
        arr->SetDimension(pointFieldDim);
        arr->Resize(static_cast<IGsize>(pts.size()));
        std::vector<double> v(pointFieldDim);
        for (size_t i = 0; i < pts.size(); ++i) {
            for (int d = 0; d < pointFieldDim; ++d) {
                v[d] = 1.0 * pts[i][0] + 10.0 * pts[i][1] + 100.0 * pts[i][2] +
                       1000.0 * static_cast<double>(i) + d;
            }
            arr->SetElement(static_cast<IGsize>(i), v.data());
        }
        mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, arr);
    }
    if (!cellFieldName.empty()) {
        auto carr = FloatArray::New();
        carr->SetName(cellFieldName);
        carr->SetDimension(1);
        carr->Resize(static_cast<IGsize>(cells.size()));
        for (size_t i = 0; i < cells.size(); ++i) {
            double v = 9.0 + static_cast<double>(i);
            carr->SetElement(static_cast<IGsize>(i), &v);
        }
        mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, carr);
    }
    return mesh;
}

ArrayObject::Pointer FindArray(const StructuredMesh::Pointer& mesh, const std::string& name,
                               IGenum* attachType = nullptr) {
    auto attrs = mesh->GetAttributeSet()->GetAllAttributes();
    for (IGsize i = 0; i < attrs->GetNumberOfElements(); ++i) {
        auto& a = attrs->GetElement(i);
        if (a.isDeleted || a.pointer == nullptr) continue;
        if (a.pointer->GetName() == name) {
            if (attachType != nullptr) *attachType = a.attachmentType;
            return a.pointer;
        }
    }
    return nullptr;
}

IGsize CountValid(const StructuredMesh::Pointer& mesh, ArrayObject::Pointer mask) {
    IGsize n = 0;
    for (IGsize p = 0; p < mesh->GetNumberOfPoints(); ++p) {
        if (mask->GetElementValue(p, 0) != 0.0) ++n;
    }
    return n;
}

/// 逐格点比较有效掩膜与「只依赖几何」的解析判定（独立于过滤器内部实现）。
void CheckMaskAgainstGeometry(const StructuredMesh::Pointer& out, ArrayObject::Pointer mask,
                              const std::function<bool(double, double, double)>& expected,
                              const std::string& label) {
    IGsize mismatches = 0;
    IGsize expectedValid = 0;
    IGsize gotValid = 0;
    for (IGsize p = 0; p < out->GetNumberOfPoints(); ++p) {
        const Point& q = out->GetPoint(p);
        const bool e = expected(static_cast<double>(q[0]), static_cast<double>(q[1]),
                                static_cast<double>(q[2]));
        const bool g = (mask->GetElementValue(p, 0) != 0.0);
        if (e) ++expectedValid;
        if (g) ++gotValid;
        if (e != g) ++mismatches;
    }
    std::cout << "         expectedValid=" << expectedValid << " gotValid=" << gotValid
              << " mismatches=" << mismatches << "\n";
    Check(mismatches == 0, label + "：有效掩膜与几何解析判定逐点一致");
}

/// 用显式采样范围执行重采样：x,y ∈ [0,1]、z ∈ [-0.5,0.5]，3×3×3 格点。
StructuredMesh::Pointer RunResampleExplicit(DataObject::Pointer input) {
    auto filter = ResampleToImageFilter::New();
    filter->SetInput(input);
    filter->SetSamplingDimensions(3, 3, 3);
    filter->SetUseInputBounds(false);
    filter->SetSamplingBounds(0.0, 1.0, -0.5, 0.5, -0.5, 0.5);
    if (!filter->Execute()) return StructuredMesh::Pointer(nullptr);
    return DynamicCast<StructuredMesh>(filter->GetOutput(0));
}

} // namespace

// ---------------------------------------------------------------------------
// A. 四面体基线回归
// ---------------------------------------------------------------------------
static void TestTetraBaseline() {
    std::cout << "\n=== A. 四面体基线回归（Models/ResampleCubeVector.vtk, 64^3）===\n";
    DataObject::Pointer input = FileIO::ReadFile("./Models/ResampleCubeVector.vtk");
    if (input == nullptr) {
        Check(false, "读取 ./Models/ResampleCubeVector.vtk");
        return;
    }

    auto filter = ResampleToImageFilter::New();
    filter->SetInput(input);
    filter->SetSamplingDimensions(64, 64, 64);
    if (!filter->Execute()) {
        Check(false, "执行重采样");
        return;
    }
    auto out = DynamicCast<StructuredMesh>(filter->GetOutput(0));
    if (out == nullptr) {
        Check(false, "输出为 StructuredMesh");
        return;
    }

    const IGsize nPts = out->GetNumberOfPoints();
    const IGsize nCells = out->GetNumberOfCells();
    std::cout << "         points=" << nPts << " cells=" << nCells << "\n";
    Check(nPts == 262144, "输出格点数 = 262144 (64^3)");
    Check(nCells == 250047, "输出单元数 = 250047 (63^3)");

    auto mask = FindArray(out, ResampleToImageFilter::GetMaskArrayName());
    Check(mask != nullptr, "输出包含 vtkValidPointMask");
    if (mask == nullptr) return;
    const IGsize valid = CountValid(out, mask);
    std::cout << "         validPoints=" << valid << "/" << nPts << "\n";
    Check(valid == 229376, "有效格点数 = 229376（= 262144 * 7/8）");

    const Point& p0 = out->GetPoint(0);
    const Point& p1 = out->GetPoint(1);
    const double dx = static_cast<double>(p1[0]) - static_cast<double>(p0[0]);
    std::cout << "         origin=(" << static_cast<double>(p0[0]) << ", "
              << static_cast<double>(p0[1]) << ", " << static_cast<double>(p0[2])
              << ") spacing_x=" << dx << "\n";
    Check(std::fabs(dx - 1.0 / 63.0) < 1.0e-6, "spacing_x = 1/63 ≈ 0.015873");

    auto field = FindArray(out, "field");
    Check(field != nullptr, "输出保留点数组 field");
    if (field != nullptr) {
        Check(field->GetDimension() == 3, "field 分量数保持为 3");
        for (IGsize p = 0; p < nPts; ++p) {
            if (mask->GetElementValue(p, 0) == 0.0) continue;
            bool nearZero = true;
            for (int d = 0; d < 3; ++d) {
                if (std::fabs(field->GetElementValue(p, d)) > 1.0e-4) nearZero = false;
            }
            Check(nearZero, "首个有效格点 field ≈ (0,0,0)（原点处插值）");
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// B/C/D. 一维、二维单元的三维几何判定
// ---------------------------------------------------------------------------
static void TestLowDimensionalCells() {
    std::cout << "\n=== B. 三角形单元：平面外格点必须判为无效 ===\n";
    {
        std::vector<Point> pts = {Point(0.f, 0.f, 0.f), Point(1.f, 0.f, 0.f), Point(1.f, 1.f, 0.f),
                                  Point(0.f, 1.f, 0.f)};
        std::vector<CellSpec> cells = {{{0, 1, 2}, IG_TRIANGLE}, {{0, 2, 3}, IG_TRIANGLE}};
        auto mesh = MakeMesh(pts, cells, "field", 1);
        auto out = RunResampleExplicit(mesh);
        Check(out != nullptr, "Triangle 网格重采样执行成功");
        if (out != nullptr) {
            auto mask = FindArray(out, ResampleToImageFilter::GetMaskArrayName());
            CheckMaskAgainstGeometry(
                    out, mask,
                    [](double x, double y, double z) {
                        return std::fabs(z) <= 1.0e-9 && x >= -1.0e-9 && x <= 1.0 + 1.0e-9 &&
                               y >= -1.0e-9 && y <= 1.0 + 1.0e-9;
                    },
                    "Triangle(z=0 平面)");
        }
    }

    std::cout << "\n=== C. 线段单元：偏离轴线的格点必须判为无效 ===\n";
    {
        std::vector<Point> pts = {Point(0.f, 0.f, 0.f), Point(0.5f, 0.f, 0.f), Point(1.f, 0.f, 0.f)};
        std::vector<CellSpec> cells = {{{0, 1}, IG_LINE}, {{1, 2}, IG_LINE}};
        auto mesh = MakeMesh(pts, cells, "field", 1);
        auto out = RunResampleExplicit(mesh);
        Check(out != nullptr, "Line 网格重采样执行成功");
        if (out != nullptr) {
            auto mask = FindArray(out, ResampleToImageFilter::GetMaskArrayName());
            CheckMaskAgainstGeometry(
                    out, mask,
                    [](double x, double y, double z) {
                        return std::fabs(y) <= 1.0e-9 && std::fabs(z) <= 1.0e-9 &&
                               x >= -1.0e-9 && x <= 1.0 + 1.0e-9;
                    },
                    "Line(x 轴)");
        }
    }

    std::cout << "\n=== D. 四边形单元：平面内与倾斜平面均按三维局部坐标判定 ===\n";
    {
        std::vector<Point> pts = {Point(0.f, 0.f, 0.f), Point(1.f, 0.f, 0.f), Point(1.f, 1.f, 0.f),
                                  Point(0.f, 1.f, 0.f)};
        std::vector<CellSpec> cells = {{{0, 1, 2, 3}, IG_QUAD}};
        auto mesh = MakeMesh(pts, cells, "field", 1);
        auto out = RunResampleExplicit(mesh);
        Check(out != nullptr, "Quad 网格重采样执行成功");
        if (out != nullptr) {
            auto mask = FindArray(out, ResampleToImageFilter::GetMaskArrayName());
            CheckMaskAgainstGeometry(
                    out, mask,
                    [](double x, double y, double z) {
                        return std::fabs(z) <= 1.0e-9 && x >= -1.0e-9 && x <= 1.0 + 1.0e-9 &&
                               y >= -1.0e-9 && y <= 1.0 + 1.0e-9;
                    },
                    "Quad(z=0 平面)");
        }
    }
    {
        // 倾斜平面 z = y：只有落在该平面上的格点有效
        std::vector<Point> pts = {Point(0.f, 0.f, 0.f), Point(1.f, 0.f, 0.f), Point(1.f, 1.f, 1.f),
                                  Point(0.f, 1.f, 1.f)};
        std::vector<CellSpec> cells = {{{0, 1, 2, 3}, IG_QUAD}};
        auto mesh = MakeMesh(pts, cells, "field", 1);
        auto out = RunResampleExplicit(mesh);
        Check(out != nullptr, "倾斜 Quad 重采样执行成功");
        if (out != nullptr) {
            auto mask = FindArray(out, ResampleToImageFilter::GetMaskArrayName());
            CheckMaskAgainstGeometry(
                    out, mask,
                    [](double x, double y, double z) {
                        return std::fabs(z - y) <= 1.0e-9 && x >= -1.0e-9 && x <= 1.0 + 1.0e-9 &&
                               y >= -1.0e-9 && y <= 1.0 + 1.0e-9;
                    },
                    "Quad(z=y 倾斜平面)");
        }
    }
}

// ---------------------------------------------------------------------------
// E. 不受支持的单元类型
// ---------------------------------------------------------------------------
static void TestUnsupportedCells() {
    std::cout << "\n=== E. 不受支持的单元类型必须显式提示 / 拒绝 ===\n";
    std::vector<Point> pts = {Point(0.f, 0.f, 0.f), Point(1.f, 0.f, 0.f), Point(0.f, 1.f, 0.f),
                              Point(0.f, 0.f, 1.f), Point(1.f, 0.f, 1.f), Point(0.f, 1.f, 1.f)};
    std::vector<CellSpec> cells = {{{0, 1, 2, 3, 4, 5}, IG_PRISM}};
    auto mesh = MakeMesh(pts, cells, "field", 1);

    {
        auto filter = ResampleToImageFilter::New();
        filter->SetInput(mesh);
        filter->SetSamplingDimensions(4, 4, 4);
        const bool ok = filter->Execute();
        Check(!ok, "默认 FailOnUnsupportedCells=true 时拒绝执行（不产出不完整结果）");
        Check(filter->GetMessage().find("不受支持") != std::string::npos,
              "GetMessage 含“不受支持”提示");
        Check(filter->GetMessage().find("Prism") != std::string::npos, "GetMessage 给出类型名 Prism");
        std::cout << "         message: " << filter->GetMessage() << "\n";
    }
    {
        auto filter = ResampleToImageFilter::New();
        filter->SetInput(mesh);
        filter->SetSamplingDimensions(4, 4, 4);
        filter->SetFailOnUnsupportedCells(false);
        const bool ok = filter->Execute();
        Check(ok, "FailOnUnsupportedCells=false 时继续执行");
        Check(filter->GetMessage().find("不受支持") != std::string::npos,
              "继续执行时仍给出不受支持的告警");
        auto out = DynamicCast<StructuredMesh>(filter->GetOutput(0));
        if (out != nullptr) {
            auto mask = FindArray(out, ResampleToImageFilter::GetMaskArrayName());
            Check(CountValid(out, mask) == 0, "Prism 不被采样 → 全部格点无效（掩膜=0）");
        }
    }
    {
        // 混合网格（Tetra + Prism）：提示中只列出 Prism
        std::vector<Point> p2 = {Point(0.f, 0.f, 0.f), Point(1.f, 0.f, 0.f), Point(0.f, 1.f, 0.f),
                                 Point(0.f, 0.f, 1.f), Point(3.f, 0.f, 0.f), Point(4.f, 0.f, 0.f),
                                 Point(3.f, 1.f, 0.f), Point(3.f, 0.f, 1.f)};
        std::vector<CellSpec> c2 = {{{0, 1, 2, 3}, IG_TETRA}, {{4, 5, 6, 7}, IG_PRISM}};
        auto m2 = MakeMesh(p2, c2, "field", 1);
        auto filter = ResampleToImageFilter::New();
        filter->SetInput(m2);
        filter->SetSamplingDimensions(4, 4, 4);
        filter->SetFailOnUnsupportedCells(false);
        Check(filter->Execute(), "混合网格（Tetra+Prism）在告警模式下可执行");
        Check(filter->GetMessage().find("Prism") != std::string::npos, "混合网格的提示中列出 Prism");
        Check(filter->GetMessage().find("Tetra×") == std::string::npos,
              "提示中不误报受支持的 Tetra（未出现在“不受支持”明细里）");
    }
}

// ---------------------------------------------------------------------------
// F/G. 属性处理策略
// ---------------------------------------------------------------------------
static void TestAttributePolicy() {
    std::cout << "\n=== F/G. 属性策略：类型保留、ID 不插值、同名冲突 ===\n";
    std::vector<Point> pts = {Point(0.f, 0.f, 0.f), Point(1.f, 0.f, 0.f), Point(0.f, 1.f, 0.f),
                              Point(0.f, 0.f, 1.f)};
    std::vector<CellSpec> cells = {{{0, 1, 2, 3}, IG_TETRA}};
    auto mesh = MakeMesh(pts, cells, "dval", 2, "shared");

    {
        auto intArr = IntArray::New();
        intArr->SetName("intval");
        intArr->SetDimension(1);
        intArr->Resize(4);
        for (int i = 0; i < 4; ++i) {
            int v = 10 + i;
            intArr->SetElement(static_cast<IGsize>(i), &v);
        }
        mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, intArr);

        // 名称以 Ids 结尾 → 判定为 ID 类，禁止线性插值；顶点值互不相等以便判别
        auto idArr = FloatArray::New();
        idArr->SetName("PointIds");
        idArr->SetDimension(1);
        idArr->Resize(4);
        const double ids[4] = {100.0, 200.0, 300.0, 400.0};
        for (int i = 0; i < 4; ++i) idArr->SetElement(static_cast<IGsize>(i), &ids[i]);
        mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, idArr);

        // 同名点数组：值为 1（单元同名数组值为 9+，用于验证点数据优先）
        auto sharedPoint = FloatArray::New();
        sharedPoint->SetName("shared");
        sharedPoint->SetDimension(1);
        sharedPoint->Resize(4);
        for (int i = 0; i < 4; ++i) {
            double v = 1.0;
            sharedPoint->SetElement(static_cast<IGsize>(i), &v);
        }
        mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, sharedPoint);
    }

    auto filter = ResampleToImageFilter::New();
    filter->SetInput(mesh);
    filter->SetSamplingDimensions(6, 6, 6);
    Check(filter->Execute(), "含多类型属性的网格执行成功");
    auto out = DynamicCast<StructuredMesh>(filter->GetOutput(0));
    if (out == nullptr) return;
    auto mask = FindArray(out, ResampleToImageFilter::GetMaskArrayName());

    // 存储类型与分量数保留
    auto dval = FindArray(out, "dval");
    Check(dval != nullptr, "输出保留数组 dval");
    Check(dval != nullptr && dval->GetDimension() == 2, "dval 分量数保持 2");
    auto intval = FindArray(out, "intval");
    Check(intval != nullptr && intval->GetArrayType() == IG_IntArray,
          "intval 输出保持 IntArray 类型（不被强制成 float）");
    auto idOut = FindArray(out, "PointIds");
    Check(idOut != nullptr && idOut->GetArrayType() == IG_FloatArray,
          "PointIds 输出保持 FloatArray 类型");

    // 同名冲突：点数据优先，输出中只有一个 shared
    IGsize sharedCount = 0;
    IGenum sharedAttach = IG_POINT;
    {
        auto attrs = out->GetAttributeSet()->GetAllAttributes();
        for (IGsize i = 0; i < attrs->GetNumberOfElements(); ++i) {
            auto& a = attrs->GetElement(i);
            if (a.isDeleted || a.pointer == nullptr) continue;
            if (a.pointer->GetName() == "shared") {
                ++sharedCount;
                sharedAttach = a.attachmentType;
            }
        }
    }
    Check(sharedCount == 1, "同名点/单元数组冲突后只保留一个 shared 数组");
    Check(sharedAttach == IG_POINT, "冲突解决策略 = 点数据优先（IG_POINT）");
    Check(filter->GetMessage().find("同名数组冲突") != std::string::npos,
          "GetMessage 显式提示同名数组冲突");
    if (dval != nullptr) {
        auto shared = FindArray(out, "shared");
        bool allOne = true;
        for (IGsize p = 0; p < out->GetNumberOfPoints(); ++p) {
            if (mask->GetElementValue(p, 0) == 0.0) continue;
            if (shared->GetElementValue(p, 0) != 1.0) allOne = false;
        }
        Check(allOne, "冲突数组取值来自点数据（值为 1，而非单元数据的 9+）");
    }

    // ID 类数组不插值：有效值必须恰好等于某个源点值
    {
        IGsize exact = 0;
        IGsize blended = 0;
        for (IGsize p = 0; p < out->GetNumberOfPoints(); ++p) {
            if (mask->GetElementValue(p, 0) == 0.0) continue;
            const double v = idOut->GetElementValue(p, 0);
            if (v == 100.0 || v == 200.0 || v == 300.0 || v == 400.0) ++exact;
            else ++blended;
        }
        std::cout << "         PointIds exact=" << exact << " blended=" << blended << "\n";
        Check(blended == 0, "默认策略下 ID 类数组未被线性插值（取值全部等于源点值）");
        Check(exact > 0, "ID 类数组确实产生了输出");
    }
    {
        // 对照组：显式允许插值时应出现混合值，证明上面的判别有效
        auto f2 = ResampleToImageFilter::New();
        f2->SetInput(mesh);
        f2->SetSamplingDimensions(6, 6, 6);
        f2->SetDisableInterpolationForDiscreteArrays(false);
        f2->Execute();
        auto out2 = DynamicCast<StructuredMesh>(f2->GetOutput(0));
        auto mask2 = FindArray(out2, ResampleToImageFilter::GetMaskArrayName());
        auto id2 = FindArray(out2, "PointIds");
        IGsize blended = 0;
        for (IGsize p = 0; p < out2->GetNumberOfPoints(); ++p) {
            if (mask2->GetElementValue(p, 0) == 0.0) continue;
            const double v = id2->GetElementValue(p, 0);
            if (!(v == 100.0 || v == 200.0 || v == 300.0 || v == 400.0)) ++blended;
        }
        std::cout << "         (interpolation enabled) blended=" << blended << "\n";
        Check(blended > 0, "关闭“禁止插值”后确实出现插值混合值（判别有效）");
    }
}

// ---------------------------------------------------------------------------
// H. 点集转八叉树：独立复算体素归属与统计量
// ---------------------------------------------------------------------------
static void TestOctreeParity() {
    std::cout << "\n=== H. 点集转八叉树：体素归属与统计量的独立复算 ===\n";

    // 可复现的伪随机点集（LCG），标量场 v = x + 10y + 100z
    const int kNumPoints = 2000;
    std::vector<Point> pts;
    std::vector<double> fieldValues;
    unsigned long long state = 12345ull;
    auto nextRand = [&state]() {
        state = state * 6364136223846793005ull + 1442695040888963407ull;
        return static_cast<double>((state >> 11) & 0x1FFFFFFFFFFFFFull) /
               static_cast<double>(0x1FFFFFFFFFFFFFull);
    };
    for (int i = 0; i < kNumPoints; ++i) {
        const double x = nextRand(), y = nextRand(), z = nextRand();
        pts.push_back(Point(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
        fieldValues.push_back(x + 10.0 * y + 100.0 * z);
    }

    std::vector<CellSpec> noCells;
    auto mesh = MakeMesh(pts, noCells, std::string(), 0);
    {
        auto vArr = FloatArray::New();
        vArr->SetName("v");
        vArr->SetDimension(1);
        vArr->Resize(kNumPoints);
        for (int i = 0; i < kNumPoints; ++i) {
            vArr->SetElement(static_cast<IGsize>(i), &fieldValues[i]);
        }
        mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, vArr);
    }

    auto filter = PointSetToOctreeFilter::New();
    filter->SetInput(mesh);
    filter->SetNumberOfPointsPerCell(1);
    filter->SetProcessInputPointArray(true);
    filter->SetInputPointArrayName("v");
    filter->SetComputeLastValue(false);
    filter->SetComputeMin(true);
    filter->SetComputeMax(true);
    filter->SetComputeCount(true);
    filter->SetComputeSum(false);
    filter->SetComputeMean(true);
    Check(filter->Execute(), "八叉树转换执行成功");
    std::cout << "         message: " << filter->GetMessage() << "\n";

    auto out = DynamicCast<StructuredMesh>(filter->GetOutput(0));
    Check(out != nullptr, "输出为 StructuredMesh");
    if (out == nullptr) return;

    auto octree = FindArray(out, "octree");
    Check(octree != nullptr && octree->GetArrayType() == IG_UnsignedCharArray,
          "octree 单元数组存在且为 unsigned char");

    auto outField = FindArray(out, "v");
    Check(outField != nullptr, "输出包含以输入数组名命名的统计数组 v");
    if (outField == nullptr) return;
    Check(outField->GetDimension() == 5,
          "默认勾选 Min/Max/Count/Mean → 输出 5 个分量（Min,Max,Count,Sum,Mean）");

    // ---- 从输出格点独立推导网格几何 ----
    std::vector<double> xs, ys, zs;
    for (IGsize p = 0; p < out->GetNumberOfPoints(); ++p) {
        const Point& q = out->GetPoint(p);
        xs.push_back(static_cast<double>(q[0]));
        ys.push_back(static_cast<double>(q[1]));
        zs.push_back(static_cast<double>(q[2]));
    }
    auto makeUnique = [](std::vector<double>& v) {
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end(),
                            [](double a, double b) { return std::fabs(a - b) < 1.0e-7; }),
                v.end());
    };
    makeUnique(xs);
    makeUnique(ys);
    makeUnique(zs);

    const int nx = static_cast<int>(xs.size());
    const int ny = static_cast<int>(ys.size());
    const int nz = static_cast<int>(zs.size());
    std::cout << "         grid dims = " << nx << " x " << ny << " x " << nz
              << " (points=" << out->GetNumberOfPoints()
              << " cells=" << out->GetNumberOfCells() << ")\n";
    const double ox = xs.front(), oy = ys.front(), oz = zs.front();
    const double sx = (nx > 1) ? (xs.back() - xs.front()) / (nx - 1) : 1.0;
    const double sy = (ny > 1) ? (ys.back() - ys.front()) / (ny - 1) : 1.0;
    const double sz = (nz > 1) ? (zs.back() - zs.front()) / (nz - 1) : 1.0;

    const int extentX = std::max(1, nx - 1);
    const int extentY = std::max(1, ny - 1);
    const IGsize nCells = out->GetNumberOfCells();
    Check(nCells == static_cast<IGsize>(extentX) * extentY * std::max(1, nz - 1),
          "单元数 = (nx-1)(ny-1)(nz-1)");

    // ---- 按 VTK 的公式独立复算每个点的体素归属与统计量 ----
    auto ijkOf = [](double v, double o, double s, int n) {
        int k = (s > 0.0) ? static_cast<int>(std::floor((v - o) / s + 1.0e-9)) : 0;
        if (k < 0) k = 0;
        if (n >= 2 && k > n - 2) k = n - 2;
        if (n < 2) k = 0;
        return k;
    };

    std::vector<double> refCount(nCells, 0.0);
    std::vector<double> refSum(nCells, 0.0);
    std::vector<double> refMin(nCells, 1.0e300);
    std::vector<double> refMax(nCells, -1.0e300);
    for (int i = 0; i < kNumPoints; ++i) {
        const int i0 = ijkOf(pts[i][0], ox, sx, nx);
        const int j0 = ijkOf(pts[i][1], oy, sy, ny);
        const int k0 = ijkOf(pts[i][2], oz, sz, nz);
        const IGsize cid = static_cast<IGsize>(i0) + static_cast<IGsize>(j0) * extentX +
                           static_cast<IGsize>(k0) * extentX * extentY;
        if (cid >= nCells) continue;
        const double v = fieldValues[i];
        refCount[cid] += 1.0;
        refSum[cid] += v;
        if (v < refMin[cid]) refMin[cid] = v;
        if (v > refMax[cid]) refMax[cid] = v;
    }

    IGsize countMismatch = 0, minMismatch = 0, maxMismatch = 0, sumMismatch = 0, meanMismatch = 0;
    IGsize selfInconsistent = 0;
    IGsize nonEmptyCells = 0;
    for (IGsize c = 0; c < nCells; ++c) {
        const double cnt = outField->GetElementValue(c, 2);
        const double sum = outField->GetElementValue(c, 3);
        const double mean = outField->GetElementValue(c, 4);
        const double mn = outField->GetElementValue(c, 0);
        const double mx = outField->GetElementValue(c, 1);
        if (refCount[c] != cnt) ++countMismatch;
        if (refCount[c] > 0) {
            ++nonEmptyCells;
            if (std::fabs(mean * cnt - sum) > 1.0e-3 || mn > mx + 1.0e-6 ||
                mean < mn - 1.0e-6 || mean > mx + 1.0e-6) {
                ++selfInconsistent;
            }
            if (std::fabs(mn - refMin[c]) > 1.0e-5) ++minMismatch;
            if (std::fabs(mx - refMax[c]) > 1.0e-5) ++maxMismatch;
            if (std::fabs(sum - refSum[c]) > 1.0e-3) ++sumMismatch;
            if (std::fabs(mean - refSum[c] / refCount[c]) > 1.0e-4) ++meanMismatch;
        }
    }
    std::cout << "         nonEmptyCells=" << nonEmptyCells << " countMismatch=" << countMismatch
              << " minMismatch=" << minMismatch << " maxMismatch=" << maxMismatch
              << " sumMismatch=" << sumMismatch << " meanMismatch=" << meanMismatch
              << " selfInconsistent=" << selfInconsistent << "\n";
    Check(selfInconsistent == 0, "每体素内部自洽：mean*count=sum 且 min<=mean<=max");
    Check(countMismatch == 0, "每体素 Count 与独立复算一致（体素归属与 VTK 公式一致）");
    Check(minMismatch == 0, "每体素 Min 与独立复算一致");
    Check(maxMismatch == 0, "每体素 Max 与独立复算一致");
    Check(sumMismatch == 0, "每体素 Sum 与独立复算一致");
    Check(meanMismatch == 0, "每体素 Mean = Sum/Count 与独立复算一致");

    double totalCount = 0.0;
    for (IGsize c = 0; c < nCells; ++c) totalCount += outField->GetElementValue(c, 2);
    std::cout << "         totalCount=" << totalCount << " (input " << kNumPoints << ")\n";
    Check(std::fabs(totalCount - static_cast<double>(kNumPoints)) < 0.5,
          "所有体素 Count 之和 = 输入点数");

    // ---- VTK 特有的 octree 8 位占用编码：以「体素中心」为界，位权 1/2/4/16 按位或累加 ----
    {
        std::vector<unsigned char> refOctree(nCells, 0);
        for (int i = 0; i < kNumPoints; ++i) {
            const int i0 = ijkOf(pts[i][0], ox, sx, nx);
            const int j0 = ijkOf(pts[i][1], oy, sy, ny);
            const int k0 = ijkOf(pts[i][2], oz, sz, nz);
            const IGsize cid = static_cast<IGsize>(i0) + static_cast<IGsize>(j0) * extentX +
                               static_cast<IGsize>(k0) * extentX * extentY;
            if (cid >= nCells) continue;
            // 体素中心 = 原点 + ijk*spacing + spacing/2（VTK 在中心处比较，而非格点处）
            const double cx = ox + i0 * sx + 0.5 * sx;
            const double cy = oy + j0 * sy + 0.5 * sy;
            const double cz = oz + k0 * sz + 0.5 * sz;
            unsigned int v = (pts[i][0] > cx ? 2u : 1u);
            v *= (pts[i][1] > cy ? 4u : 1u);
            v *= (pts[i][2] > cz ? 16u : 1u);
            refOctree[cid] |= static_cast<unsigned char>(v);
        }
        IGsize octMismatch = 0;
        for (IGsize c = 0; c < nCells; ++c) {
            const unsigned char got = static_cast<unsigned char>(octree->GetElementValue(c, 0));
            if (got != refOctree[c]) ++octMismatch;
        }
        Check(octMismatch == 0,
              "octree 占用位编码与独立复算一致（以体素中心为界、位权 1/2/4/16 按位或）");
    }

    // ---- 空体素的默认值：VTK 为 Min=FLT_MAX、Max=FLT_LOWEST、Count/Sum/Mean=0 ----
    {
        const double fMax = static_cast<double>(std::numeric_limits<float>::max());
        const double fLow = static_cast<double>(std::numeric_limits<float>::lowest());
        IGsize emptyCells = 0;
        IGsize badDefaults = 0;
        for (IGsize c = 0; c < nCells; ++c) {
            if (outField->GetElementValue(c, 2) != 0.0) continue; // Count != 0 即非空
            ++emptyCells;
            const bool ok = outField->GetElementValue(c, 0) == fMax &&
                            outField->GetElementValue(c, 1) == fLow &&
                            outField->GetElementValue(c, 3) == 0.0 &&
                            outField->GetElementValue(c, 4) == 0.0;
            if (!ok) ++badDefaults;
        }
        std::cout << "         emptyCells=" << emptyCells << " badDefaults=" << badDefaults << "\n";
        Check(emptyCells > 0, "存在空体素（用于验证默认值语义）");
        Check(badDefaults == 0,
              "空体素默认值与 VTK 一致（Min=FLT_MAX、Max=FLT_LOWEST、Count/Sum/Mean=0，非 NaN）");
    }

    // 关闭「处理点属性」时只输出 octree 编码
    {
        auto f2 = PointSetToOctreeFilter::New();
        f2->SetInput(mesh);
        f2->SetNumberOfPointsPerCell(1);
        f2->SetProcessInputPointArray(false);
        f2->Execute();
        auto o2 = DynamicCast<StructuredMesh>(f2->GetOutput(0));
        Check(FindArray(o2, "v") == nullptr, "关闭“处理点属性”时不输出统计数组");
        Check(FindArray(o2, "octree") != nullptr, "关闭“处理点属性”时仍输出 octree 编码数组");
    }

    // 「每体素平均点数」参数必须改变输出规模
    {
        auto f3 = PointSetToOctreeFilter::New();
        f3->SetInput(mesh);
        f3->SetNumberOfPointsPerCell(10);
        f3->Execute();
        auto o3 = DynamicCast<StructuredMesh>(f3->GetOutput(0));
        Check(o3 != nullptr && o3->GetNumberOfCells() < nCells,
              "每体素平均点数=10 时输出体素数显著减少（参数生效）");
        std::cout << "         cells(per=1)=" << nCells
                  << " cells(per=10)=" << (o3 ? o3->GetNumberOfCells() : 0) << "\n";
    }
}

// ---------------------------------------------------------------------------
// I. 线性场再现：同时验证「单元搜索」与「插值权重」
//    四面体上使用线性场 f = 3x + 5y + 7z，任意有效格点的插值结果必须精确等于
//    该点处的解析值——这是最能暴露权重错误的判据（权重错则偏差量级为 O(1)）。
// ---------------------------------------------------------------------------
static void TestLinearReproduction() {
    std::cout << "\n=== I. 线性场再现（四面体 + f = 3x+5y+7z，验证插值权重）===\n";
    std::vector<Point> pts = {Point(0.f, 0.f, 0.f), Point(1.f, 0.f, 0.f), Point(0.f, 1.f, 0.f),
                              Point(0.f, 0.f, 1.f)};
    std::vector<CellSpec> cells = {{{0, 1, 2, 3}, IG_TETRA}};
    auto mesh = MakeMesh(pts, cells, std::string(), 0);
    {
        auto lin = FloatArray::New();
        lin->SetName("lin");
        lin->SetDimension(1);
        lin->Resize(4);
        for (int i = 0; i < 4; ++i) {
            const double v = 3.0 * pts[i][0] + 5.0 * pts[i][1] + 7.0 * pts[i][2];
            lin->SetElement(static_cast<IGsize>(i), &v);
        }
        mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, lin);
    }

    auto out = RunResampleExplicit(mesh);
    Check(out != nullptr, "线性场网格重采样执行成功");
    if (out == nullptr) return;
    auto mask = FindArray(out, ResampleToImageFilter::GetMaskArrayName());
    auto lin = FindArray(out, "lin");
    Check(lin != nullptr && lin->GetArrayType() == IG_FloatArray, "输出保留线性场数组 lin");

    IGsize valid = 0, bad = 0;
    double worst = 0.0;
    for (IGsize p = 0; p < out->GetNumberOfPoints(); ++p) {
        if (mask->GetElementValue(p, 0) == 0.0) continue;
        ++valid;
        const Point& q = out->GetPoint(p);
        const double expect = 3.0 * static_cast<double>(q[0]) + 5.0 * static_cast<double>(q[1]) +
                              7.0 * static_cast<double>(q[2]);
        const double got = lin->GetElementValue(p, 0);
        const double diff = std::fabs(got - expect);
        if (diff > worst) worst = diff;
        if (diff > 1.0e-3) ++bad;
    }
    std::cout << "         validPoints=" << valid << " worstAbsError=" << worst << "\n";
    Check(valid > 0, "存在有效格点用于验证插值");
    Check(bad == 0, "线性场被精确再现（单元搜索与插值权重正确）");
}

// ---------------------------------------------------------------------------
// J. 线性场再现：六面体（与 VTK 的 vtkHexahedron 权重路径对照）
// ---------------------------------------------------------------------------
static void TestLinearReproductionHexahedron() {
    std::cout << "\n=== J. 线性场再现（六面体 + f = 3x+5y+7z）===\n";
    std::vector<Point> pts = {Point(0.f, 0.f, 0.f), Point(1.f, 0.f, 0.f), Point(1.f, 1.f, 0.f),
                              Point(0.f, 1.f, 0.f), Point(0.f, 0.f, 1.f), Point(1.f, 0.f, 1.f),
                              Point(1.f, 1.f, 1.f), Point(0.f, 1.f, 1.f)};
    std::vector<CellSpec> cells = {{{0, 1, 2, 3, 4, 5, 6, 7}, IG_HEXAHEDRON}};
    auto mesh = MakeMesh(pts, cells, std::string(), 0);
    {
        auto lin = FloatArray::New();
        lin->SetName("lin");
        lin->SetDimension(1);
        lin->Resize(8);
        for (int i = 0; i < 8; ++i) {
            const double v = 3.0 * pts[i][0] + 5.0 * pts[i][1] + 7.0 * pts[i][2];
            lin->SetElement(static_cast<IGsize>(i), &v);
        }
        mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, lin);
    }

    auto filter = ResampleToImageFilter::New();
    filter->SetInput(mesh);
    filter->SetSamplingDimensions(3, 3, 3);
    filter->SetUseInputBounds(false);
    filter->SetSamplingBounds(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
    Check(filter->Execute(), "六面体网格重采样执行成功");
    auto out = DynamicCast<StructuredMesh>(filter->GetOutput(0));
    if (out == nullptr) return;
    auto mask = FindArray(out, ResampleToImageFilter::GetMaskArrayName());
    auto lin = FindArray(out, "lin");

    IGsize valid = 0, bad = 0;
    double worst = 0.0;
    for (IGsize p = 0; p < out->GetNumberOfPoints(); ++p) {
        if (mask->GetElementValue(p, 0) == 0.0) continue;
        ++valid;
        const Point& q = out->GetPoint(p);
        const double expect = 3.0 * static_cast<double>(q[0]) + 5.0 * static_cast<double>(q[1]) +
                              7.0 * static_cast<double>(q[2]);
        const double diff = std::fabs(lin->GetElementValue(p, 0) - expect);
        if (diff > worst) worst = diff;
        if (diff > 1.0e-3) ++bad;
    }
    std::cout << "         validPoints=" << valid << " worstAbsError=" << worst << "\n";
    Check(valid == 27, "单位立方体内 3^3 采样格点全部有效");
    Check(bad == 0, "六面体线性场被精确再现");
}

int main() {
    TestTetraBaseline();
    TestLowDimensionalCells();
    TestUnsupportedCells();
    TestAttributePolicy();
    TestLinearReproduction();
    TestLinearReproductionHexahedron();
    TestOctreeParity();

    std::cout << "\n================ 汇总 ================\n";
    std::cout << "通过 " << (g_total - g_failed) << " / " << g_total << "，失败 " << g_failed << "\n";
    return (g_failed == 0) ? 0 : 1;
}
