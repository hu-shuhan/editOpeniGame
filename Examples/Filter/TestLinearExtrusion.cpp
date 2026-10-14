// 集成提交：feat: integrate additional standard filters
// 查询该提交：git log --diff-filter=A --format="%h %s" -- Examples/Filter/TestLinearExtrusion.cpp
// 集成回归（待提交）：迁移标准过滤器时，防止仅复制菜单而遗漏算法或测试资产。
// 保留源仓库测试的几何、属性与边界断言；交互示例仅用于人工检查。
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <iGameCellArray.h>
#include <iGameCellType.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePoints.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <Modeling/iGameLinearExtrusionFilter.h>

// LinearExtrusionFilter（线性拉伸）控制台用例。
//
// 语义基线：VTK Filters/Modeling/vtkLinearExtrusionFilter.cxx
//   - 每个输入点按 ScaleFactor * 偏移复制出第二层点，输出点数为 2*N；
//   - 顶点 -> 线；线/折线的每段 -> 四点带；面 -> Capping 两端面 + 自由边侧裙；
//   - 面数满足 2*F + B（Capping 开）或 B（Capping 关），B 为自由边数；
//   - 内部共享边不生成侧裙；单元属性按来源单元复制，旧法向不复制。
//
// 模型（Examples/Models，构建时自动拷贝到 Examples 构建目录的 Models 下）：
//   LinearExtrusion_OpenSurface.vtk：4x4 开放四边形片，25 点 / 16 面 / 自由边 B=16，
//                                    带点属性 point_id(0..24) 与单元属性 patch_id；
//   LinearExtrusion_ClosedCube.vtk ：封闭立方体，8 点 / 12 三角面 / B=0，
//                                    带点属性 point_id(0..7) 与单元属性 face_id。
//
// 全部通过时逐行打印 PASS 并以退出码 0 结束；任一失败打印 FAIL 并返回 1。
// 在 Examples 构建目录下运行（模型用相对路径 ./Models/...），也可用第一个参数覆盖模型目录。
//
// 注意：单元类型常量（IG_LINE / IG_QUAD / IG_TETRA ...）在本文件里必须写全限定名
// iGame::IG_xxx —— iGame 命名空间下有同名符号会遮蔽全局枚举。

namespace ig = iGame;

