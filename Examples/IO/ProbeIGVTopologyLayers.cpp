// IGV topology probe.
//
// Answers three questions needed by the writer:
//   Q1 Does VolumeMesh::GetCells() return faces or volumes?
//   Q2 Do SurfaceMesh/VolumeMesh/UnstructuredMesh always provide ncells+1 offsets?
//   Q3 Can non-unstructured meshes lack Types?
//
// If these are wrong, the writer emits face/volume indices that disagree with iGame's
// internal representation and read-back topology is silently wrong.
//
// Invariants checked: offsets length, zero start, strict increase, final offset equals
// buffer size, connectivity in range, Types in the whitelist, and point count derivable
// from offsets.

#include <MeshCodec/EncodeAdapter/iGameMeshEncodeAdapterFromDataObject.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGameIGVCellType.h>
#include <iGamePointSet.h>
#include <iGameStructuredMesh.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

using namespace iGame;

int g_failed = 0;
int g_checks = 0;

void Check(bool ok, const std::string& what) {
    ++g_checks;
    std::cerr << (ok ? "  [ ok ] " : "  [FAIL] ") << what << "\n";
    if (!ok) { ++g_failed; }
}

void Info(const std::string& what) {
    std::cerr << "  [info] " << what << "\n";
}

std::string MeshTypeName(IGenum type) {
    switch (type) {
        case IG_POINT_SET: return "PointSet";
        case IG_SURFACE_MESH: return "SurfaceMesh";
        case IG_VOLUME_MESH: return "VolumeMesh";
        case IG_UNSTRUCTURED_MESH: return "UnstructuredMesh";
        case IG_STRUCTURED_MESH: return "StructuredMesh";
        default: return "Unknown(" + std::to_string(static_cast<long long>(type)) + ")";
    }
}

/// 用 IGV 白名单判断单元码是否可写出；同时给出"禁止"或"未知"的原因。
std::string ClassifyCellType(int value) {
    if (igv::IsWritableCellType(value)) {
        return "writable";
    }
    if (const auto* forbidden = igv::FindForbiddenCellType(value)) {
        return "FORBIDDEN(" + std::string(forbidden->name) + ")";
    }
    return "UNKNOWN";
}

/// 统计 "单元尺寸 -> 单元数" 的分布，用于判断是否真的定长。
std::map<int, IGsize> CellSizeHistogram(const UnsignedIntArray::Pointer& offsets) {
    std::map<int, IGsize> hist;
    if (!offsets || offsets->GetNumberOfValues() < 2) { return hist; }
    const IGsize n = offsets->GetNumberOfValues() - 1;
    for (IGsize i = 0; i < n; ++i) {
        const int size = static_cast<int>(offsets->GetValue(i + 1) - offsets->GetValue(i));
        ++hist[size];
    }
    return hist;
}

/// 轻量校验：offset 表的末值是否等于 index 表长度。
bool OffsetTailMismatchesIndexSize(const std::vector<unsigned int>& offsets,
                                   const std::vector<unsigned int>& index) {
    if (offsets.empty()) { return !index.empty(); }
    return offsets.back() != static_cast<unsigned int>(index.size());
}

struct ProbeStats {
    bool sawOffsetsLengthOk = false;
    bool sawOffsetsStartZero = false;
    bool sawOffsetsIncreasing = false;
    bool sawOffsetsEndMatchesBuffer = false;
    bool sawIndicesInRange = false;
    bool sawAllTypesWritable = false;
    bool sawPolyhedronSecondaryIndex = false;
    bool sawFixedCellSizeWithRealOffsets = false;
};

void ProbeOne(const std::string& path, ProbeStats& stats) {
    std::cerr << "\n--------------------------------------------------------------------------------\n";
    std::cerr << "模型: " << path << "\n";
    std::cerr << "--------------------------------------------------------------------------------\n";

    // const 目的：MeshEncodeAdapterFromDataObject 的构造签名是
    //   explicit MeshEncodeAdapterFromDataObject(const DataObject::Pointer& dataObj)
    // 直接传非 const 左值无法绑定到 const 引用（SmartPointer 的转换会产生临时量）。
    const DataObject::Pointer obj = FileIO::ReadFile(path);
    if (!obj) {
        Check(false, "读取失败: " + path);
        return;
    }

    const IGenum meshType = obj->GetDataObjectType();
    Info("GetDataObjectType() = " + MeshTypeName(meshType) +
         " (" + std::to_string(static_cast<long long>(meshType)) + ")");

    if (meshType != IG_POINT_SET && meshType != IG_SURFACE_MESH &&
        meshType != IG_VOLUME_MESH && meshType != IG_UNSTRUCTURED_MESH &&
        meshType != IG_STRUCTURED_MESH) {
        Info("不在 IGV 支持的五类之内，跳过");
        return;
    }

    // ---- 各层的原始计数（回答 Q1：哪一层是什么）----
    if (auto* ps = dynamic_cast<PointSet*>(obj.GetPointer())) {
        Info("PointSet::GetNumberOfPoints() = " + std::to_string(ps->GetNumberOfPoints()));
    }
    if (auto* sm = dynamic_cast<SurfaceMesh*>(obj.GetPointer())) {
        Info("SurfaceMesh::GetNumberOfFaces()  = " + std::to_string(sm->GetNumberOfFaces()));
        if (sm->GetFaces()) {
            Info("  GetFaces()->GetNumberOfCells()  = " +
                 std::to_string(sm->GetFaces()->GetNumberOfCells()));
            Info("  GetCells()  == GetFaces() ?     = " +
                 std::string(sm->GetCells() == sm->GetFaces() ? "yes" : "no"));
        }
    }
    if (auto* vm = dynamic_cast<VolumeMesh*>(obj.GetPointer())) {
        Info("VolumeMesh::GetNumberOfVolumes() = " + std::to_string(vm->GetNumberOfVolumes()));
        Info("VolumeMesh::GetNumberOfFaces()   = " + std::to_string(vm->GetNumberOfFaces()));
        Info("  GetVolumes()->GetNumberOfCells()= " +
             std::to_string(vm->GetVolumes() ? vm->GetVolumes()->GetNumberOfCells() : -1));
        Info("  GetCells() == GetVolumes() ?    = " +
             std::string(vm->GetCells() == vm->GetVolumes() ? "yes" : "no"));
        Info("  GetIsPolyhedronType()           = " +
             std::string(vm->GetIsPolyhedronType() ? "true" : "false"));
        if (vm->GetVolumeFaces()) {
            Info("  GetVolumeFaces()->GetNumberOfCells() = " +
                 std::to_string(vm->GetVolumeFaces()->GetNumberOfCells()));
        } else {
            Info("  GetVolumeFaces() = null");
        }
    }
    if (auto* um = dynamic_cast<UnstructuredMesh*>(obj.GetPointer())) {
        Info("UnstructuredMesh::GetNumberOfCells() = " +
             std::to_string(um->GetNumberOfCells()));
    }
    if (auto* st = dynamic_cast<StructuredMesh*>(obj.GetPointer())) {
        const igIndex* dims = st->GetDimensionSize();
        Info("StructuredMesh::GetDimensionSize() = [" +
             std::to_string(static_cast<long long>(dims[0])) + ", " +
             std::to_string(static_cast<long long>(dims[1])) + ", " +
             std::to_string(static_cast<long long>(dims[2])) + "]");
        Info("  nx*ny*nz = " + std::to_string(
                 static_cast<long long>(dims[0]) * dims[1] * dims[2]));
    }

    // ---- 通过 MeshEncodeAdapterFromDataObject 取统一视图（IGV 写侧将用这一层）----
    auto adapter = std::make_shared<MeshEncodeAdapterFromDataObject>(obj);

    const IGsize nPoints = adapter->GetNumberOfPoints();
    const IGsize nCells = adapter->GetNumberOfCells();
    const IGsize nFaces = adapter->GetNumberOfFaces();
    const IGsize offsetSize = adapter->GetCellIdOffsetSize();
    const IGsize bufferSize = adapter->GetCellIdBufferSize();
    const bool isFixed = adapter->IsFixedCellSize();
    const bool isSecondary = adapter->IsSecondaryIndexPolyhedronMesh();
    const int fixedCellSize = adapter->GetFixedCellSize();

    Info("--- MeshEncodeAdapterFromDataObject ---");
    Info("GetNumberOfPoints()      = " + std::to_string(nPoints));
    Info("GetNumberOfCells()       = " + std::to_string(nCells));
    Info("GetNumberOfFaces()       = " + std::to_string(nFaces));
    Info("GetCellIdOffsetSize()    = " + std::to_string(offsetSize));
    Info("GetCellIdBufferSize()    = " + std::to_string(bufferSize));
    Info("IsFixedCellSize()        = " + std::string(isFixed ? "true" : "false"));
    Info("GetFixedCellSize()       = " + std::to_string(fixedCellSize));
    Info("IsSecondaryIndexPolyhedronMesh() = " + std::string(isSecondary ? "true" : "false"));

    UnsignedIntArray::Pointer offsets = adapter->GetCellIdOffset();
    IdArray::Pointer connectivity = adapter->GetCellIdBuffer();
    Points::Pointer points = adapter->GetPoints();

    if (points) {
        Info("Points: n=" + std::to_string(points->GetNumberOfPoints()) +
             " components=" + std::to_string(points->GetDimension()) +
             " bytesPerValue=" + std::to_string(points->GetArrayTypedSize()));
        if (points->GetDimension() != 3) {
            Check(false, "Points 分量数应为 3，实际 " + std::to_string(points->GetDimension()));
        }
        if (points->GetArrayTypedSize() != static_cast<IGsize>(sizeof(float))) {
            Check(false, "Points 值宽应为 4 (float32)，实际 " +
                         std::to_string(points->GetArrayTypedSize()));
        }
    } else {
        Check(false, "GetPoints() 返回空");
    }

    // 交叉校验：适配器报的点数必须等于 Points 数组自身的点数（否则 P1 写出的
    // NumberOfPoints 会与 Points dataspace 不一致，读侧交叉校验会拒绝）。
    if (points) {
        Check(nPoints == points->GetNumberOfPoints(),
              "适配器 GetNumberOfPoints()(" + std::to_string(nPoints) +
              ") 应等于 Points 数组元素数(" +
              std::to_string(points->GetNumberOfPoints()) + ")");
    }

    // =========================================================================
    // Q2：Offsets 长度是否为 ncells + 1
    // =========================================================================
    if (meshType == IG_POINT_SET) {
        Info("PointSet：无拓扑，跳过 Offsets/Connectivity/Types 检查");
        return;
    }

    if (!offsets) {
        Check(false, "GetCellIdOffset() 返回空（非 PointSet 网格却无 Offsets）");
        return;
    }
    Check(offsetSize == nCells + 1,
          "Q2: GetCellIdOffsetSize()==" + std::to_string(offsetSize) +
          " 应为 GetNumberOfCells()+1==" + std::to_string(nCells + 1));
    Check(offsets->GetNumberOfValues() == offsetSize,
          "Offsets 元素数(" + std::to_string(offsets->GetNumberOfValues()) +
          ") 应等于 GetCellIdOffsetSize()(" + std::to_string(offsetSize) + ")");
    stats.sawOffsetsLengthOk = (offsetSize == nCells + 1);

    // I2: Offsets[0] == 0
    if (offsets->GetNumberOfValues() > 0) {
        const bool startsZero = (offsets->GetValue(0) == 0);
        Check(startsZero, "I2: Offsets[0] 应为 0，实际 " +
                          std::to_string(offsets->GetValue(0)));
        stats.sawOffsetsStartZero = startsZero;
    }

    // I3: 严格递增
    bool increasing = true;
    for (IGsize i = 0; i + 1 < offsets->GetNumberOfValues(); ++i) {
        if (offsets->GetValue(i + 1) <= offsets->GetValue(i)) { increasing = false; break; }
    }
    Check(increasing, "I3: Offsets 应严格递增");
    stats.sawOffsetsIncreasing = increasing;

    // I4: 末值 == bufferSize
    if (offsets->GetNumberOfValues() > 0) {
        const IGsize lastOffset = offsets->GetValue(offsets->GetNumberOfValues() - 1);
        const bool endMatches = (lastOffset == bufferSize);
        if (isFixed) {
            // 固定单元尺寸时 bufferSize 可能含 padding，此不等式是"发现"而非"失败"
            Info("I4(注意): 固定单元尺寸网格。Offsets 末值=" + std::to_string(lastOffset) +
                 " vs GetCellIdBufferSize()=" + std::to_string(bufferSize) +
                 (endMatches ? "  （相等）" :
                  "  （不等 → bufferSize 含 padding，IGV 写 Offsets 必须用 GetCellIdOffset 而非 bufferSize）"));
            stats.sawFixedCellSizeWithRealOffsets = endMatches;
        } else {
            Check(endMatches,
                  "I4: Offsets 末值(" + std::to_string(lastOffset) +
                  ") 应等于 GetCellIdBufferSize()(" + std::to_string(bufferSize) + ")");
        }
        stats.sawOffsetsEndMatchesBuffer = endMatches;
    }

    // I7: 单元尺寸分布（判断是否真定长；也回答"能否靠差分求 cell size"）
    const auto hist = CellSizeHistogram(offsets);
    {
        std::ostringstream oss;
        oss << "I7 单元尺寸分布(尺寸:单元数) = {";
        bool first = true;
        for (const auto& [size, count] : hist) {
            if (!first) { oss << ", "; }
            oss << size << ":" << count;
            first = false;
        }
        oss << "}  共 " << hist.size() << " 种";
        Info(oss.str());
        if (hist.size() == 1) {
            Info("I7 结论: 该网格**真正定长**，单元尺寸 = " +
                 std::to_string(hist.begin()->first));
        } else if (hist.empty()) {
            Info("I7 结论: 无单元（空网格）");
        } else {
            Info("I7 结论: 变长网格，Offsets 差分是唯一正确的单元尺寸来源");
        }
        if (isFixed && hist.size() > 1) {
            Check(false, "IsFixedCellSize()==true 但差分出的单元尺寸有 " +
                         std::to_string(hist.size()) + " 种 —— 不一致");
        }
        if (!isFixed && hist.size() == 1 && hist.begin()->first > 0) {
            Info("注意: IsFixedCellSize()==false 但实际尺寸唯一。"
                 "GetFixedCellSize() = " + std::to_string(fixedCellSize));
        }
    }

    // =========================================================================
    // I5: Connectivity 索引范围
    // =========================================================================
    if (connectivity) {
        Info("Connectivity: n=" + std::to_string(connectivity->GetNumberOfValues()));
        Check(connectivity->GetNumberOfValues() == bufferSize,
              "Connectivity 元素数(" + std::to_string(connectivity->GetNumberOfValues()) +
              ") 应等于 GetCellIdBufferSize()(" + std::to_string(bufferSize) + ")");

        IGsize maxId = -1;
        IGsize minId = 0;
        bool inRange = true;
        IGsize firstBad = -1;
        for (IGsize i = 0; i < connectivity->GetNumberOfValues(); ++i) {
            const IGsize v = connectivity->GetValue(i);
            if (i == 0) { minId = v; }
            if (v > maxId) { maxId = v; }
            if (v < minId) { minId = v; }
            if (v >= nPoints && inRange) { inRange = false; firstBad = i; }
        }
        Info("Connectivity 索引范围 = [" + std::to_string(minId) + ", " +
             std::to_string(maxId) + "]，点数 = " + std::to_string(nPoints));
        const std::string badNote = inRange
                                        ? std::string()
                                        : ("（首个越界位置 " + std::to_string(firstBad) + "）");
        Check(inRange, "I5: Connectivity 索引应全部 < 点数" + badNote);
        stats.sawIndicesInRange = inRange;
    } else {
        Check(false, "GetCellIdBuffer() 返回空（非 PointSet 网格却无 Connectivity）");
    }

    // =========================================================================
    // Q1 + Q3：Types —— 只在 UnstructuredMesh 上取（适配器对其它类型会 assert）
    // =========================================================================
    if (meshType == IG_UNSTRUCTURED_MESH) {
        UnsignedIntArray::Pointer types = adapter->GetCellTypes();
        if (types) {
            Info("Types: n=" + std::to_string(types->GetNumberOfValues()) +
                 " (ncells=" + std::to_string(nCells) + ")");
            Check(types->GetNumberOfValues() == nCells,
                  "Q3: Types 元素数(" + std::to_string(types->GetNumberOfValues()) +
                  ") 应等于 GetNumberOfCells()(" + std::to_string(nCells) + ")");

            std::map<int, IGsize> typeHist;
            bool allWritable = true;
            for (IGsize i = 0; i < types->GetNumberOfValues(); ++i) {
                const int t = static_cast<int>(types->GetValue(i));
                ++typeHist[t];
                if (!igv::IsWritableCellType(t)) { allWritable = false; }
            }
            std::ostringstream oss;
            oss << "Q3 Types 取值分布 = {";
            bool first = true;
            for (const auto& [t, c] : typeHist) {
                if (!first) { oss << ", "; }
                oss << t << "(" << ClassifyCellType(t) << "):" << c;
                first = false;
            }
            oss << "}";
            Info(oss.str());

            if (allWritable) {
                Info("Q3 结论: 全部 Types 值都在 IGV 白名单内，可安全写出");
            } else {
                // 这是"发现"：iGame 数据里出现了 IGV 禁止的码，P1 必须处理
                Info("Q3 结论: **存在白名单外的 Types 值** —— P1 写侧会拒绝该文件。"
                     "需判断是数据本身用了内部码，还是白名单需要扩充。");
            }
            stats.sawAllTypesWritable = allWritable;

            // 多面体：一级索引缺失
            const bool hasPolyhedron =
                typeHist.count(static_cast<int>(IG_POLYHEDRON)) > 0;
            if (hasPolyhedron) {
                Info("Q1 注意: Types 含 IG_POLYHEDRON(8) —— 走二级索引路径，"
                     "GetCellIdBuffer() 给出的一级索引需经 SplitSecondaryIndex 校验");
            }
        } else {
            Info("Q3: GetCellTypes() 返回空 —— 该 UnstructuredMesh 无单元类型数组");
        }
    } else {
        Info("Q1/Q3: 该类型无 Types 数据集（适配器只对 UnstructuredMesh 暴露 GetCellTypes）。"
             "P1 写侧处理策略：");
        Info("        - SurfaceMesh/VolumeMesh：Types 由单元点数 + GetCellTypes 或"
             " 逐单元类型推导，需在 P1 明确");
        Info("        - StructuredMesh：无拓扑 dataset，只写 Dimensions");
    }

    // =========================================================================
    // 多面体二级索引（Q1 的关键分支）
    // =========================================================================
    if (isSecondary) {
        std::vector<unsigned int> vol2faceIdx, vol2faceOff, face2ptIdx, face2ptOff;
        adapter->SplitSecondaryIndex(vol2faceIdx, vol2faceOff, face2ptIdx, face2ptOff);
        Info("Q1 二级索引: volume2faces index=" + std::to_string(vol2faceIdx.size()) +
             " offset=" + std::to_string(vol2faceOff.size()) +
             " | face2points index=" + std::to_string(face2ptIdx.size()) +
             " offset=" + std::to_string(face2ptOff.size()));
        Check(!OffsetTailMismatchesIndexSize(vol2faceOff, vol2faceIdx),
              "Q1: volume2faces offset 末值应等于 index 长度");
        stats.sawPolyhedronSecondaryIndex = true;

        Info("Q1 结论: 该网格是**多面体（二级索引）**。IGV 只存一级索引（），"
             "P1 必须决定用哪条路径取 cell→point 一级索引。");
    }
}

} // namespace