namespace {

int g_FailureCount = 0;

void Check(bool ok, const std::string& what) {
    if (ok) {
        std::cout << "PASS: " << what << std::endl;
    } else {
        std::cout << "FAIL: " << what << std::endl;
        ++g_FailureCount;
    }
}

void CheckNear(double actual, double expected, double tolerance, const std::string& what) {
    const bool ok = std::fabs(actual - expected) <= tolerance;
    Check(ok, what);
    if (!ok) { std::cout << "      actual=" << actual << " expected=" << expected << std::endl; }
}

// ---------------------------------------------------------------------------
// 独立读取模型：每个用例都有自己的输入对象，避免上一用例的结果影响下一用例
// ---------------------------------------------------------------------------
ig::DataObject::Pointer LoadModel(const std::string& fileName) {
    auto object = ig::FileIO::ReadFile(fileName);
    if (object == nullptr) { std::cout << "FAIL: cannot read model " << fileName << std::endl; }
    return object;
}

// 按名字与挂载类型取属性数组：GetScalar(name) 不区分挂载类型，点/单元同名时会取错
ig::ArrayObject* FindArray(ig::DataObject::Pointer object, IGenum attachmentType, const std::string& name) {
    if (object == nullptr || object->GetAttributeSet() == nullptr) { return nullptr; }
    auto attributes = (attachmentType == IG_POINT) ? object->GetAttributeSet()->GetAllPointAttributes()
                                                  : object->GetAttributeSet()->GetAllCellAttributes();
    if (attributes == nullptr) { return nullptr; }
    for (IGsize i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto& attribute = attributes->GetElement(i);
        if (attribute.IsNone() || attribute.pointer == nullptr) { continue; }
        if (attribute.pointer->GetName() == name) { return attribute.pointer; }
    }
    return nullptr;
}

// 统计指定类型的输出单元个数
IGsize CountCellsOfType(ig::UnstructuredMesh::Pointer mesh, IGenum type) {
    IGsize count = 0;
    for (IGsize cellId = 0; cellId < mesh->GetNumberOfCells(); ++cellId) {
        if (mesh->GetCellType(cellId) == type) { ++count; }
    }
    return count;
}

// 输入多边形网格里只被一个单元使用的边（自由边）数：与 filter 的 2*F+B / B 计数对应
IGsize CountBoundaryEdges(ig::CellArray::Pointer cells, IGsize numberOfCells) {
    std::vector<std::pair<igIndex, igIndex>> edges;
    for (IGsize cellId = 0; cellId < numberOfCells; ++cellId) {
        const igIndex* ids = nullptr;
        const int count = cells->GetCellIds(cellId, ids);
        if (count < 3 || ids == nullptr) { continue; }
        for (int i = 0; i < count; ++i) {
            igIndex a = ids[i];
            igIndex b = ids[(i + 1) % count];
            if (a == b) { continue; }
            if (a > b) { std::swap(a, b); }
            edges.emplace_back(a, b);
        }
    }
    std::sort(edges.begin(), edges.end());
    IGsize boundary = 0;
    for (size_t i = 0; i < edges.size();) {
        size_t j = i;
        while (j < edges.size() && edges[j] == edges[i]) { ++j; }
        if (j - i == 1) { ++boundary; }
        i = j;
    }
    return boundary;
}

// 读回输入模型的几何规模与自由边数，用于把断言期望值算出来而不是写死
struct ModelInfo {
    IGsize numberOfPoints{0};
    IGsize numberOfCells{0};
    IGsize boundaryEdges{0};
    ig::Points::Pointer points;
    ig::CellArray::Pointer cells;
};

bool ReadModelInfo(ig::DataObject::Pointer object, ModelInfo& info) {
    auto pointSet = ig::DynamicCast<ig::PointSet>(object);
    if (pointSet == nullptr || pointSet->GetPoints() == nullptr) { return false; }
    info.points = pointSet->GetPoints();
    info.numberOfPoints = info.points->GetNumberOfPoints();

    // SurfaceMesh 与 UnstructuredMesh 都能通过 DataObject 接口取到 CellArray
    info.cells = object->GetCellArray();
    if (info.cells == nullptr) { return false; }
    info.numberOfCells = info.cells->GetNumberOfCells();
    info.boundaryEdges = CountBoundaryEdges(info.cells, info.numberOfCells);
    return true;
}

// ---------------------------------------------------------------------------
// 用例 1：开放曲面 + Vector 模式 + Capping 开
// 期望：点数 2N；面数 2F+B；第二层整体位移 ScaleFactor * Vector；
//       点属性 point_id 两层各出现一次；单元属性 patch_id 覆盖全部输出单元。
// ---------------------------------------------------------------------------
void TestOpenSurfaceVectorExtrusion(const std::string& modelPath) {
    std::cout << "--- case 1: open surface, Vector mode, Capping on ---" << std::endl;

    auto input = LoadModel(modelPath);
    if (input == nullptr) { return; }
    ModelInfo info;
    if (!ReadModelInfo(input, info)) {
        Check(false, "read input geometry");
        return;
    }
    std::cout << "      input: points=" << info.numberOfPoints << " cells=" << info.numberOfCells
              << " boundaryEdges=" << info.boundaryEdges << std::endl;

    const double scale = 2.0;
    auto filter = ig::LinearExtrusionFilter::New();
    filter->SetExtrusionTypeToVectorExtrusion();
    filter->SetVector(0.0, 0.0, 1.0);
    filter->SetScaleFactor(scale);
    filter->CappingOn();
    filter->SetInput(input);

    if (!filter->Execute()) {
        Check(false, "Execute() succeeds");
        std::cout << "      message: " << filter->GetMessage() << std::endl;
        return;
    }
    Check(true, "Execute() succeeds");

    auto output = ig::DynamicCast<ig::UnstructuredMesh>(filter->GetOutput());
    if (output == nullptr) {
        Check(false, "output is an UnstructuredMesh");
        return;
    }
    Check(output->GetPoints() != nullptr, "output has points");

    // 点数 = 2N
    Check(output->GetNumberOfPoints() == 2 * info.numberOfPoints,
          "output point count == 2 * input point count (" + std::to_string(2 * info.numberOfPoints) + ")");

    // 面数 = 2F + B
    const IGsize expectedCells = 2 * info.numberOfCells + info.boundaryEdges;
    Check(output->GetNumberOfCells() == expectedCells,
          "output cell count == 2*F + B (" + std::to_string(expectedCells) + ")");

    // 两层坐标：第一层原样，第二层 = 原坐标 + ScaleFactor * Vector
    bool firstLayerOk = true;
    bool secondLayerOk = true;
    for (IGsize i = 0; i < info.numberOfPoints; ++i) {
        ig::Vector3d original;
        info.points->GetPoint(i, original);
        ig::Vector3d first;
        output->GetPoints()->GetPoint(i, first);
        ig::Vector3d second;
        output->GetPoints()->GetPoint(i + info.numberOfPoints, second);
        for (int d = 0; d < 3; ++d) {
            if (std::fabs(first[d] - original[d]) > 1e-5) { firstLayerOk = false; }
            const double expected = original[d] + ((d == 2) ? scale : 0.0);
            if (std::fabs(second[d] - expected) > 1e-5) { secondLayerOk = false; }
        }
    }
    Check(firstLayerOk, "first layer keeps the input coordinates");
    Check(secondLayerOk, "second layer is displaced by ScaleFactor * Vector == (0,0,2)");

    // 输入是四边形片：输出单元应全部是四边形，不会凭空产生线单元
    Check(CountCellsOfType(output, iGame::IG_QUAD) == output->GetNumberOfCells(),
          "every output cell is a quad (input is a quad sheet)");
    Check(CountCellsOfType(output, iGame::IG_LINE) == 0, "no line cell is produced for a polygon-only input");

    // 单元顺序与来源：每个二维单元「原端面 + 位移端面」成对输出，即前 2F 个单元里
    // 偶数下标是原端面、奇数下标是紧跟在它后面的位移端面（连接表整体 +N）；
    // 侧裙排在最后 B 个位置。注意侧裙同样会用到第二层点，所以判断位移端面必须限定在 2F 之内。
    {
        bool capPairsOk = true;
        for (IGsize faceId = 0; faceId < info.numberOfCells; ++faceId) {
            const igIndex* originalIds = nullptr;
            const igIndex* displacedIds = nullptr;
            const int originalCount = output->GetCells()->GetCellIds(2 * faceId, originalIds);
            const int displacedCount = output->GetCells()->GetCellIds(2 * faceId + 1, displacedIds);
            if (originalCount < 3 || originalCount != displacedCount || originalIds == nullptr ||
                displacedIds == nullptr) {
                capPairsOk = false;
                break;
            }
            for (int k = 0; k < originalCount; ++k) {
                if (displacedIds[k] != originalIds[k] + static_cast<igIndex>(info.numberOfPoints)) {
                    capPairsOk = false;
                    break;
                }
            }
            if (!capPairsOk) { break; }
        }
        Check(capPairsOk, "each displaced cap follows its original cap with connectivity shifted by N");

        IGsize skirtCount = 0;
        for (IGsize cellId = 2 * info.numberOfCells; cellId < output->GetNumberOfCells(); ++cellId) {
            const igIndex* ids = nullptr;
            const int count = output->GetCells()->GetCellIds(cellId, ids);
            if (count == 4 && ids != nullptr) { ++skirtCount; }
        }
        Check(skirtCount == info.boundaryEdges,
              "the trailing cells are the boundary skirts (" + std::to_string(info.boundaryEdges) + " quads)");

        // 侧裙的顶点顺序必须是「环状」的，不能是自交的蝴蝶结顺序：
        // iGame 把四边形按「从 0 号点出发的扇形」拆成 (c0,c1,c2) 与 (c0,c2,c3)（见 iGameSurfaceMesh.cpp），
        // 若顺序写成 c0,c1,c3,c2 这种自交排列，两个三角形会互相重叠并各留一条缝，
        // 渲染出来就是侧面上的条状空隙。
        // 判断方法：环状四边形的两条对角线 (c0,c2) 与 (c1,c3) 必然相交，叉积非零；
        // 蝴蝶结顺序的两条对角线恰好互相平行，叉积为零。这里正是靠这一点抓回归。
        bool skirtCyclicOrderOk = true;
        IGsize skirtChecked = 0;
        for (IGsize cellId = 2 * info.numberOfCells; cellId < output->GetNumberOfCells(); ++cellId) {
            const igIndex* ids = nullptr;
            const int count = output->GetCells()->GetCellIds(cellId, ids);
            if (count != 4 || ids == nullptr) { continue; }
            ++skirtChecked;

            ig::Vector3d p[4];
            for (int k = 0; k < 4; ++k) { output->GetPoints()->GetPoint(ids[k], p[k]); }
            const ig::Vector3d diagonal02 = p[2] - p[0];
            const ig::Vector3d diagonal13 = p[3] - p[1];
            if (diagonal02.cross(diagonal13).norm() < 1e-9) {
                skirtCyclicOrderOk = false;
                break;
            }
        }
        Check(skirtCyclicOrderOk,
              "skirt quads are in cyclic order, not self-intersecting (" + std::to_string(skirtChecked) + " quads)");
    }

    // 点属性：长度 = 输出点数，两层都等于来源点的 point_id，且类型保持
    {
        auto inputPointIds = FindArray(input, IG_POINT, "point_id");
        auto outputPointIds = FindArray(output, IG_POINT, "point_id");
        Check(inputPointIds != nullptr && outputPointIds != nullptr, "point attribute point_id is propagated");
        if (inputPointIds != nullptr && outputPointIds != nullptr) {
            Check(outputPointIds->GetNumberOfElements() == output->GetNumberOfPoints(),
                  "output point_id length == output point count");
            Check(outputPointIds->GetArrayType() == inputPointIds->GetArrayType(),
                  "output point_id keeps the input array type");
            bool valuesOk = true;
            for (IGsize i = 0; i < info.numberOfPoints; ++i) {
                const double expected = inputPointIds->GetValue(i);
                if (outputPointIds->GetValue(i) != expected) { valuesOk = false; }
                if (outputPointIds->GetValue(i + info.numberOfPoints) != expected) { valuesOk = false; }
            }
            Check(valuesOk, "each input point_id value appears once in each layer");
        }
    }

    // 单元属性：长度 = 输出单元数；每个面的原端面与位移端面都取该面的值
    // （端面成对输出：第 2i 个是面 i 的原端面，第 2i+1 个是它的位移端面）
    {
        auto inputPatchIds = FindArray(input, IG_CELL, "patch_id");
        auto outputPatchIds = FindArray(output, IG_CELL, "patch_id");
        Check(inputPatchIds != nullptr && outputPatchIds != nullptr, "cell attribute patch_id is propagated");
        if (inputPatchIds != nullptr && outputPatchIds != nullptr) {
            Check(outputPatchIds->GetNumberOfElements() == output->GetNumberOfCells(),
                  "output patch_id length == output cell count");
            bool capsOk = true;
            for (IGsize faceId = 0; faceId < info.numberOfCells; ++faceId) {
                const double expected = inputPatchIds->GetValue(faceId);
                if (outputPatchIds->GetValue(2 * faceId) != expected) { capsOk = false; }
                if (outputPatchIds->GetValue(2 * faceId + 1) != expected) { capsOk = false; }
            }
            Check(capsOk, "both caps of a face keep that face's patch_id");
        }
    }

    // 输入不变性
    Check(info.points->GetNumberOfPoints() == info.numberOfPoints &&
                  info.cells->GetNumberOfCells() == info.numberOfCells,
          "input geometry is not modified");
}

// ---------------------------------------------------------------------------
// 用例 2：同一模型 Capping 关
// 期望：点数仍为 2N；面数 = B（只有自由边侧裙，没有端面）。
// ---------------------------------------------------------------------------
void TestOpenSurfaceWithoutCapping(const std::string& modelPath) {
    std::cout << "--- case 2: open surface, Capping off ---" << std::endl;

    auto input = LoadModel(modelPath);
    if (input == nullptr) { return; }
    ModelInfo info;
    if (!ReadModelInfo(input, info)) {
        Check(false, "read input geometry");
        return;
    }

    auto filter = ig::LinearExtrusionFilter::New();
    filter->SetExtrusionTypeToVectorExtrusion();
    filter->SetVector(0.0, 0.0, 1.0);
    filter->CappingOff();
    filter->SetInput(input);
    Check(filter->Execute(), "Execute() succeeds with Capping off");

    auto output = ig::DynamicCast<ig::UnstructuredMesh>(filter->GetOutput());
    if (output == nullptr) {
        Check(false, "output is an UnstructuredMesh");
        return;
    }
    Check(output->GetNumberOfPoints() == 2 * info.numberOfPoints, "point count stays 2N");
    Check(output->GetNumberOfCells() == info.boundaryEdges,
          "cell count == B (" + std::to_string(info.boundaryEdges) + "), no cap is produced");
}

// ---------------------------------------------------------------------------
// 用例 3：封闭曲面（无自由边）
// 期望：Capping 开 -> 2F 个端面、0 个侧裙；Capping 关 -> 没有任何输出单元。
// 这是文档里必须说明的边界情形，对齐 VTK 的循环结构（没有边界边就没有 skirt）。
// ---------------------------------------------------------------------------
void TestClosedSurface(const std::string& modelPath) {
    std::cout << "--- case 3: closed surface (no boundary edge) ---" << std::endl;

    ModelInfo info;
    {
        auto input = LoadModel(modelPath);
        if (input == nullptr) { return; }
        if (!ReadModelInfo(input, info)) {
            Check(false, "read input geometry");
            return;
        }
        Check(info.boundaryEdges == 0, "model really is closed (B == 0)");
        std::cout << "      input: points=" << info.numberOfPoints << " cells=" << info.numberOfCells
                  << " boundaryEdges=" << info.boundaryEdges << std::endl;
    }

    // Capping 开：只有两层端面
    {
        auto input = LoadModel(modelPath);
        if (input == nullptr) { return; }
        auto filter = ig::LinearExtrusionFilter::New();
        filter->SetExtrusionTypeToVectorExtrusion();
        filter->SetVector(0.0, 0.0, 1.0);
        filter->CappingOn();
        filter->SetInput(input);
        Check(filter->Execute(), "Execute() succeeds on a closed surface (Capping on)");
        auto output = ig::DynamicCast<ig::UnstructuredMesh>(filter->GetOutput());
        if (output != nullptr) {
            Check(output->GetNumberOfPoints() == 2 * info.numberOfPoints, "point count == 2N");
            Check(output->GetNumberOfCells() == 2 * info.numberOfCells,
                  "cell count == 2F (" + std::to_string(2 * info.numberOfCells) + "), no skirt exists");
        } else {
            Check(false, "output is an UnstructuredMesh");
        }
    }

    // Capping 关：没有端面也没有侧裙，输出 0 个单元（Execute 仍成功）
    {
        auto input = LoadModel(modelPath);
        if (input == nullptr) { return; }
        auto filter = ig::LinearExtrusionFilter::New();
        filter->SetExtrusionTypeToVectorExtrusion();
        filter->SetVector(0.0, 0.0, 1.0);
        filter->CappingOff();
        filter->SetInput(input);
        Check(filter->Execute(), "Execute() still succeeds on a closed surface with Capping off");
        auto output = ig::DynamicCast<ig::UnstructuredMesh>(filter->GetOutput());
        if (output != nullptr) {
            Check(output->GetNumberOfPoints() == 2 * info.numberOfPoints,
                  "point layer is still duplicated even when no cell is produced");
            Check(output->GetNumberOfCells() == 0, "cell count == 0 (closed surface + Capping off)");
        } else {
            Check(false, "output is an UnstructuredMesh");
        }
    }
}

// ---------------------------------------------------------------------------
// 用例 4：三种位移模式的数值语义
//   Vector：位移 = ScaleFactor * Vector
//   Point ：位移 = ScaleFactor * (p - ExtrusionPoint)
//   Normal：位移 = ScaleFactor * 点法向；输入无点法向时回退 Vector 并给出 GetMessage()
// ---------------------------------------------------------------------------
void TestExtrusionModes(const std::string& modelPath) {
    std::cout << "--- case 4: Vector / Point / Normal displacement semantics ---" << std::endl;

    // Point 模式：以原点为基点、ScaleFactor = 2 => 位移后的点 = 3p
    {
        auto input = LoadModel(modelPath);
        if (input == nullptr) { return; }
        ModelInfo info;
        if (!ReadModelInfo(input, info)) { return; }

        auto filter = ig::LinearExtrusionFilter::New();
        filter->SetExtrusionTypeToPointExtrusion();
        filter->SetExtrusionPoint(0.0, 0.0, 0.0);
        filter->SetScaleFactor(2.0);
        filter->SetInput(input);
        Check(filter->Execute(), "Point mode: Execute() succeeds");
        auto output = ig::DynamicCast<ig::UnstructuredMesh>(filter->GetOutput());
        if (output != nullptr) {
            bool ok = true;
            for (IGsize i = 0; i < info.numberOfPoints; ++i) {
                ig::Vector3d original;
                info.points->GetPoint(i, original);
                ig::Vector3d moved;
                output->GetPoints()->GetPoint(i + info.numberOfPoints, moved);
                for (int d = 0; d < 3; ++d) {
                    if (std::fabs(moved[d] - 3.0 * original[d]) > 1e-5) { ok = false; }
                }
            }
            Check(ok, "Point mode: displacement == ScaleFactor * (p - ExtrusionPoint)");
        } else {
            Check(false, "Point mode: output is an UnstructuredMesh");
        }
    }

    // 负 ScaleFactor：沿反方向拉伸，点仍然分两层
    {
        auto input = LoadModel(modelPath);
        if (input == nullptr) { return; }
        auto filter = ig::LinearExtrusionFilter::New();
        filter->SetExtrusionTypeToVectorExtrusion();
        filter->SetVector(0.0, 0.0, 1.0);
        filter->SetScaleFactor(-1.5);
        filter->SetInput(input);
        Check(filter->Execute(), "negative ScaleFactor: Execute() succeeds");
        auto output = ig::DynamicCast<ig::UnstructuredMesh>(filter->GetOutput());
        if (output != nullptr) {
            ig::Vector3d point;
            output->GetPoints()->GetPoint(0, point);
            ig::Vector3d moved;
            output->GetPoints()->GetPoint(output->GetNumberOfPoints() / 2, moved);
            CheckNear(moved[2] - point[2], -1.5, 1e-5, "negative ScaleFactor moves the layer downward");
        }
    }

    // Normal 模式但没有点法向：回退到 Vector 模式，成功且给出提示
    {
        auto input = LoadModel(modelPath);
        if (input == nullptr) { return; }
        ModelInfo info;
        if (!ReadModelInfo(input, info)) { return; }

        auto filter = ig::LinearExtrusionFilter::New();
        filter->SetExtrusionTypeToNormalExtrusion();
        filter->SetVector(0.0, 0.0, 1.0);
        filter->SetScaleFactor(1.0);
        filter->SetInput(input);
        Check(filter->Execute(), "Normal mode without point normals still succeeds (falls back to Vector)");
        Check(!filter->GetMessage().empty(), "the fallback is reported through GetMessage()");
        auto output = ig::DynamicCast<ig::UnstructuredMesh>(filter->GetOutput());
        if (output != nullptr) {
            bool ok = true;
            for (IGsize i = 0; i < info.numberOfPoints; ++i) {
                ig::Vector3d original;
                info.points->GetPoint(i, original);
                ig::Vector3d moved;
                output->GetPoints()->GetPoint(i + info.numberOfPoints, moved);
                if (std::fabs(moved[2] - (original[2] + 1.0)) > 1e-5) { ok = false; }
            }
            Check(ok, "fallback used the Vector displacement");
        }
    }

    // Normal 模式 + 显式点法向：位移跟随法向，且旧法向不复制到输出
    {
        auto input = LoadModel(modelPath);
        if (input == nullptr) { return; }
        ModelInfo info;
        if (!ReadModelInfo(input, info)) { return; }

        auto normals = ig::FloatArray::New();
        normals->SetName("Normals");
        normals->SetDimension(3);
        normals->Resize(info.numberOfPoints);
        for (IGsize i = 0; i < info.numberOfPoints; ++i) {
            normals->SetValue(i * 3 + 0, 0.0);
            normals->SetValue(i * 3 + 1, 0.0);
            normals->SetValue(i * 3 + 2, 1.0);
        }
        input->GetAttributeSet()->AddAttribute(IG_NORMAL, IG_POINT, normals);

        auto filter = ig::LinearExtrusionFilter::New();
        filter->SetExtrusionTypeToNormalExtrusion();
        filter->SetScaleFactor(3.0);
        filter->SetInput(input);
        Check(filter->Execute(), "Normal mode with a valid point normal array succeeds");
        Check(filter->GetMessage().empty(), "no fallback message when point normals exist");
        auto output = ig::DynamicCast<ig::UnstructuredMesh>(filter->GetOutput());
        if (output != nullptr) {
            ig::Vector3d original;
            info.points->GetPoint(0, original);
            ig::Vector3d moved;
            output->GetPoints()->GetPoint(info.numberOfPoints, moved);
            CheckNear(moved[2], original[2] + 3.0, 1e-5, "displacement == ScaleFactor * point normal");
            Check(FindArray(output, IG_POINT, "Normals") == nullptr,
                  "stale point normals are not copied to the output (VTK CopyNormalsOff)");
        }
    }
}

// ---------------------------------------------------------------------------
// 用例 5：UnstructuredMesh 的线 / 折线输入（对应 VTK 的 verts->lines、lines->strips）
// 期望：线 -> 1 个四边形；3 点折线 -> 2 个四边形；Capping 对它们没有影响。
// ---------------------------------------------------------------------------
void TestLineAndPolyLineExtrusion() {
    std::cout << "--- case 5: line / polyline input (UnstructuredMesh) ---" << std::endl;

    // 线（2 点）+ 折线（3 点）混合
    auto mesh = ig::UnstructuredMesh::New();
    mesh->SetPoints(ig::Points::New());
    mesh->GetPoints()->AddPoint(0.0, 0.0, 0.0);  // 0,1：线
    mesh->GetPoints()->AddPoint(1.0, 0.0, 0.0);
    mesh->GetPoints()->AddPoint(3.0, 0.0, 0.0);  // 2,3,4：折线
    mesh->GetPoints()->AddPoint(4.0, 0.0, 0.0);
    mesh->GetPoints()->AddPoint(4.0, 1.0, 0.0);

    auto cells = ig::CellArray::New();
    auto types = ig::UnsignedIntArray::New();
    igIndex line[2] = {0, 1};
    cells->AddCellIds(line, 2);
    types->AddValue(iGame::IG_LINE);
    igIndex polyLine[3] = {2, 3, 4};
    cells->AddCellIds(polyLine, 3);
    types->AddValue(iGame::IG_POLY_LINE);
    mesh->SetCells(cells, types);

    for (int capping = 0; capping <= 1; ++capping) {
        auto filter = ig::LinearExtrusionFilter::New();
        filter->SetExtrusionTypeToVectorExtrusion();
        filter->SetVector(0.0, 0.0, 1.0);
        filter->SetCapping(capping != 0);
        filter->SetInput(mesh);
        Check(filter->Execute(), std::string("line/polyline: Execute() succeeds (Capping ") +
                                         (capping != 0 ? "on)" : "off)"));

        auto output = ig::DynamicCast<ig::UnstructuredMesh>(filter->GetOutput());
        if (output == nullptr) {
            Check(false, "line/polyline: output is an UnstructuredMesh");
            continue;
        }
        Check(output->GetNumberOfPoints() == 10, "line/polyline: point count == 2 * 5");
        Check(output->GetNumberOfCells() == 3, "line/polyline: 1 quad (line) + 2 quads (polyline segments)");
        Check(CountCellsOfType(output, iGame::IG_QUAD) == 3, "line/polyline: every output cell is a quad");
        Check(CountCellsOfType(output, iGame::IG_LINE) == 0, "line/polyline: no line cell is produced");
    }
}

// ---------------------------------------------------------------------------
// 用例 6：非法输入与保持行为
//   无输入 / 体单元网格 / 无单元网格 必须失败并给出 GetMessage()，且不保留上一次输出。
// ---------------------------------------------------------------------------
void TestInvalidInputs(const std::string& modelPath) {
    std::cout << "--- case 6: invalid inputs ---" << std::endl;

    // 无输入
    {
        auto filter = ig::LinearExtrusionFilter::New();
        Check(!filter->Execute(), "Execute() fails when there is no input");
        Check(!filter->GetMessage().empty(), "GetMessage() explains the missing input");
        Check(filter->GetOutput() == nullptr, "a failed execution leaves no output behind");
    }

    // 体单元（四面体）UnstructuredMesh：明确拒绝而不是静默跳过
    {
        auto mesh = ig::UnstructuredMesh::New();
        mesh->SetPoints(ig::Points::New());
        mesh->GetPoints()->AddPoint(0.0, 0.0, 0.0);
        mesh->GetPoints()->AddPoint(1.0, 0.0, 0.0);
        mesh->GetPoints()->AddPoint(0.0, 1.0, 0.0);
        mesh->GetPoints()->AddPoint(0.0, 0.0, 1.0);
        auto cells = ig::CellArray::New();
        auto types = ig::UnsignedIntArray::New();
        igIndex tetra[4] = {0, 1, 2, 3};
        cells->AddCellIds(tetra, 4);
        types->AddValue(iGame::IG_TETRA);
        mesh->SetCells(cells, types);

        auto filter = ig::LinearExtrusionFilter::New();
        filter->SetInput(mesh);
        Check(!filter->Execute(), "Execute() fails for a tetrahedron-only UnstructuredMesh");
        Check(!filter->GetMessage().empty(), "GetMessage() names the unsupported cell");
        std::cout << "      message: " << filter->GetMessage() << std::endl;
    }

    // 有单元类型数组但没有任何单元
    {
        auto mesh = ig::UnstructuredMesh::New();
        mesh->SetPoints(ig::Points::New());
        mesh->GetPoints()->AddPoint(0.0, 0.0, 0.0);
        auto filter = ig::LinearExtrusionFilter::New();
        filter->SetInput(mesh);
        Check(!filter->Execute(), "Execute() fails when the UnstructuredMesh has no cells");
    }

    // 没有任何点的表面网格
    {
        auto mesh = ig::SurfaceMesh::New();
        auto filter = ig::LinearExtrusionFilter::New();
        filter->SetInput(mesh);
        Check(!filter->Execute(), "Execute() fails when the input has no points");
    }

    // 失败之后再给合法输入：仍然能正常执行（上一次失败不残留状态）
    {
        auto input = LoadModel(modelPath);
        if (input == nullptr) { return; }
        auto filter = ig::LinearExtrusionFilter::New();
        filter->SetExtrusionTypeToVectorExtrusion();
        filter->SetInput(input);
        Check(filter->Execute(), "a valid run right after a failed run still succeeds");
        Check(filter->GetOutput() != nullptr, "the successful run produces an output");
    }
}

// ---------------------------------------------------------------------------
// 用例 7：SurfaceMesh 输入分支与默认参数
//   默认参数 = VTK 构造函数默认值：Normal 模式、Capping 开、ScaleFactor 1、Vector (0,0,1)。
// ---------------------------------------------------------------------------
void TestSurfaceMeshDefaults() {
    std::cout << "--- case 7: SurfaceMesh input with default parameters ---" << std::endl;

    // 与 FileIO 一致：直接注入 CellArray，不走 AddFace（AddFace 依赖未初始化的成员）
    auto mesh = ig::SurfaceMesh::New();
    auto points = ig::Points::New();
    points->AddPoint(0.f, 0.f, 0.f);
    points->AddPoint(1.f, 0.f, 0.f);
    points->AddPoint(1.f, 1.f, 0.f);
    points->AddPoint(0.f, 1.f, 0.f);
    mesh->SetPoints(points);
    auto faces = ig::CellArray::New();
    igIndex quad[4] = {0, 1, 2, 3};
    faces->AddCellIds(quad, 4);
    mesh->SetFaces(faces);

    auto filter = ig::LinearExtrusionFilter::New();
    Check(filter->GetExtrusionType() == ig::LinearExtrusionFilter::ExtrusionType::Normal,
          "default extrusion type is Normal (same as the VTK constructor)");
    Check(filter->GetCapping(), "default Capping is on");
    CheckNear(filter->GetScaleFactor(), 1.0, 1e-12, "default ScaleFactor is 1.0");
    filter->SetInput(mesh);
    Check(filter->Execute(), "SurfaceMesh input: Execute() succeeds with defaults");

    auto output = ig::DynamicCast<ig::UnstructuredMesh>(filter->GetOutput());
    if (output == nullptr) {
        Check(false, "SurfaceMesh input: output is an UnstructuredMesh");
        return;
    }
    Check(output->GetNumberOfPoints() == 8, "SurfaceMesh input: point count == 2N");
    // 1 个四边形：F=1、B=4 => 2*1 + 4 = 6
    Check(output->GetNumberOfCells() == 6, "SurfaceMesh input: cell count == 2F + B == 6");
    // 没有点法向 -> 回退 Vector(0,0,1) * 1
    ig::Vector3d moved;
    output->GetPoints()->GetPoint(4, moved);
    CheckNear(moved[2], 1.0, 1e-5, "SurfaceMesh input: fallback Vector displacement applied (z == 1)");
}

}  // namespace

int main(int argc, char* argv[]) {
    std::cout << "==== LinearExtrusionFilter test ====" << std::endl;

    // 默认使用仓库自带的测试模型；第一个参数可覆盖模型目录（末尾不要带分隔符）
    const std::string modelDir = (argc > 1) ? std::string(argv[1]) : std::string("./Models");
    const std::string openSurface = modelDir + "/LinearExtrusion_OpenSurface.vtk";
    const std::string closedCube = modelDir + "/LinearExtrusion_ClosedCube.vtk";

    TestOpenSurfaceVectorExtrusion(openSurface);
    TestOpenSurfaceWithoutCapping(openSurface);
    TestClosedSurface(closedCube);
    TestExtrusionModes(openSurface);
    TestLineAndPolyLineExtrusion();
    TestInvalidInputs(openSurface);
    TestSurfaceMeshDefaults();

    std::cout << "===================================" << std::endl;
    if (g_FailureCount == 0) {
        std::cout << "PASS: LinearExtrusionFilter test finished, all checks passed" << std::endl;
        return 0;
    }
    std::cout << "FAIL: " << g_FailureCount << " check(s) failed" << std::endl;
    return 1;
}