int main(int argc, char** argv) {
    // 默认覆盖五类网格 + 多面体 + 二阶 + 结构化。路径相对 Examples 构建目录。
    const std::vector<std::string> kDefaultModels = {
        // PointSet（顶点云 / 散点云）
        "Models/AIGen_Points_VertexCloud.vtk",
        "Models/AIGen_Points_ScatterCloud.vtk",
        // SurfaceMesh（混合面）
        "Models/CellCenter_surface_mixed.vtk",
        "Models/ExtractCellsByType_surface.vtk",
        // VolumeMesh（六面体）
        "Models/CellCenter_hexa_grid.vtk",
        "Models/AIGen_Hex_PipeSegment.vtk",
        // UnstructuredMesh（四面体 / 混合 / 二阶 / 多面体）
        "Models/CountCellVertices_mixed_cells.vtk",
        "Models/AIGen_Tet_TwistedRod.vtk",
        "Models/iGameValidateCellsFilter_mixed.vtk",
        "Models/LinearExtrusion_ClosedCube.vtk",
        // StructuredMesh
        "Models/Structured_Volume_Test.vtk",
        // 含折线（IG_POLY_LINE，数值 35 与 VTK 不一致）的用例
        "Models/tube_test.vtk",
        "Models/TriangleStripTestModel.vtk",
    };

    std::vector<std::string> models;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) { models.emplace_back(argv[i]); }
    } else {
        models = kDefaultModels;
    }

    std::cerr << "================================================================\n";
    std::cerr << "IGV P1 前置问题探针：拓扑层级 / Offsets 契约 / Types\n";
    std::cerr << "回答   的 Q1 / Q2 / Q3\n";
    std::cerr << "================================================================\n";
    std::cerr << "模型数: " << models.size() << "\n";

    ProbeStats stats;
    int probed = 0;
    for (const auto& m : models) {
        if (!std::filesystem::exists(m)) {
            std::cerr << "\n[跳过] 模型不存在: " << m << "\n";
            continue;
        }
        ++probed;
        ProbeOne(m, stats);
    }

    std::cerr << "\n================================================================\n";
    std::cerr << "Summary\n";
    std::cerr << "================================================================\n";
    std::cerr << "已探测模型数                    : " << probed << "\n";
    std::cerr << "Q2 Offsets 长度 == ncells+1     : "
              << (stats.sawOffsetsLengthOk ? "观测到成立" : "**未观测到**") << "\n";
    std::cerr << "I2 Offsets[0] == 0              : "
              << (stats.sawOffsetsStartZero ? "成立" : "**不成立**") << "\n";
    std::cerr << "I3 Offsets 严格递增             : "
              << (stats.sawOffsetsIncreasing ? "成立" : "**不成立**") << "\n";
    std::cerr << "I4 Offsets 末值 == bufferSize   : "
              << (stats.sawOffsetsEndMatchesBuffer ? "成立" : "**不等（可能含 padding）**") << "\n";
    std::cerr << "I5 Connectivity 索引在范围内    : "
              << (stats.sawIndicesInRange ? "成立" : "**越界**") << "\n";
    std::cerr << "Q3 Types 全部在 IGV 白名单内    : "
              << (stats.sawAllTypesWritable ? "成立" : "**存在白名单外的码**") << "\n";
    std::cerr << "Q1 观测到多面体二级索引网格     : "
              << (stats.sawPolyhedronSecondaryIndex ? "是" : "否") << "\n";
    std::cerr << "I4' 定长网格 Offsets 末值与 bufferSize 相等 : "
              << (stats.sawFixedCellSizeWithRealOffsets ? "是" : "否") << "\n";

    std::cerr << "\n检查项: " << g_checks << " 项，失败 " << g_failed << " 项\n";
    if (g_failed == 0) {
        std::cerr << "全部通过：IGV 写出侧的不变量 I1–I7 在这些模型上成立。\n";
    } else {
        std::cerr << "存在失败项：**先解决它们再进 P1**（P1 会把这些不变量当前提）。\n";
    }
    std::cerr << "================================================================\n";

    return g_failed == 0 ? 0 : 1;
}
