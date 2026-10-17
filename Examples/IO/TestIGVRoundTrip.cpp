// 查询本测试的引入提交：
//   git log --diff-filter=A --format="%h %s" -- Examples/IO/TestIGVRoundTrip.cpp
//
// 本文件是 IGV（iGameVis 通用格式）P1 阶段的验收程序，覆盖设计文档
// kia/igv分阶段实现方案.md §9 P1 的 V1.1–V1.7 全部验证项。
//
// =============================================================================
// 为什么需要它（要保证的正确行为与重要边界用例的原因）
// =============================================================================
//
// IGV 的 P9 准入第 1/2 条是「写出五类网格 + 全属性」与「完整读回、bitwise 保真」。
// 这两条**只能靠可执行的逐值比对来证明**，读代码无法证明：
//
//   1. 「无损」在本方案中的定义是 **bitwise 相等**，不是近似相等
//      （设计文档 §11.2）。IGC 的历史缺陷正是"double 走 float 编码"这类
//      静默精度损失 —— 用近似判据它可能落在容差内而蒙混过关。
//   2. 写出与读取两侧会「互为正确答案」：若只测 round-trip，写出 bug 会被读取
//      bug 掩盖，测试全绿而文件格式是错的。因此本程序**额外**包含
//      「读 P0 手写 golden 样例」这一段（该样例由 h5py 独立产出，不是
//      IGVWriter 的产物），形成双向锚定。
//   3. 「零解压直读」的物理保证（P9-3 的前置条件）必须**可断言**：
//      对每个数据 dataset 断言含 DEFLATE filter；attribute 在任何情况下可读。
//   4. 写侧的每一个「拒绝」都必须有用例（A9：校验不得退化为 assert）。
//      「写了 if 判断」不等于「校验有效」—— 只有逐个红过，才证明校验真的在拦。
//
// 重要边界用例及其原因：
//   * **PointSet 的空拓扑分支**是独立代码路径。iGame 的 `PointSet` 没有单元存储
//     （无 `SetCells`/`AddCell`），而且 VTK 读到的 `VTK_VERTEX` 点云实际是
//     `UnstructuredMesh`（每个点一个 IG_VERTEX 单元，见 iGameVTKAbstractReader.cpp:1279），
//     所以 PointSet 夹具必须**直接构造**，不能靠读 .vtk 得到。
//   * **f64 / u8 属性不能靠文件夹具提供**：iGame 的 VTK 读入路径对这两类属性
//     没有稳定保证。若不显式构造，V1.2「双精度不降精度」就永远测不到。
//   * **内存路径必须用"磁盘上不存在同名文件"的数据**（V1.5 / R14）：否则
//     「HDF5 忽略了 file image、直接打开了磁盘同名文件」这个 bug 会被静默通过。
//   * **DirectPath** 走的是 `WriteToFilePathDirect()`，而 `WriteToFile()` 路径
//     必须**不**失败（基类 `Execute()` 之后会覆盖落盘，见 IGVWriter 的说明）。
//   * **V1.6** 用真实 VTKHDF 文件（`Examples/Models/*.vtkhdf`）反证读取器不会把
//     "长得像"的文件当 IGV 读。这一条的价值在于**拒绝得干净**（不崩溃、不误解、
//     错误信息指出根组不对）。
//
// 引入原因：IGV P1 阶段的验收需求（设计文档 §9 P1）。属"新功能的验收测试"，
// 不是某个 BUG 的回归测试，因此没有对应的修复提交号。
//
// 运行：cd Examples && ./testIGVRoundTrip [--models <dir>] [--golden <dir>]
//   退出码 0 = V1.1–V1.7 全绿。
// =============================================================================

#include <IGV/iGameIGVReader.h>
#include <IGV/iGameIGVWriter.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGameIGVCellType.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

using namespace iGame;
namespace fs = std::filesystem;

int g_checks = 0;
int g_failed = 0;
std::string g_section;

/// V1.8 的统计：真正走完 IGV 往返的单元码值数，以及因上游读取器缺分支而未覆盖的数。
int g_cellTypeValuesCovered = 0;
int g_cellTypeUpstreamGaps = 0;

void Section(const std::string& name) {
    g_section = name;
 std::cerr << "\n------------------------------------------------------------------------------\n"
              << name << "\n"
              << "------------------------------------------------------------------------------\n";
}

void Check(bool ok, const std::string& what) {
    ++g_checks;
 std::cerr << (ok ? " [ ok ] " : " [FAIL] ") << what << "\n";
    if (!ok) { ++g_failed; }
}

void Info(const std::string& what) {
 std::cerr << " [info] " << what << "\n";
}

/// 用例用的临时目录（构建目录下）。用 fs::temp_directory_path 反而会在
/// 沙箱/CI 上遇到权限问题；固定在可执行文件旁边更稳。
fs::path WorkDir() {
    static const fs::path dir = [] {
        fs::path d = fs::current_path() / "igv_roundtrip_tmp";
        std::error_code ec;
        fs::create_directories(d, ec);
        return d;
    }();
    return dir;
}

/// 可执行文件所在目录的候选列表（用于稳健定位 `Models/` 与 golden 样例）。
///
/// 为什么需要它：MSBuild 把可执行文件放在 `<build>/Examples/Release/`，
/// 而运行时 CWD 可能是别处；golden 样例在源码树的 `kia/testdata/igv`。
/// 靠"CWD 往上两级"这种假设既不可靠也不跨平台。
///
/// 这里给出一串**候选根目录**，再逐个向上找目标 —— 覆盖
/// "exe 与资源同处构建树" 和 "exe 在源码树旁" 两种常见布局。
std::vector<fs::path> ResourceSearchRoots(const char* argv0) {
    std::vector<fs::path> roots;
    std::error_code ec;

    // 1) argv[0] 所在目录（Windows 上 MSBuild 传的是完整路径；Linux/macOS 可能只有名字）
    if (argv0 != nullptr && *argv0 != '\0') {
        const fs::path a0(argv0);
        if (a0.has_parent_path()) {
            roots.push_back(fs::absolute(a0, ec).parent_path());
        }
    }

    // 2) 当前工作目录
    roots.push_back(fs::current_path(ec));

    // 3) 从 CMAKE 注入的源码目录（最可靠，见 Examples/CMakeLists.txt 的注入）
#ifdef IGAME_IGV_SOURCE_DIR
    roots.emplace_back(IGAME_IGV_SOURCE_DIR);
#endif

    // 去重 + 去掉不存在的
    std::vector<fs::path> out;
    for (const fs::path& r : roots) {
        if (r.empty()) { continue; }
        if (!fs::exists(r, ec)) { continue; }
        bool dup = false;
        for (const fs::path& o : out) {
            if (o == r) { dup = true; break; }
        }
        if (!dup) { out.push_back(r); }
    }
    return out;
}

/// 在一串候选根目录里，逐个向上找含指定相对路径的目录。
/// 返回第一个命中的绝对路径；找不到返回空。
fs::path FindResource(const std::vector<fs::path>& roots, const fs::path& relative) {
    std::error_code ec;
    for (const fs::path& root : roots) {
        for (fs::path cur = root; !cur.empty(); cur = cur.parent_path()) {
            const fs::path candidate = cur / relative;
            if (fs::exists(candidate, ec)) { return candidate; }
            if (cur == cur.parent_path()) { break; }
        }
    }
    return {};
}

// =============================================================================
// 逐值比对辅助（§11.2：bitwise，不是近似）
// =============================================================================

bool SameFloats(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) { return false; }
    return a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0;
}

bool SameInts(const std::vector<int32_t>& a, const std::vector<int32_t>& b) {
    if (a.size() != b.size()) { return false; }
    return a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(int32_t)) == 0;
}

bool SameI64(const std::vector<int64_t>& a, const std::vector<int64_t>& b) {
    if (a.size() != b.size()) { return false; }
    return a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(int64_t)) == 0;
}

bool SameU8(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    if (a.size() != b.size()) { return false; }
    return a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(uint8_t)) == 0;
}

/// 提取一个 DataObject 的"可比快照"：点 / 拓扑 / 属性。
///
/// 拓扑统一走 `DataObject::GetCellArray()`（虚函数）：
///   * `UnstructuredMesh` -> `m_Cells`
///   * `SurfaceMesh`      -> `m_Faces`
///   * `VolumeMesh`       -> `m_Volumes`
///   * `PointSet`         -> nullptr（无单元表达能力，这正是"空拓扑分支"）
///   * `StructuredMesh`   -> 刻意返回 nullptr（见 iGameStructuredMesh.h:25），
///                          其拓扑由 Dimensions 隐式表达，另行比较
struct Snapshot {
    IGenum type{IG_NONE};
    int64_t pointCount{0};
    int64_t cellCount{0};
    std::vector<float> points;
    std::vector<int32_t> connectivity;
    std::vector<int64_t> offsets;
    std::vector<uint8_t> types;
    bool hasTopology{false};
    int64_t dims[3]{0, 0, 0};
    bool hasDims{false};

    struct Attr {
        std::string name;
        IGenum type{IG_NONE};
        IGenum attachment{IG_NONE};
        int components{1};
        int valueSize{0};
        int64_t elementCount{0};
        std::vector<float> f32;
        std::vector<double> f64;
        std::vector<unsigned char> u8;
    };
    std::vector<Attr> attrs;
};

Snapshot TakeSnapshot(DataObject::Pointer obj) {
    Snapshot s;
    if (!obj) { return s; }
    s.type = obj->GetDataObjectType();

    // 点数（一律经 PointSet，因为除纯 DataObject 外都派生自它）
    if (auto ps = DynamicCast<PointSet>(obj)) {
        if (Points::Pointer pts = ps->GetPoints()) {
            s.pointCount = static_cast<int64_t>(pts->GetNumberOfPoints());
            const size_t n = static_cast<size_t>(s.pointCount) * 3;
            s.points.assign(pts->RawPointer(), pts->RawPointer() + n);
        }
    }

    // 结构化网格的维度
    if (s.type == IG_STRUCTURED_MESH) {
        if (auto sm = DynamicCast<StructuredMesh>(obj)) {
            const igIndex* d = sm->GetDimensionSize();
            if (d) {
                s.hasDims = true;
                s.dims[0] = d[0];
                s.dims[1] = d[1];
                s.dims[2] = d[2];
            }
        }
    }

    // 拓扑
    if (CellArray::Pointer cells = obj->GetCellArray()) {
        s.hasTopology = true;
        s.cellCount = static_cast<int64_t>(cells->GetNumberOfCells());

        if (IdArray::Pointer ids = cells->GetCellIdArray()) {
            const IGsize n = ids->GetNumberOfIds();
            s.connectivity.reserve(static_cast<size_t>(n));
            for (IGsize i = 0; i < n; ++i) {
                s.connectivity.push_back(static_cast<int32_t>(ids->GetId(i)));
            }
        }
        if (UnsignedIntArray::Pointer off = cells->GetOffset()) {
            const IGsize n = off->GetNumberOfValues();
            // 固定尺寸网格的 offset 数组只有 {0}（见 iGameCellArray 构造）；
            // 这种情况下顶层由 FixedCellSize 表达，快照里按"无 offset 数组"处理，
            // 只在写出的 .igv 里检查展开后的 Offsets。
            if (static_cast<int64_t>(n) == s.cellCount + 1) {
                s.offsets.reserve(static_cast<size_t>(n));
                for (IGsize i = 0; i < n; ++i) {
                    s.offsets.push_back(static_cast<int64_t>(off->GetValue(i)));
                }
            }
        }
    }

    // 单元类型：只有 UnstructuredMesh 有权威来源
    if (auto um = DynamicCast<UnstructuredMesh>(obj)) {
        if (UnsignedIntArray::Pointer ts = um->GetCellTypes()) {
            const IGsize n = ts->GetNumberOfValues();
            s.types.reserve(static_cast<size_t>(n));
            for (IGsize i = 0; i < n; ++i) {
                // GetValue() 返回 double —— 不能对它做位运算（& 0xFF 会报
                // "left operand has type double"），直接 static_cast 截断。
                s.types.push_back(static_cast<uint8_t>(ts->GetValue(i)));
            }
        }
    }

    // 属性
    if (AttributeSet* set = obj->GetAttributeSet()) {
        const IGsize n = set->GetNumberOfAttributes();
        for (IGsize i = 0; i < n; ++i) {
            AttributeSet::Attribute& a = set->GetAttribute(i);
            if (a.IsNone()) { continue; }
            ArrayObject::Pointer arr = a.GetPointer();
            if (!arr) { continue; }

            Snapshot::Attr sa;
            sa.name = arr->GetName();
            sa.type = a.GetType();
            sa.attachment = a.GetAttachmentType();
            sa.components = arr->GetDimension();
            sa.valueSize = static_cast<int>(arr->GetArrayTypedSize());
            sa.elementCount = static_cast<int64_t>(arr->GetNumberOfElements());

            if (auto f = DynamicCast<FloatArray>(arr)) {
                sa.f32.assign(f->RawPointer(), f->RawPointer() + arr->GetNumberOfValues());
            } else if (auto d = DynamicCast<DoubleArray>(arr)) {
                sa.f64.assign(d->RawPointer(), d->RawPointer() + arr->GetNumberOfValues());
            } else if (auto u = DynamicCast<UnsignedCharArray>(arr)) {
                sa.u8.assign(u->RawPointer(), u->RawPointer() + arr->GetNumberOfValues());
            }
            s.attrs.push_back(std::move(sa));
        }
    }
    return s;
}

const Snapshot::Attr* FindAttr(const Snapshot& s, const std::string& name) {
    for (const auto& a : s.attrs) {
        if (a.name == name) { return &a; }
    }
    return nullptr;
}

bool AttrEquals(const Snapshot::Attr& a, const Snapshot::Attr& b) {
    if (a.name != b.name || a.type != b.type || a.attachment != b.attachment ||
        a.components != b.components || a.valueSize != b.valueSize ||
        a.elementCount != b.elementCount) {
        return false;
    }
    if (!a.f32.empty() || !b.f32.empty()) { return SameFloats(a.f32, b.f32); }
    if (!a.f64.empty() || !b.f64.empty()) {
        if (a.f64.size() != b.f64.size()) { return false; }
        return a.f64.empty() || std::memcmp(a.f64.data(), b.f64.data(),
                                            a.f64.size() * sizeof(double)) == 0;
    }
    return SameU8(a.u8, b.u8);
}

/// 逻辑对象逐值比对（V1.1 第一段，步骤 3–8）。
void CompareObjects(const Snapshot& a, const Snapshot& b, const std::string& label) {
 Check(a.type == b.type, label + ": mesh type equal");
 Check(a.pointCount == b.pointCount,
          label + ": point count equal (" + std::to_string(a.pointCount) + " vs " +
              std::to_string(b.pointCount) + ")");
 Check(SameFloats(a.points, b.points),
          label + ": Points bitwise equal (" + std::to_string(a.points.size()) + " floats)");

    // 顺序不变（§3.3「不做空间重排」）
 Check(SameInts(a.connectivity, b.connectivity) && SameI64(a.offsets, b.offsets) &&
              SameU8(a.types, b.types),
          label + ": Connectivity/Offsets/Types equal and order preserved");

    if (a.type == IG_STRUCTURED_MESH) {
  Check(a.hasDims && b.hasDims, label + ": both sides have Dimensions");
        if (a.hasDims && b.hasDims) {
   Check(a.dims[0] == b.dims[0] && a.dims[1] == b.dims[1] && a.dims[2] == b.dims[2],
                  label + ": Dimensions equal [" + std::to_string(b.dims[0]) + ", " +
                      std::to_string(b.dims[1]) + ", " + std::to_string(b.dims[2]) + "]");
        }
    }

    // 属性集合（按名字比对，规避读回时属性顺序可能变化）
 Check(a.attrs.size() == b.attrs.size(),
          label + ": attribute count equal (" + std::to_string(a.attrs.size()) + " vs " +
              std::to_string(b.attrs.size()) + ")");

    for (const auto& oa : a.attrs) {
        const Snapshot::Attr* ob = FindAttr(b, oa.name);
        if (!ob) {
   Check(false, label + ": attribute missing after read-back: '" + oa.name + "'");
            continue;
        }
  Check(AttrEquals(oa, *ob),
              label + ": attribute '" + oa.name + "' bitwise equal (" +
                  std::to_string(oa.elementCount) + "x" + std::to_string(oa.components) +
                  ", valueSize=" + std::to_string(oa.valueSize) + ")");
    }
}

// =============================================================================
// 夹具
// =============================================================================

/// 直接构造一个带 f64 标量属性 + u8 标量属性的 UnstructuredMesh。
///
/// 为什么必须手工构造：iGame 的 VTK 读入路径对 f64 / u8 属性没有稳定保证，
/// 若不显式构造，V1.2「double 不降精度」与「u8 原样保留」就永远测不到。
/// 这也是 IGC 历史上的缺陷点（IGC 把 double 也走 float 编码）。
UnstructuredMesh::Pointer MakeF64U8Fixture() {
    auto mesh = UnstructuredMesh::New();

    // 4 个点
    Points::Pointer pts = Points::New();
    pts->Resize(4);
    const float coords[12] = {0.f, 0.f, 0.f,
                              1.f, 0.f, 0.f,
                              0.f, 1.f, 0.f,
                              0.f, 0.f, 1.f};
    std::memcpy(pts->RawPointer(), coords, sizeof(coords));
    mesh->SetPoints(pts);

    // 1 个四面体（IG_TETRA = 9）
    CellArray::Pointer cells = CellArray::New();
    igIndex ids[4] = {0, 1, 2, 3};
    cells->AddCellIds(ids, 4);
    UnsignedIntArray::Pointer types = UnsignedIntArray::New();
    types->AddValue(static_cast<unsigned int>(IG_TETRA));
    mesh->SetCells(cells, types);

    AttributeSet* set = mesh->GetAttributeSet();

    // f64 标量：取值刻意选在 float32 无法精确表示的位置
    //   0.1 / 0.1234567890123 这类值若被降精度，memcmp 立刻发现
    {
        DoubleArray::Pointer d = DoubleArray::New();
        d->SetDimension(1);
        d->SetName("d_scalar");
        d->Resize(4);
        const double v[4] = {0.1, 0.1234567890123, 1.0 / 3.0, -2.718281828459045};
        std::memcpy(d->RawPointer(), v, sizeof(v));
        set->AddAttribute(IG_SCALAR, IG_POINT, d);
    }
    // u8 标量
    {
        UnsignedCharArray::Pointer u = UnsignedCharArray::New();
        u->SetDimension(1);
        u->SetName("u8_scalar");
        u->Resize(4);
        const unsigned char v[4] = {7, 200, 0, 255};
        std::memcpy(u->RawPointer(), v, sizeof(v));
        set->AddAttribute(IG_SCALAR, IG_POINT, u);
    }
    // f32 的单元属性（3 分量），确认多分量 range 的 2*(1+k) 路径
    {
        FloatArray::Pointer f = FloatArray::New();
        f->SetDimension(3);
        f->SetName("cell_vec");
        f->Resize(1);
        const float v[3] = {1.5f, -2.25f, 0.0f};
        std::memcpy(f->RawPointer(), v, sizeof(v));
        set->AddAttribute(IG_VECTOR, IG_CELL, f);
    }
    return mesh;
}

/// 直接构造一个真正的 PointSet（无单元）。
///
/// 为什么不能从 .vtk 读：VTK 里的 `VTK_VERTEX` 点云会被 `iGameVTKAbstractReader`
/// 建成 **UnstructuredMesh**（每个点一个 IG_VERTEX 单元，见
/// iGameVTKAbstractReader.cpp:1279 与 Examples/IO/VertexReaderValidation.cpp），
/// 而 iGame 的 `PointSet` 根本没有单元存储（无 SetCells/AddCell）。
/// 因此「PointSet 空拓扑分支」只能靠直接构造来覆盖。
PointSet::Pointer MakePointSetFixture() {
    auto ps = PointSet::New();
    Points::Pointer pts = Points::New();
    pts->Resize(5);
    const float coords[15] = {0.f, 0.f, 0.f,
                              1.f, 0.f, 0.f,
                              0.f, 2.f, 0.f,
                              0.f, 0.f, 3.f,
                              -1.f, -1.f, -1.f};
    std::memcpy(pts->RawPointer(), coords, sizeof(coords));
    ps->SetPoints(pts);

    FloatArray::Pointer f = FloatArray::New();
    f->SetDimension(1);
    f->SetName("p_scalar");
    f->Resize(5);
    const float v[5] = {1.f, 2.f, 3.f, 4.f, 5.f};
    std::memcpy(f->RawPointer(), v, sizeof(v));
    ps->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, f);
    return ps;
}

// =============================================================================
// 结构级校验（V1.3 / V1.4）—— 用 h5py 之外的办法：直接借 HDF5 C API
// =============================================================================
//
// 本程序已链接 HDF5（Examples 全量链接 HDF5::HDF5），因此可以直接断言
// dcpl / dataspace，而不必依赖外部 h5dump 可执行文件是否可用
// （本机上 h5dump 因缺运行时 DLL 而无法启动，见 P0 记录的同类问题）。

} // namespace

// =============================================================================
// 结构级校验（V1.3 / V1.4）
// =============================================================================
//
// 直接借 HDF5 C API 做断言，而不依赖外部 `h5dump` 可执行文件是否可用
// —— 本机上 h5dump 因缺运行时 DLL 而无法启动（P0 记录过同一类问题），
// 把验收挂在它上面等于把测试挂在环境上。
//
// 注意：`Examples/*` 的既有目标都链接了 HDF5（见 Examples/CMakeLists.txt 的
// `igame_link_standard_example`），因此这里可以直接用 HDF5 类型。
// 本测试目标另外被包在 `if (ENABLE_IGV_MODULE)` 里。

#include <hdf5.h>

namespace {

struct IGVStructureReport {
    bool hasDeflateForAllDataDatasets{true};
    bool allDataspacesFixed{true};
    bool pointsChunkKeepsRecord{true};
    bool rank1ChunksAreRank1{true};
    bool pointsIsF32{false};
    int attributeCount{0};
};

/// 遍历文件里**所有** dataset：检查 DEFLATE / 固定 dataspace / chunk 秩，
/// 并对 Points 单独检查 dtype 与 chunk 记录完整性；最后数一遍 attribute。
bool InspectIGVFile(const std::string& path, IGVStructureReport& out, std::string& err) {
    const hid_t file = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0) {
        H5Eclear2(H5E_DEFAULT);
        err = "H5Fopen failed";
        return false;
    }

    const hid_t root = H5Gopen2(file, "IGV", H5P_DEFAULT);
    if (root < 0) {
        H5Eclear2(H5E_DEFAULT);
        H5Fclose(file);
        err = "root group /IGV is missing";
        return false;
    }

    struct Ctx {
        IGVStructureReport* rep = nullptr;
        const hid_t root = -1;
        std::string* err = nullptr;
    };
    Ctx ctx{&out, root, &err};

    // 用 H5Ovisit3 遍历全部对象（只读对象头，不读数据）。
    //
    // 注意 flags 必须带上 H5O_INFO_NUM_ATTRS：下面要读 `info->num_attrs`，
    // 而它只在请求了那个 flag 时才被填充。只传 H5O_INFO_BASIC 会得到未初始化
    // 的 num_attrs（属性计数随机）。
    const auto visitor = [](hid_t /*obj*/, const char* name, const H5O_info2_t* info,
                            void* opData) -> herr_t {
        auto* c = static_cast<Ctx*>(opData);
        if (info->type == H5O_TYPE_NAMED_DATATYPE) { return 0; }

        const std::string full = std::string("/IGV/") + name;

        // attribute 计数：H5Ovisit3 给出的 H5O_info2_t::num_attrs 是 hsize_t，
        // 但这里改用 H5Oget_info3(..., H5O_INFO_NUM_ATTRS) 拿一次 int，
        // 免得在同一段代码里混用两种计数口径。
        int nAttrs = 0;
        {
            const hid_t o = H5Oopen(c->root, name, H5P_DEFAULT);
            if (o >= 0) {
                H5O_info2_t oi{};
                if (H5Oget_info3(o, &oi, H5O_INFO_NUM_ATTRS) >= 0) {
                    nAttrs = static_cast<int>(oi.num_attrs);
                }
                H5Oclose(o);
            }
        }
        c->rep->attributeCount += nAttrs;

        if (info->type == H5O_TYPE_GROUP) {
            return 0;
        }

        if (info->type != H5O_TYPE_DATASET) { return 0; }

        const hid_t ds = H5Dopen2(c->root, name, H5P_DEFAULT);
        if (ds < 0) {
            H5Eclear2(H5E_DEFAULT);
            return 0;
        }

        const bool isPoints = (full == "/IGV/Points");

        // dtype（只对 Points 强制 f32）
        {
            const hid_t t = H5Dget_type(ds);
            if (t >= 0) {
                if (isPoints) {
                    const H5T_class_t cls = H5Tget_class(t);
                    const size_t sz = H5Tget_size(t);
                    const H5T_order_t ord = H5Tget_order(t);
                    c->rep->pointsIsF32 =
                        (cls == H5T_FLOAT && sz == 4 && ord == H5T_ORDER_LE);
                }
                H5Tclose(t);
            }
        }

        // dataspace：必须固定（当前维度 == 最大维度，且不含 UNLIMITED）
        {
            const hid_t sp = H5Dget_space(ds);
            if (sp >= 0) {
                const int rank = H5Sget_simple_extent_ndims(sp);
                if (rank > 0) {
                    std::vector<hsize_t> dims(static_cast<size_t>(rank));
                    std::vector<hsize_t> maxDims(static_cast<size_t>(rank));
                    if (H5Sget_simple_extent_dims(sp, dims.data(), maxDims.data()) >= 0) {
                        for (size_t i = 0; i < dims.size(); ++i) {
                            if (maxDims[i] == H5S_UNLIMITED || maxDims[i] != dims[i]) {
                                c->rep->allDataspacesFixed = false;
                            }
                        }
                    }
                }
                H5Sclose(sp);
            }
        }

        // dcpl：DEFLATE filter + chunk 约束
        {
            const hid_t dcpl = H5Dget_create_plist(ds);
            if (dcpl >= 0) {
                bool hasDeflate = false;
                const int nf = H5Pget_nfilters(dcpl);
                for (int i = 0; i < nf; ++i) {
                    unsigned int flags = 0;
                    size_t cdN = 0;
                    const H5Z_filter_t f = H5Pget_filter2(dcpl, static_cast<unsigned>(i),
                                                          &flags, &cdN, nullptr, 0, nullptr,
                                                          nullptr);
                    if (f == H5Z_FILTER_DEFLATE) { hasDeflate = true; }
                }

                const int layout = H5Pget_layout(dcpl);
                const bool chunked = (layout == H5D_CHUNKED);

                // 计数组是小 dataset（8 字节），规范允许它们不压缩、不 chunk。
                const bool isCountDataset =
                    (full.find("NumberOf") != std::string::npos) ||
                    (full == "/IGV/Version");

                if (!isCountDataset) {
                    // 小 dataset（< 4 KiB）按 §5.3 不压缩；大 dataset 必须压缩。
                    const hid_t sp = H5Dget_space(ds);
                    hsize_t npoints = 0;
                    if (sp >= 0) {
                        npoints = H5Sget_simple_extent_npoints(sp);
                        H5Sclose(sp);
                    }
                    const hid_t t = H5Dget_type(ds);
                    size_t elemBytes = 8;
                    if (t >= 0) {
                        elemBytes = H5Tget_size(t);
                        H5Tclose(t);
                    }
                    const size_t totalBytes = static_cast<size_t>(npoints) * elemBytes;
                    if (totalBytes >= 4096 && !hasDeflate) {
                        c->rep->hasDeflateForAllDataDatasets = false;
                    }
                }

                if (chunked) {
                    const hid_t sp = H5Dget_space(ds);
                    int rank = 1;
                    if (sp >= 0) {
                        rank = H5Sget_simple_extent_ndims(sp);
                        H5Sclose(sp);
                    }
                    if (rank < 1) { rank = 1; }
                    std::vector<hsize_t> chunk(static_cast<size_t>(rank), 0);
                    if (H5Pget_chunk(dcpl, rank, chunk.data()) >= 0) {
                        if (rank == 1) { /* a rank-1 dataset has a rank-1 chunk */ }
                        if (isPoints) {
                            // Points 形如 (k,3)：chunk 不得切断记录
                            std::vector<hsize_t> dims;
                            const hid_t sp2 = H5Dget_space(ds);
                            if (sp2 >= 0) {
                                const int r2 = H5Sget_simple_extent_ndims(sp2);
                                if (r2 == 2) {
                                    dims.resize(2);
                                    std::vector<hsize_t> mx(2);
                                    H5Sget_simple_extent_dims(sp2, dims.data(), mx.data());
                                } else if (rank == 1) {
                                    c->rep->pointsChunkKeepsRecord = false;
                                }
                                H5Sclose(sp2);
                            }
                            if (rank == 2 && chunk.size() == 2 && !dims.empty() &&
                                chunk[1] != dims[1]) {
                                c->rep->pointsChunkKeepsRecord = false;
                            }
                        }
                    }
                }
                H5Pclose(dcpl);
            }
        }

        H5Dclose(ds);
        return 0;
    };

    if (H5Ovisit3(root, H5_INDEX_NAME, H5_ITER_NATIVE, visitor, &ctx,
                  H5O_INFO_BASIC | H5O_INFO_NUM_ATTRS) < 0) {
        H5Eclear2(H5E_DEFAULT);
    }
    // 根组自带的 attribute
    {
        H5O_info2_t ri{};
        if (H5Oget_info3(root, &ri, H5O_INFO_NUM_ATTRS) >= 0) {
            out.attributeCount += static_cast<int>(ri.num_attrs);
        }
    }

    H5Gclose(root);
    H5Fclose(file);
    return true;
}

/// 对一个 .igv 文件做 V1.3 / V1.4 的结构断言。
void CheckFileStructure(const fs::path& path, const std::string& label) {
    IGVStructureReport report;
    std::string err;
    if (!InspectIGVFile(path.string(), report, err)) {
  Check(false, label + ": structure check failed - " + err);
        return;
    }

 Check(report.hasDeflateForAllDataDatasets,
          label + ": every data dataset >= 4 KiB is gzip (DEFLATE)");
 Check(report.attributeCount > 0,
          label + ": attributes readable (" + std::to_string(report.attributeCount) + ")");
 Check(report.allDataspacesFixed,
          label + ": all datasets use a fixed dataspace (current dims == max dims, A16)");
 Check(report.pointsChunkKeepsRecord,
          label + ": Points chunks do not split a record (chunk_dims[1] == 3)");
 Check(report.pointsIsF32, label + ": Points dtype is H5T_IEEE_F32LE");
}

// =============================================================================
// 主流程
// =============================================================================

} // namespace

int main(int argc, char** argv) {
    std::string modelsDir = "Models";
    std::string goldenDir;
    bool onlyList = false;

    // 资源定位根目录（argv[0] 所在目录 / CWD / 源码目录），逐个向上找。
    const std::vector<fs::path> roots = ResourceSearchRoots(argc > 0 ? argv[0] : nullptr);

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--models" && i + 1 < argc) { modelsDir = argv[++i]; }
        else if (a == "--golden" && i + 1 < argc) { goldenDir = argv[++i]; }
        else if (a == "--list") { onlyList = true; }
    }
    if (onlyList) {
  std::cerr << "This program has no --list mode; ignoring the flag.\n";
    }

    // Models 目录：用户没显式指定时，用同一套向上搜索定位。
    if (modelsDir == "Models" || modelsDir.empty()) {
        const fs::path found = FindResource(roots, "Models");
        if (!found.empty()) { modelsDir = found.string(); }
    }

    if (goldenDir.empty()) {
        // 默认找仓库里的 P0 golden 样例（kia/testdata/igv）。
        // 用候选根目录向上搜索，而不是假设"CWD 往上两级" ——
        // 否则在 <build>/Examples/Release/ 下会解析成不存在的 <build>/kia/testdata/igv
        // （实测踩到过这一点）。
        const fs::path found = FindResource(roots, fs::path("kia") / "testdata" / "igv");
        if (!found.empty()) {
            goldenDir = found.string();
        } else {
            goldenDir = (fs::current_path() / ".." / "kia" / "testdata" / "igv")
                            .lexically_normal()
                            .string();
   std::cerr << "[warn] golden sample directory not found; guessing: " << goldenDir << "\n";
        }
    }

 std::cerr << "================================================================\n"
              << "IGV P1 acceptance: V1.1-V1.7\n"
              << "Covers kia/igv implementation plan, section 9 (P1)\n"
              << "================================================================\n"
              << "Working dir   : " << fs::current_path().string() << "\n"
              << "Models dir    : " << modelsDir << "\n"
              << "Golden dir    : " << goldenDir << "\n"
              << "Temp dir      : " << WorkDir().string() << "\n";

    // =========================================================================
    // V1.1 — 五类网格 round-trip 无损
    // =========================================================================
    Section("V1.1 lossless round-trip for the five mesh kinds (P9-1 / P9-2)");

    struct Fixture {
        std::string label;
        std::string path;   ///< 空表示用直接构造的夹具
        int fixtureKind;    ///< 0 = 读文件, 1 = PointSet, 2 = f64/u8
    };
    const std::vector<Fixture> fixtures = {
        {"SurfaceMesh",       modelsDir + "/CellCenter_surface_mixed.vtk", 0},
        {"VolumeMesh",        modelsDir + "/CellCenter_hexa_grid.vtk",     0},
        {"UnstructuredMesh",  modelsDir + "/CountCellVertices_mixed_cells.vtk", 0},
        {"StructuredMesh",    modelsDir + "/Structured_Volume_Test.vtk",  0},
        {"PointSet(empty topology)",   "", 1},
        {"Unstructured(f64/u8 attribute)", "", 2},
    };

    for (const Fixture& fx : fixtures) {
        DataObject::Pointer objA;
        if (fx.fixtureKind == 1) {
            objA = MakePointSetFixture();
        } else if (fx.fixtureKind == 2) {
            objA = MakeF64U8Fixture();
        } else {
            if (!fs::exists(fx.path)) {
    Info("skip " + fx.label + ": model not found: " + fx.path);
                continue;
            }
            objA = FileIO::ReadFile(fx.path);
        }
        if (!objA) {
   Check(false, fx.label + ": fixture preparation failed");
            continue;
        }

        const fs::path out = WorkDir() / (fx.label + ".igv");
        (void)out;
        // 文件名里可能有括号等字符，统一净化
        std::string safe = fx.label;
        for (char& c : safe) {
            if (c == '(' || c == ')' || c == '/' || c == ' ') { c = '_'; }
        }
        const fs::path outPath = WorkDir() / (safe + ".igv");

  Info(fx.label + ": DataObjectType=" + std::to_string(static_cast<long long>(
                                 objA->GetDataObjectType())) +
             " -> " + outPath.filename().string());

        // 1. IGVWriter 直写
        IGVWriter::Pointer writer = IGVWriter::New();
        if (!writer->WriteToFile(objA, outPath.string())) {
   Check(false, fx.label + ": writefailed - " + writer->GetLastError());
            continue;
        }
  Check(true, fx.label + ": write OK (" + std::to_string(fs::file_size(outPath)) + " bytes)");

        // 2. IGVReader 读回
        IGVReader::Pointer reader = IGVReader::New();
        DataObject::Pointer objB = reader->ReadFile(outPath.string());
        if (!objB) {
   Check(false, fx.label + ": read-back failed - " + reader->GetLastError());
            continue;
        }

        // 3–8. 逻辑对象逐值比对
        const Snapshot sa = TakeSnapshot(objA);
        const Snapshot sb = TakeSnapshot(objB);
  Info(fx.label + ": attribute count " + std::to_string(sa.attrs.size()) + " -> " +
             std::to_string(sb.attrs.size()));
        CompareObjects(sa, sb, fx.label);

        // PointSet 的空拓扑分支必须真的没有拓扑 dataset（§3.3）
        if (fx.fixtureKind == 1) {
   Check(!sb.hasTopology, fx.label + ": read-back object has no topology (empty-topology branch)");
   Check(sb.attrs.size() == 1, fx.label + ": point attribute count is 1");
        }

        // 第二段：文件结构比对（V1.3 / V1.4）
        CheckFileStructure(outPath, fx.label);
    }

    // =========================================================================
    // V1.2 — 双精度属性不降精度
    // =========================================================================
    Section("V1.2 f64 and u8 attributes keep full precision (igv general format, section 2.4(B)3)");
    {
        // 上面的 "Unstructured(f64/u8 attribute)" 用例已经覆盖了逐值比对。
        // 这里再单独验证一件事：读回的属性**真的是 DoubleArray**，
        // 而不是"值恰好相等但类型被降成 float"。
        auto objA = MakeF64U8Fixture();
        const fs::path p = WorkDir() / "v12_f64u8.igv";
        IGVWriter::Pointer w = IGVWriter::New();
        if (!w->WriteToFile(objA, p.string())) {
   Check(false, "V1.2: writefailed - " + w->GetLastError());
        } else {
            IGVReader::Pointer r = IGVReader::New();
            DataObject::Pointer objB = r->ReadFile(p.string());
            if (!objB) {
    Check(false, "V1.2: read-back failed - " + r->GetLastError());
            } else {
                AttributeSet* set = objB->GetAttributeSet();
                AttributeSet::Attribute& d = set->GetAttribute(std::string("d_scalar"), IG_SCALAR);
    Check(!d.IsNone(), "V1.2: read-back object contains d_scalar");
                if (!d.IsNone()) {
     Check(d.GetPointer()->GetArrayTypedSize() ==
                              static_cast<IGsize>(sizeof(double)),
                          "V1.2: d_scalar is still 8 bytes (double) after read-back, not narrowed to float");
                    auto darr = DynamicCast<DoubleArray>(d.GetPointer());
     Check(darr != nullptr, "V1.2: d_scalar downcasts to DoubleArray");
                    if (darr) {
                        // 0.1 在 float32 下是 0.100000001490116...，与 double 不等
                        const double expect[4] = {0.1, 0.1234567890123, 1.0 / 3.0,
                                                  -2.718281828459045};
                        bool same = true;
                        for (int i = 0; i < 4; ++i) {
                            if (std::memcmp(&darr->RawPointer()[i], &expect[i],
                                            sizeof(double)) != 0) {
                                same = false;
                            }
                        }
      Check(same, "V1.2: d_scalar keeps all 4 doubles bit-identical"
                                    " (0.1 / 0.1234567890123 / 1/3 / -e are not narrowed to float)");
                    }
                }
                AttributeSet::Attribute& u = set->GetAttribute(std::string("u8_scalar"), IG_SCALAR);
    Check(!u.IsNone(), "V1.2: read-back object contains u8_scalar");
                if (!u.IsNone()) {
     Check(u.GetPointer()->GetArrayTypedSize() ==
                              static_cast<IGsize>(sizeof(unsigned char)),
                          "V1.2: u8_scalar is still 1 byte after read-back");
                    unsigned char got = 0;
                    if (auto ua = DynamicCast<UnsignedCharArray>(u.GetPointer())) {
                        got = ua->RawPointer()[1];
                    }
     Check(got == 200, "V1.2: u8_scalar[1] == 200 (not reinterpreted as signed, not narrowed)");
                }
            }
        }
    }

    // =========================================================================
    // V1.5 — 内存路径与文件路径等价（R13 / R14）
    // =========================================================================
    Section("V1.5 memory path equals file path (R13 / R14)");
    {
        auto objA = MakeF64U8Fixture();

        // 用 MemoryImage + 一个"磁盘上不存在的路径"命名，确保不会误读磁盘文件
        const fs::path ghost = WorkDir() / "v15_not_on_disk_at_all.igv";
        std::error_code ec;
        fs::remove(ghost, ec);
  Check(!fs::exists(ghost), "V1.5: target path really does not exist on disk (R14 precondition)");

        // 为什么不再走 WriteToFile() 再另起一个 writer：
        //
        // `FileWriter::WriteToFile()` = `SetInput()` + `SetFilePath()` + `Execute()`，
        // 而 `GenerateBuffers()` 开头会 `m_MemoryImage.clear()`。上一版因此写了两次：
        // 第一次 WriteToFile() 把镜像落到 ghost，第二次 `w2->SetDataObject(objA)`
        // 之后**直接**调 `GenerateBuffers()` —— 但那次调用没有设置输入对象，
        // `ValidateDataObject()` 报 "m_DataObject is empty"，返回 false，
        // 于是 V1.5 失败，而且日志里那条错误信息具有误导性
        // （看起来像"writer 不认识这个对象"，其实只是没喂输入）。
        //
        // 现在只写一次、只用一个 writer：`SetDataObject()` 就是 `SetInput(0, ...)`
        // 的别名（iGameFileWriter.cpp:253），走标准 `Execute()` 即可。
        IGVWriter::Pointer w = IGVWriter::New();
        w->SetWriteOptions(igv::IGVWriteOptions{});   // 默认 MemoryImage + gzip
        w->SetDataObject(objA);
        w->SetFilePath(ghost.string());
        const bool wrote = w->Execute();
  Check(wrote, "V1.5: write to the ghost path succeeded (memory image is kept)");
        if (wrote) {
            // 基类 Execute() 只清 m_Buffers，m_MemoryImage 保留（见 IGVWriter.h 说明）。
            const std::vector<char>& image = w->GetMemoryImage();
    Check(!image.empty(), "V1.5: memory image is not empty (" +
                                          std::to_string(image.size()) + " bytes)");

            if (!image.empty()) {
                // 1) 内存路径读
                IGVReader::Pointer rMem = IGVReader::New();
                rMem->SetMemoryBuffer(image.data(), image.size());
  Check(rMem->Execute(), "V1.5: memory path Execute succeeded");
                DataObject::Pointer objMem = rMem->GetOutput();
  Check(objMem != nullptr, "V1.5: memory path produced a non-null object");

                // 2) 文件路径读（同一份字节写到磁盘）
                const fs::path onDisk = WorkDir() / "v15_on_disk.igv";
                {
                    std::ofstream out(onDisk, std::ios::binary | std::ios::trunc);
                    out.write(image.data(), static_cast<std::streamsize>(image.size()));
                }
                IGVReader::Pointer rFile = IGVReader::New();
                DataObject::Pointer objFile = rFile->ReadFile(onDisk.string());
  Check(objFile != nullptr, "V1.5: file path read-back is non-null");

                if (objMem && objFile) {
                    CompareObjects(TakeSnapshot(objFile), TakeSnapshot(objMem),
                                   "V1.5 memory vs file");
                }

                // 3)（R13）先释放调用方缓冲，再触发 Reader 析构，
                //    断言不崩溃、不二次释放。
                {
                    std::vector<char> owned(image);
                    IGVReader::Pointer rTmp = IGVReader::New();
                    rTmp->SetMemoryBuffer(owned.data(), owned.size());
                    const bool e = rTmp->Execute();
  Check(e, "V1.5: caller-owned buffer path Execute succeeded");
                    owned.assign(owned.size(), 0);   // 先"污染"内容
                    owned.clear();
                    owned.shrink_to_fit();           // 再释放
                    rTmp = nullptr;                  // 再析构 Reader
  Check(true, "V1.5: caller buffer freed before the Reader is destroyed - no crash, no double free (R13)");
                }
            }
        }
    }

    // =========================================================================
    // V1.5b — R3 时序：CreateDataObject 在 HDF5 句柄已关闭后仍正确
    // =========================================================================
    Section("V1.5b R3 ordering check: CreateDataObject does not need an open HDF5 handle");
    {
        auto objA = MakeF64U8Fixture();
        const fs::path p = WorkDir() / "v15b_timing.igv";
        IGVWriter::Pointer w = IGVWriter::New();
        if (!w->WriteToFile(objA, p.string())) {
   Check(false, "V1.5b: writefailed - " + w->GetLastError());
        } else {
            IGVReader::Pointer r = IGVReader::New();
            r->SetAssertHdf5ClosedInCreateDataObject(true);
            DataObject::Pointer o = r->ReadFile(p.string());
   Check(o != nullptr,
                  "V1.5b: H5Fclose already happened inside Parsing(), yet CreateDataObject()"
                  " still rebuilds the object (proves it does not need an open HDF5 handle)");
        }
    }

    // =========================================================================
    // V1.6 — 读取 VTKHDF 参照物（格式兼容性反证）
    // =========================================================================
    Section("V1.6 reading VTKHDF references: must be cleanly rejected (P9-7 / section 1.7)");
    {
        const std::vector<std::string> vtkhdf = {
            modelsDir + "/append_slice.vtkhdf",
            modelsDir + "/can-vtu.vtkhdf",
            modelsDir + "/composite_partial_array.vtkhdf",
        };
        for (const std::string& f : vtkhdf) {
            if (!fs::exists(f)) {
    Info("skip (missing): " + f);
                continue;
            }
            IGVReader::Pointer r = IGVReader::New();
            DataObject::Pointer o = r->ReadFile(f);
            const std::string err = r->GetLastError();
   Check(o == nullptr, fs::path(f).filename().string() + ": rejected (returned nullptr)");
   Check(err.find("/IGV") != std::string::npos || err.find("IGV") != std::string::npos,
                  fs::path(f).filename().string() + ": error message points at the wrong root group - '" + err + "'");
        }
    }

    // =========================================================================
    // V1.6b — 读 P0 手写 golden 样例（双向锚定）
    // =========================================================================
    Section("V1.6b reading the P0 hand-written golden samples (independent anchor, not produced by IGVWriter)");
    {
        struct GoldenExpect {
            std::string file;
            int64_t points;
            int64_t cells;      ///< -1 = 无单元
            bool hasTopology;
            int64_t attrs;
        };
        const std::vector<GoldenExpect> goldens = {
            {"example_unstructured.igv", 5, 2, true, 2},
            {"example_pointset.igv", 3, -1, false, 1},
        };
        for (const auto& g : goldens) {
            const fs::path p = fs::path(goldenDir) / g.file;
            if (!fs::exists(p)) {
    Check(false, "V1.6b: golden sample not found: " + p.string());
                continue;
            }
            IGVReader::Pointer r = IGVReader::New();
            DataObject::Pointer o = r->ReadFile(p.string());
            if (!o) {
    Check(false, "V1.6b: read " + g.file + " failed - " + r->GetLastError());
                continue;
            }
            const Snapshot s = TakeSnapshot(o);
   Check(s.pointCount == g.points,
                  g.file + ": point count == " + std::to_string(g.points));
   Check(s.hasTopology == g.hasTopology,
                  g.file + ": topology presence == " + (g.hasTopology ? "yes" : "no"));
            if (g.hasTopology) {
    Check(s.cellCount == g.cells,
                      g.file + ": cell count == " + std::to_string(g.cells));
    Check(s.types.size() == 2 && s.types[0] == 9 && s.types[1] == 9,
                      g.file + ": all Types are IG_TETRA(9) - iGame codes, NOT VTK codes");
            }
   Check(static_cast<int64_t>(s.attrs.size()) == g.attrs,
                  g.file + ": attribute count == " + std::to_string(g.attrs));

            // golden 里的 pressure 是 f64 标量，必须原样读回
            if (g.file == "example_unstructured.igv") {
                const Snapshot::Attr* a = FindAttr(s, "pressure");
    Check(a != nullptr && a->valueSize == 8 && a->elementCount == 5,
                      g.file + ": pressure is five f64 values (valueSize=8)");
                if (a && a->f64.size() == 5) {
                    const double expect[5] = {0.5, 1.5, 2.5, 3.5, 4.5};
                    bool same = true;
                    for (int i = 0; i < 5; ++i) {
                        if (a->f64[static_cast<size_t>(i)] != expect[i]) { same = false; }
                    }
     Check(same, g.file + ": pressure values are equal");
                }
            }
        }
    }

    // =========================================================================
    // V1.7 — 写侧「拒绝写出」用例表（A9 的落点）
    // =========================================================================
    Section("V1.7 write-side rejection table (A9: validation must not degrade into asserts)");
    {
        // --- 1. Points 为 DoubleArray（模拟 f64 点坐标）---
        // 注意：iGame 的 Points 恒为 float32 缓冲区（iGamePoints.cpp: Points 内部
        // 就是 FloatArray），所以无法直接造出"f64 的 Points"。这一条改为验证
        // **等价的可达路径**：用一个点数为 0 的对象（下面第 3 项）与
        // "分量数不对"（第 2 项）来覆盖同一类校验分支。
        // 真正的 f64-Points 拒绝逻辑由 ValidateDataObject 的
        // GetArrayTypedSize() != sizeof(float) 分支承担，此处用直接构造的
        // 非 PointSet DataObject 触达不到，故显式记录为**未覆盖**而不是假通过。
  Info("V1.7-1 note: iGame's Points is always a float32 buffer (Points wraps a FloatArray "
             "internally), so an f64 Points input cannot be constructed. The branch (A8 hard "
             "check) exists in the code, but this test does NOT claim to cover it. This is a "
             "known coverage gap, not a pass.");

        // --- 2. Points 分量数为 2（用 Points::SetDimension 造不出来，
        //        改为构造一个点数正常但分量非 3 的场景不可行 ->
        //        同样记录为未覆盖）
  Info("V1.7-2 note: iGame's Points is constructed with SetDimension(3), so a Points with "
             "component count != 3 cannot be built from outside; this branch is not claimed as "
             "covered.");

        // --- 3. Points 为空 ---
        {
            auto ps = PointSet::New();
            ps->SetPoints(Points::New());   // 0 个点
            IGVWriter::Pointer w = IGVWriter::New();
            const bool ok = w->WriteToFile(ps, (WorkDir() / "reject_empty.igv").string());
   Check(!ok, "V1.7-3 Points is empty -> rejected");
   Info("  reason: " + w->GetLastError());
        }

        // --- 4–7. Types 白名单（IG_FACE=3 / IG_VOLUME=7 / IG_POLYHEDRON=8 / 999）---
        //
        // 注意 `IG_VOLUME` 在 iGame 里是**两个不同枚举**的成员：
        //   * `IGCellType::IG_VOLUME`（单元类型，值 7）—— iGameCellType.h:17
        //   * `iGameType.h:77` 的 DrawMode 枚举（值 1<<3）—— iGameType.h
        // 无限定名会报 C2872「不明确的符号」。这里要的是**单元类型**。
        struct BadType {
            int code;
            std::string name;
        };
        const std::vector<BadType> badTypes = {
            {static_cast<int>(IG_FACE), "IG_FACE(3)"},
            {static_cast<int>(IGCellType::IG_VOLUME), "IG_VOLUME(7)"},
            {static_cast<int>(IG_POLYHEDRON), "IG_POLYHEDRON(8)"},
            {999, "undefined large value 999"},
        };
        for (const BadType& bt : badTypes) {
            auto mesh = UnstructuredMesh::New();
            Points::Pointer pts = Points::New();
            pts->Resize(3);
            const float c[9] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
            std::memcpy(pts->RawPointer(), c, sizeof(c));
            mesh->SetPoints(pts);

            CellArray::Pointer cells = CellArray::New();
            igIndex ids[3] = {0, 1, 2};
            cells->AddCellIds(ids, 3);
            UnsignedIntArray::Pointer types = UnsignedIntArray::New();
            types->AddValue(static_cast<unsigned int>(bt.code));
            mesh->SetCells(cells, types);

            IGVWriter::Pointer w = IGVWriter::New();
            const bool ok = w->WriteToFile(mesh, (WorkDir() / "reject_type.igv").string());
   Check(!ok, "V1.7 Types containing " + bt.name + " -> rejected");
   Info("  reason: " + w->GetLastError());
        }

        // --- 8. 属性名含 '/' ---
        {
            auto ps = MakePointSetFixture();
            FloatArray::Pointer f = FloatArray::New();
            f->SetDimension(1);
            f->SetName("a/b");
            f->Resize(5);
            f->RawPointer()[0] = 1.f;
            ps->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, f);
            IGVWriter::Pointer w = IGVWriter::New();
            const bool ok = w->WriteToFile(ps, (WorkDir() / "reject_slash.igv").string());
   Check(!ok, "V1.7-8 attribute name containing '/' -> rejected");
   Info("  reason: " + w->GetLastError());
        }

        // --- 9. 属性名为空 ---
        {
            auto ps = MakePointSetFixture();
            FloatArray::Pointer f = FloatArray::New();
            f->SetDimension(1);
            f->SetName("");
            f->Resize(5);
            ps->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, f);
            IGVWriter::Pointer w = IGVWriter::New();
            const bool ok = w->WriteToFile(ps, (WorkDir() / "reject_noname.igv").string());
   Check(!ok, "V1.7-9 attribute name is empty -> rejected");
   Info("  reason: " + w->GetLastError());
        }

        // --- 10. 点属性 elementCount = pointCount + 1 ---
        {
            auto ps = MakePointSetFixture();   // 5 个点
            FloatArray::Pointer f = FloatArray::New();
            f->SetDimension(1);
            f->SetName("too_long");
            f->Resize(6);                       // 6 != 5
            ps->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, f);
            IGVWriter::Pointer w = IGVWriter::New();
            const bool ok = w->WriteToFile(ps, (WorkDir() / "reject_len.igv").string());
   Check(!ok, "V1.7-10 point attribute elementCount = pointCount+1 -> rejected");
   Info("  reason: " + w->GetLastError());
        }

        // --- 11. 单元属性 elementCount = cellCount - 1 ---
        {
            auto mesh = MakeF64U8Fixture();     // 1 个单元
            FloatArray::Pointer f = FloatArray::New();
            f->SetDimension(1);
            f->SetName("cell_short");
            f->Resize(0);                       // 0 != 1 -> 会被 IsNone? 不，空数组
            // Resize(0) 会让 AddAttribute 走 dimension 除法得到 0 个元素；
            // 这里改用 1 个元素但挂到 CELL 上再人为把 cellCount 抬高不可行，
            // 因此直接构造 2 个元素（!= 1）。
            f->Resize(2);
            mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, f);
            IGVWriter::Pointer w = IGVWriter::New();
            const bool ok = w->WriteToFile(mesh, (WorkDir() / "reject_celllen.igv").string());
   Check(!ok, "V1.7-11 cell attribute elementCount != cellCount -> rejected");
   Info("  reason: " + w->GetLastError());
        }

        // --- 12. 非 5 类网格 ---
        {
            auto raw = DataObject::New();   // IG_DATA_OBJECT = 0，不在五类之内
            IGVWriter::Pointer w = IGVWriter::New();
            const bool ok = w->WriteToFile(raw, (WorkDir() / "reject_type.igv").string());
   Check(!ok, "V1.7-12 mesh type outside the five (IG_DATA_OBJECT) -> rejected");
   Info("  reason: " + w->GetLastError());
        }

        // --- 13. m_DataObject 为空 ---
        {
            IGVWriter::Pointer w = IGVWriter::New();
            w->SetFilePath((WorkDir() / "reject_null.igv").string());
            // Execute() 会先取 input(0)；不设输入则为 null -> 基类就会拒绝
            const bool ok = w->Execute();
   Check(!ok, "V1.7-13 m_DataObject is null -> rejected (no crash)");
        }

        // --- 附加：多面体（二级索引）必须被拒绝（V1.7 第 6 项的实质含义）---
        {
            const std::string poly = modelsDir + "/iGameValidateCellsFilter_mixed.vtk";
            if (fs::exists(poly)) {
                DataObject::Pointer o = FileIO::ReadFile(poly);
                if (o) {
                    IGVWriter::Pointer w = IGVWriter::New();
                    const bool ok = w->WriteToFile(o, (WorkDir() / "reject_poly.igv").string());
                    // 该模型含多面体与否取决于读入结果；两种都记录，不做假通过
                    if (ok) {
      Info("V1.7-extra: iGameValidateCellsFilter_mixed.vtk written successfully"
                             " (its primary index is expressible; not a polyhedron secondary index)");
                    } else {
      Info("V1.7-extra: write rejected - " + w->GetLastError());
                    }
                }
            }
        }
    }

    // =========================================================================
    // V1.3 补充 — 压缩可通过选项关闭（§11.5）
    // =========================================================================
    Section("V1.3b compression=none really does not compress (section 11.5)");
    {
        // 造一个**点数足够多**的网格，否则"不压缩"是因为"本来就不压小数组"，
        // 而不是因为 compression=none 生效（§5.3 的 4 KiB 阈值是数据量判据）。
        //
        // 上一版的做法是给 4 个点的夹具硬挂一个 4096 元素的点属性 —— 那不是
        // "大网格"，而是**非法输入**：写侧的 elementCount 校验（§2.4(B)2）
        // 正当地拒绝了它（4096 != 4），于是 V1.3b 三项全红。
        // 也就是说：那条失败是测试自己的夹具错了，不是产品缺陷。
        //
        // 现在让点数与属性长度天然一致：
        //   n = 4096 点 → Points(4096×3 f32) = 48 KiB，att(4096 f32) = 16 KiB，
        //   两者都远超 4 KiB，因此 gzip 变体必然 chunk + DEFLATE。
        constexpr IGsize kV13bPoints = 4096;
        auto obj = UnstructuredMesh::New();

        Points::Pointer pts = Points::New();
        pts->Resize(kV13bPoints);
        for (IGsize i = 0; i < kV13bPoints; ++i) {
            // 平滑变化的坐标（gzip 友好），但每个点互不相同
            pts->RawPointer()[i * 3 + 0] = static_cast<float>(i % 64) * 0.25f;
            pts->RawPointer()[i * 3 + 1] = static_cast<float>((i / 64) % 64) * 0.25f;
            pts->RawPointer()[i * 3 + 2] = static_cast<float>(i / 4096) * 0.5f;
        }
        obj->SetPoints(pts);

        // 一个 4096 个四面体的网格（连通性 4096×4×4 B = 64 KiB，同样远超阈值）
        CellArray::Pointer cells = CellArray::New();
        UnsignedIntArray::Pointer types = UnsignedIntArray::New();
        for (IGsize i = 0; i + 3 < kV13bPoints; i += 4) {
            const igIndex ids[4] = {static_cast<igIndex>(i), static_cast<igIndex>(i + 1),
                                    static_cast<igIndex>(i + 2), static_cast<igIndex>(i + 3)};
            cells->AddCellIds(ids, 4);
            types->AddValue(static_cast<unsigned int>(IG_TETRA));
        }
        obj->SetCells(cells, types);

        {
            FloatArray::Pointer big = FloatArray::New();
            big->SetDimension(1);
            big->SetName("big");
            big->Resize(kV13bPoints);      // == pointCount，合法且够大
            for (IGsize i = 0; i < kV13bPoints; ++i) {
                big->RawPointer()[i] = static_cast<float>(i) * 0.5f;
            }
            obj->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, big);
        }

        const fs::path pGz = WorkDir() / "v13_gzip.igv";
        const fs::path pNone = WorkDir() / "v13_none.igv";

        IGVWriter::Pointer w1 = IGVWriter::New();
  Check(w1->WriteToFile(obj, pGz.string()), "V1.3b: gzip variant written");

        IGVWriter::Pointer w2 = IGVWriter::New();
        // 必须限定 `igv::`：这些类型在 `namespace iGame::igv` 里，而本文件只有
        // `using namespace iGame;`（不会把 `iGame::igv` 的成员带进来）。
        igv::IGVWriteOptions opt;
        opt.compression = igv::IGVCompression::None;
        w2->SetWriteOptions(opt);
  Check(w2->WriteToFile(obj, pNone.string()), "V1.3b: none variant written");

        IGVStructureReport r1, r2;
        std::string e1, e2;
        const bool ok1 = InspectIGVFile(pGz.string(), r1, e1);
        const bool ok2 = InspectIGVFile(pNone.string(), r2, e2);
  Check(ok1 && ok2, "V1.3b: both files can be structure-checked");
        if (ok1 && ok2) {
   Check(r1.hasDeflateForAllDataDatasets,
                  "V1.3b: every data dataset in the gzip variant carries a DEFLATE filter");
   Check(!r2.hasDeflateForAllDataDatasets,
                  "V1.3b: the none variant has NO DEFLATE filter (the option really takes effect)");
   Check(fs::file_size(pGz) < fs::file_size(pNone),
                  "V1.3b: gzip variant is smaller than none (" +
                      std::to_string(fs::file_size(pGz)) + " < " +
                      std::to_string(fs::file_size(pNone)) + ")");
            // 两份都要能读回且与源逐值相等
            IGVReader::Pointer rr1 = IGVReader::New();
            IGVReader::Pointer rr2 = IGVReader::New();
            DataObject::Pointer o1 = rr1->ReadFile(pGz.string());
            DataObject::Pointer o2 = rr2->ReadFile(pNone.string());
   Check(o1 != nullptr && o2 != nullptr, "V1.3b: both variants read back");
            if (o1 && o2) {
                CompareObjects(TakeSnapshot(obj), TakeSnapshot(o1), "V1.3b gzip");
                CompareObjects(TakeSnapshot(obj), TakeSnapshot(o2), "V1.3b none");
            }
        }
    }

    // =========================================================================
    // V1.4 补充 — DirectPath 直写入口
    // =========================================================================
    Section("V1.4b WriteToFilePathDirect direct-write path");
    {
        auto obj = MakeF64U8Fixture();
        const fs::path p = WorkDir() / "v14b_direct.igv";
        std::error_code ec;
        fs::remove(p, ec);

        IGVWriter::Pointer w = IGVWriter::New();
  Check(w->WriteToFilePathDirect(obj, p.string()),
              "V1.4b: WriteToFilePathDirect succeeded - " + w->GetLastError());
  Check(fs::exists(p) && fs::file_size(p) > 0,
              "V1.4b: target file was created (" +
                  std::to_string(fs::exists(p) ? fs::file_size(p) : 0) + " bytes)");

        IGVReader::Pointer r = IGVReader::New();
        DataObject::Pointer o = r->ReadFile(p.string());
  Check(o != nullptr, "V1.4b: direct-write output reads back - " + r->GetLastError());
        if (o) {
            CompareObjects(TakeSnapshot(obj), TakeSnapshot(o), "V1.4b direct write");
        }
    }

    // =========================================================================
    // V1.8 — 高阶 / 二次型单元往返（M2 全码值覆盖）
    // =========================================================================
    //
    // 为什么单列一节：`iGameIGVCellType.h` 的白名单有 33 项，而 P1 原先的夹具
    // 只覆盖 8 项（其余 25 项全是二次型 / 高阶 / Lagrange）。现实工程模型里几乎
    // 没有这些单元，因此由 kia/tools/MakeHighOrderCellModels.py 按
    // "1 文件 = 1 VTK 码值" 生成，见 kia/测试模型列表.md §5.2。
    //
    // 本节的价值不在于"几何正确"，而在于把**码值 ↔ iGame 枚举 ↔ IGV Types** 这条
    // 映射链在全部 33 个值上跑一遍。P1 §3.1 的真实缺陷（VTK 码被当成 IGCellType 码）
    // 正是这条链上最危险的一环。
    Section("V1.8 high-order / quadratic cell types round-trip (M2 full-code coverage)");
    {
        std::vector<std::string> highOrder;
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(modelsDir, ec)) {
            if (!entry.is_regular_file()) { continue; }
            const std::string name = entry.path().filename().string();
            // 命名约定见 MakeHighOrderCellModels.py：IGV_highorder_<VTK码>_<短名>.vtk
            if (name.rfind("IGV_highorder_", 0) == 0 && entry.path().extension() == ".vtk") {
                highOrder.push_back(entry.path().string());
            }
        }
        std::sort(highOrder.begin(), highOrder.end());

        if (highOrder.empty()) {
            Info("V1.8: no IGV_highorder_*.vtk found in " + modelsDir +
                 " - run `python kia/tools/MakeHighOrderCellModels.py` first."
                 " This section does NOT claim coverage.");
        } else {
            Info("V1.8: found " + std::to_string(highOrder.size()) +
                 " generated high-order model(s)");
        }

        for (const std::string& p : highOrder) {
            const std::string label = fs::path(p).stem().string();

            DataObject::Pointer objA = FileIO::ReadFile(p);
            if (!objA) {
                Check(false, label + ": FileIO::ReadFile failed (the VTK reader cannot "
                                  "build this cell type - record it as an upstream gap)");
                continue;
            }

            const Snapshot sa = TakeSnapshot(objA);
            // 读到了对象，但单元被读没了 = 上游 VTK 读取器没有该单元类型的分支
            // （iGameVTKAbstractReader.cpp 的 TransferVtkCellToiGameCell 里显式丢弃）。
            // 这不是 IGV 的问题，因此**明确报为覆盖缺口**而非失败
            // （kia/测试准则.md §2.4：未覆盖必须显式声明，不得伪造通过）。
            if (sa.types.empty() && sa.connectivity.empty()) {
                Info(label + ": read as an object with 0 cells - the iGame VTK reader has no "
                            "branch for this cell type, so this VALUE IS NOT COVERED by IGV "
                            "(upstream gap, see the test model list, section 4.1)");
                ++g_cellTypeUpstreamGaps;
                continue;
            }

            const fs::path out = WorkDir() / (label + ".igv");
            IGVWriter::Pointer w = IGVWriter::New();
            if (!w->WriteToFile(objA, out.string())) {
                Check(false, label + ": IGV write failed - " + w->GetLastError());
                continue;
            }
            IGVReader::Pointer r = IGVReader::New();
            DataObject::Pointer objB = r->ReadFile(out.string());
            if (!objB) {
                Check(false, label + ": IGV read-back failed - " + r->GetLastError());
                continue;
            }
            // Types 必须原样保留（这就是本节的实质断言）
            const Snapshot sb = TakeSnapshot(objB);
            Check(SameU8(sa.types, sb.types) && !sa.types.empty(),
                  label + ": Types preserved through IGV round-trip (" +
                      std::to_string(sa.types.size()) + " cell(s), first code=" +
                      (sa.types.empty() ? std::string("-") : std::to_string(sa.types[0])) + ")");
            CompareObjects(sa, sb, label);
            ++g_cellTypeValuesCovered;
        }
        Info("V1.8: cell-type values actually round-tripped: " +
             std::to_string(g_cellTypeValuesCovered) + "; upstream reading gaps: " +
             std::to_string(g_cellTypeUpstreamGaps));
    }

    // =========================================================================
    // V1.9 — VolumeMesh（`Examples/Models/hemisphere.rst`）
    // =========================================================================
    //
    // 为什么需要这一节：`GetOutput()` 在 VTK legacy 路径上**没有 VolumeMesh 分支**
    // （见 kia/测试模型列表.md §5.1），所以 `Examples/Models` 下的 62 个 .vtk
    // 一个都做不出 IG_VOLUME_MESH —— 五类网格里唯一没有文件来源的一类。
    // `.rst` 走 AnsysReader → PVD → VTU → FileReader::CreateDataObject() 的
    // numFaces+numVolumes 分支，是现成的 VolumeMesh 来源。
    //
    // 注意：AnsysReader 通过 `ExternalProcess` 启动外部转换器
    // `Resources/pyAnsysLib/ansys_to_pvd_converter.exe`，查找方式是
    // **相对于当前工作目录**的相对路径（iGameAnsysReader.cpp:29-42）。
    //
    // 本目录布局下，exe 在 `Examples/Release/`，`Resources/` 在 `Examples/`
    // —— **exe 旁边没有 Resources**。因此这里显式检测并把 CWD 切到
    // `Examples/`（那里同时有 `Resources/` 与 `Models/`），跑完再切回来。
    // 不这样做的话，`.rst` 会以"转换器找不到"静默失败，而被误读成
    // "iGame 不支持 VolumeMesh"。
    Section("V1.9 VolumeMesh (dual-index: faces + volumes)");
    {
        // 为什么是**直接构造**、不读文件：
        //
        // 1. VTK legacy 的 `GetOutput()` 没有 VolumeMesh 分支
        //    （iGameVTKReader.cpp:78-93）→ 62 个 .vtk 一个都做不出 IG_VOLUME_MESH；
        // 2. `hemisphere.rst` 实测走 AnsysReader → PVD → VTU，拿到的是
        //    **IG_COMPOSITE_DATA_OBJECT**（DataObjectType=2）—— PVD 交回的是
        //    多数据集集合，不是单个 VolumeMesh；多块要到 P6 才处理（§12.2）。
        //
        // 所以 VolumeMesh 只能直接构造。它是 iGame 里唯一"面 + 体"双索引的网格：
        // `FileReader::CreateDataObject()` 也正是在 numFaces && numVolumes 时才建它
        // （iGameFileReader.cpp:298-305）。
        //
        // 夹具刻意让**面数(6) != 体数(1)**：这正是"必须先取体数组、再取偏移"的
        // 边界。若实现误把面数当单元数，体属性的 elementCount 校验会立刻报错。
        {
            auto mesh = VolumeMesh::New();

            Points::Pointer pts = Points::New();
            pts->Resize(8);
            const float cube[24] = {0, 0, 0,  1, 0, 0,  1, 1, 0,  0, 1, 0,
                                    0, 0, 1,  1, 0, 1,  1, 1, 1,  0, 1, 1};
            std::memcpy(pts->RawPointer(), cube, sizeof(cube));
            mesh->SetPoints(pts);

            // 6 个四边形面（一级索引：面 -> 点）
            CellArray::Pointer faces = CellArray::New();
            const igIndex quad[6][4] = {
                {0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4},
                {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
            for (const auto& q : quad) {
                faces->AddCellIds(const_cast<igIndex*>(q), 4);
            }
            mesh->SetFaces(faces);

            // 1 个六面体体单元（一级索引：体 -> 点）
            CellArray::Pointer volumes = CellArray::New();
            const igIndex hex[8] = {0, 1, 2, 3, 4, 5, 6, 7};
            volumes->AddCellIds(const_cast<igIndex*>(hex), 8);
            mesh->SetVolumes(volumes);

            // 点属性 + 体属性；体属性长度必须是**体数**(1)，不是面数(6)
            {
                FloatArray::Pointer f = FloatArray::New();
                f->SetDimension(1);
                f->SetName("p_scalar");
                f->Resize(8);
                for (IGsize i = 0; i < 8; ++i) {
                    f->RawPointer()[i] = static_cast<float>(i) * 1.25f;
                }
                mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, f);
            }
            {
                FloatArray::Pointer f = FloatArray::New();
                f->SetDimension(1);
                f->SetName("vol_scalar");
                f->Resize(1);                    // == 体数；若误用面数 6，写侧会拒绝
                f->RawPointer()[0] = 42.0f;
                mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, f);
            }

            Info("V1.9: volume mesh fixture: DataObjectType=" +
                 std::to_string(static_cast<long long>(mesh->GetDataObjectType())) +
                 " faces=" + std::to_string(static_cast<long long>(mesh->GetNumberOfFaces())) +
                 " volumes=" + std::to_string(static_cast<long long>(mesh->GetNumberOfVolumes())));
            Check(mesh->GetDataObjectType() == IG_VOLUME_MESH,
                  "V1.9: fixture really is an IG_VOLUME_MESH");
            Check(mesh->GetNumberOfFaces() != mesh->GetNumberOfVolumes(),
                  "V1.9: face count differs from volume count (" +
                      std::to_string(static_cast<long long>(mesh->GetNumberOfFaces())) + " vs " +
                      std::to_string(static_cast<long long>(mesh->GetNumberOfVolumes())) +
                      ") - this is what catches 'using faces as cells'");

            const fs::path out = WorkDir() / "v19_volume_mesh.igv";
            IGVWriter::Pointer w = IGVWriter::New();
            if (!w->WriteToFile(mesh, out.string())) {
                Check(false, "V1.9: write failed - " + w->GetLastError());
            } else {
                Check(true, "V1.9: write OK (" + std::to_string(fs::file_size(out)) + " bytes)");
                IGVReader::Pointer r = IGVReader::New();
                DataObject::Pointer objB = r->ReadFile(out.string());
                if (!objB) {
                    Check(false, "V1.9: read-back failed - " + r->GetLastError());
                } else {
                    // IGV 只存**一级索引**（体 -> 点），所以读回按 UnstructuredMesh
                    // 语义表达（§3.7）。这里断言点数/体数/属性/Types 一致，
                    // 而不要求"读回仍是 VolumeMesh"。
                    const Snapshot sa = TakeSnapshot(mesh);
                    const Snapshot sb = TakeSnapshot(objB);
                    Info("V1.9: read back as DataObjectType=" +
                         std::to_string(static_cast<long long>(objB->GetDataObjectType())) +
                         " with " + std::to_string(sb.cellCount) + " cell(s)");
                    Check(sb.cellCount == static_cast<int64_t>(mesh->GetNumberOfVolumes()),
                          "V1.9: read-back cell count equals the VOLUME count (" +
                              std::to_string(sb.cellCount) + "), not the face count");
                    CompareObjects(sa, sb, "V1.9 VolumeMesh");
                    CheckFileStructure(out, "V1.9 VolumeMesh");
                }
            }
        }

        // ---- .rst 路径：记录实测结果（既非"通过"，也非静默跳过）----
        const std::string rst = modelsDir + "/hemisphere.rst";
        if (fs::exists(rst)) {
            fs::path converterRoot;
            for (const fs::path& root : roots) {
                if (fs::exists(root / "Resources" / "pyAnsysLib" / "ansys_to_pvd_converter.exe")) {
                    converterRoot = root;
                    break;
                }
            }
            if (converterRoot.empty()) {
                Info("V1.9 note: hemisphere.rst present but ansys_to_pvd_converter.exe not found "
                     "under any search root; the .rst path is NOT tested here.");
            } else {
                std::error_code switchEc;
                const fs::path savedCwd = fs::current_path(switchEc);
                fs::current_path(converterRoot, switchEc);
                DataObject::Pointer rstObj = FileIO::ReadFile(rst);
                if (!switchEc) { fs::current_path(savedCwd, switchEc); }

                if (!rstObj) {
                    Info("V1.9 note: FileIO::ReadFile(hemisphere.rst) returned null even with the "
                         "converter present (see the log for the converter output).");
                } else {
                    const long long t = static_cast<long long>(rstObj->GetDataObjectType());
                    Info("V1.9 note: hemisphere.rst reads as DataObjectType=" + std::to_string(t) +
                         " (IG_COMPOSITE_DATA_OBJECT=2 -> PVD handed back a multi-dataset "
                         "collection). It is therefore NOT a single VolumeMesh; MultiBlock is "
                         "P6 work (design doc section 12.2).");
                    // 明确不声称覆盖：只断言"如果它哪天变成 VolumeMesh，必须同时带
                    // cell array"，而不是硬要求它现在就是 VolumeMesh。
                    Check(t != IG_VOLUME_MESH || rstObj->GetCellArray() != nullptr,
                          "V1.9 note: if hemisphere.rst ever yields IG_VOLUME_MESH it must also "
                          "have a cell array for IGV to serialize");
                }
            }
        } else {
            Info("V1.9 note: " + rst + " not found");
        }
    }

    // =========================================================================
    // V1.10 — 大规模体网格（7×7×7 六面体，真实规模的 chunk + DEFLATE）
    // =========================================================================
    //
    // 为什么需要它：V1.3b 的 4096 点只能让"每一块都压缩"这条断言勉强成立，
    // 而 §5.3 的 chunk 策略（目标 1 MiB / 最小 4 KiB）只有在**多 chunk**时才真正
    // 被检验 —— 即 chunk 边界是否会把记录切断、chunk 尺寸是否合理。
    //
    // 规模：7³=343 点、6³=216 六面体单元 → Connectivity 1728 i32 (6.9 KiB)、
    // Points 4116 B (4.0 KiB，刚好越过阈值)。刻意**同时**带上点属性与体属性，
    // 并让体属性是 3 分量（覆盖 2*(1+k) 的 range 路径）。
    Section("V1.10 a real-scale hexahedral volume mesh (chunk + DEFLATE under load)");
    {
        constexpr int kN = 7;                 // 每轴点数
        constexpr int kCells = kN - 1;        // 每轴单元数
        const int nPoints = kN * kN * kN;
        const int nCells = kCells * kCells * kCells;

        auto mesh = VolumeMesh::New();

        Points::Pointer pts = Points::New();
        pts->Resize(static_cast<IGsize>(nPoints));
        for (int k = 0; k < kN; ++k) {
            for (int j = 0; j < kN; ++j) {
                for (int i = 0; i < kN; ++i) {
                    const IGsize idx = static_cast<IGsize>((k * kN + j) * kN + i);
                    // 略带起伏，避免"完全规则"导致 gzip 过度压缩而看不出规模
                    pts->RawPointer()[idx * 3 + 0] = static_cast<float>(i) + 0.1f * (k % 2);
                    pts->RawPointer()[idx * 3 + 1] = static_cast<float>(j) - 0.1f * (i % 2);
                    pts->RawPointer()[idx * 3 + 2] = static_cast<float>(k) * 1.5f;
                }
            }
        }
        mesh->SetPoints(pts);

        // 六面体体单元（一级索引：体 -> 点）
        CellArray::Pointer volumes = CellArray::New();
        auto pointId = [kN](int i, int j, int k) {
            return static_cast<igIndex>((k * kN + j) * kN + i);
        };
        for (int k = 0; k < kCells; ++k) {
            for (int j = 0; j < kCells; ++j) {
                for (int i = 0; i < kCells; ++i) {
                    const igIndex hex[8] = {
                        pointId(i, j, k),         pointId(i + 1, j, k),
                        pointId(i + 1, j + 1, k), pointId(i, j + 1, k),
                        pointId(i, j, k + 1),     pointId(i + 1, j, k + 1),
                        pointId(i + 1, j + 1, k + 1), pointId(i, j + 1, k + 1)};
                    volumes->AddCellIds(const_cast<igIndex*>(hex), 8);
                }
            }
        }
        mesh->SetVolumes(volumes);

        // 点属性：越界即报（长度必须 == 点数）
        {
            FloatArray::Pointer f = FloatArray::New();
            f->SetDimension(1);
            f->SetName("temperature");
            f->Resize(static_cast<IGsize>(nPoints));
            for (int p = 0; p < nPoints; ++p) {
                f->RawPointer()[p] = static_cast<float>(p) * 0.25f - 40.0f;
            }
            mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, f);
        }
        // 体属性：**3 分量** —— 覆盖 range 的 2*(1+k) 排列（§8.2）
        {
            FloatArray::Pointer f = FloatArray::New();
            f->SetDimension(3);
            f->SetName("cell_velocity");
            f->Resize(static_cast<IGsize>(nCells));     // == 体数
            for (int c = 0; c < nCells; ++c) {
                f->RawPointer()[c * 3 + 0] = static_cast<float>(c) * 0.5f;
                f->RawPointer()[c * 3 + 1] = -static_cast<float>(c) * 0.25f;
                f->RawPointer()[c * 3 + 2] = 1.0f;
            }
            mesh->GetAttributeSet()->AddAttribute(IG_VECTOR, IG_CELL, f);
        }

        Info("V1.10: fixture points=" + std::to_string(nPoints) +
             " volumes=" + std::to_string(nCells) +
             " faces=" + std::to_string(static_cast<long long>(mesh->GetNumberOfFaces())));

        const fs::path out = WorkDir() / "v110_volume_grid.igv";
        IGVWriter::Pointer w = IGVWriter::New();
        if (!w->WriteToFile(mesh, out.string())) {
            Check(false, "V1.10: write failed - " + w->GetLastError());
        } else {
            const auto bytes = fs::file_size(out);
            Check(true, "V1.10: write OK (" + std::to_string(bytes) + " bytes)");
            // 未压缩下界：Points(343*3*4) + Connectivity(216*8*4) + 属性 ≈ 16 KiB
            Check(bytes > 8192,
                  "V1.10: file is big enough to exercise multi-KiB datasets (" +
                      std::to_string(bytes) + " bytes)");
            IGVReader::Pointer r = IGVReader::New();
            DataObject::Pointer objB = r->ReadFile(out.string());
            if (!objB) {
                Check(false, "V1.10: read-back failed - " + r->GetLastError());
            } else {
                CompareObjects(TakeSnapshot(mesh), TakeSnapshot(objB), "V1.10 hex grid");
                CheckFileStructure(out, "V1.10 hex grid");
            }
        }
    }

    // =========================================================================
    // V1.11 — ASCII PLY：点云（→ PointSet）与三角表面（→ SurfaceMesh）
    // =========================================================================
    //
    // 为什么单列一节：`PLYReader` 有两个已知上游隐患（见 kia/测试模型列表.md §4.3）：
    //   1. **完全不检查 `format` 行**，一律按文本解析 —— 二进制 PLY 会读出垃圾数据；
    //   2. `ReadFaces()` 把面节点写进 `int vhs[16]` 且不校验 `Component`
    //      —— 面节点数 > 16 是栈溢出。
    // 本节用**ASCII** 文件把"能正确读"这一半钉住；二进制与 >16 的情形仍属未覆盖。
    //
    // 两个夹具的**预期类型不同**，这正是本节的重点：
    //   * `...NoFace.ply`（无 `element face`）→ `PointSet`（IG_POINT_SET）
    //     —— 这是**五类网格里唯一曾经没有文件来源的一类**；
    //   * `...Ascii.ply`（有 `element face`）→ `SurfaceMesh`（IG_SURFACE_MESH）。
    // 二者都来自 `FileReader::CreateDataObject()` 的既有分支
    // （iGameFileReader.cpp:307-325），不是 IGV 新写的逻辑。
    //
    // 跑 PLY 之前请先执行 `python build/igv_ply_preflight.py <file>` ——
    // 它会在读取前就暴露 `format` 非 ascii、列数不是 3、或面节点数 > 16
    // （最后一项会让 PLYReader 直接栈溢出，且崩因与本测试毫无关系）。
    Section("V1.11 ASCII PLY: point cloud (PointSet) and triangulated surface (SurfaceMesh)");
    {
        struct PlyCase {
            std::string file;
            IGenum expectType;
            int64_t expectPoints;
            int64_t expectCells;     ///< -1 = 该对象不应有单元
            bool expectTopology;
        };
        const std::vector<PlyCase> plies = {
            // 10,000 点、无 element face -> 无拓扑分支。用精确点数断言，
            // 因为 PLYReader 的循环写成 `if (VertexNum > 0)`，
            // 若头部解析回归成 0，它会读 0 个点却不报错（静默空对象）。
            {"BMW Manifold Ascii NoFace.ply", IG_POINT_SET, 10000, -1, false},
            {"BMW Manifold Ascii.ply", IG_SURFACE_MESH, 1999956, 4000000, true},
        };

        for (const PlyCase& c : plies) {
            const std::string p = modelsDir + "/" + c.file;
            if (!fs::exists(p)) {
                Check(false, "V1.11: model not found: " + c.file +
                                 " (run build/igv_ply_preflight.py to inspect a candidate)");
                continue;
            }
            const std::string label = c.file;
            Info("V1.11: reading " + label + " (" +
                 std::to_string(fs::file_size(p) / 1024 / 1024) + " MB)");

            DataObject::Pointer objA = FileIO::ReadFile(p);
            if (!objA) {
                Check(false, label + ": FileIO::ReadFile failed");
                continue;
            }
            const Snapshot sa = TakeSnapshot(objA);
            Info("V1.11: " + label + " -> DataObjectType=" +
                 std::to_string(static_cast<long long>(objA->GetDataObjectType())) +
                 " points=" + std::to_string(sa.pointCount) +
                 " cells=" + std::to_string(sa.cellCount));

            // 夹具自身的事实断言：点的**精确**数量与预期类型
            Check(objA->GetDataObjectType() == c.expectType,
                  label + ": DataObjectType == " + std::to_string(c.expectType) +
                      " (actual " + std::to_string(static_cast<long long>(
                          objA->GetDataObjectType())) + ")");
            Check(sa.pointCount == c.expectPoints,
                  label + ": point count == " + std::to_string(c.expectPoints) +
                      " (actual " + std::to_string(sa.pointCount) + ")");
            Check(sa.hasTopology == c.expectTopology,
                  std::string(label) + ": topology presence == " +
                      (c.expectTopology ? "yes" : "no"));
            if (c.expectCells >= 0) {
                Check(sa.cellCount == c.expectCells,
                      label + ": cell count == " + std::to_string(c.expectCells) +
                          " (actual " + std::to_string(sa.cellCount) + ")");
            }
            if (!c.expectTopology) {
                // 点云：读回也必须是空拓扑（§3.3 的分支，不只是"没读出来"）
                Check(!sa.hasTopology, label + ": point cloud object has no topology");
            }

            const fs::path out = WorkDir() / (label + ".igv");
            IGVWriter::Pointer w = IGVWriter::New();
            if (!w->WriteToFile(objA, out.string())) {
                Check(false, label + ": IGV write failed - " + w->GetLastError());
                continue;
            }
            Check(true, label + ": write OK (" +
                            std::to_string(fs::file_size(out) / 1024 / 1024) + " MB)");

            // 文件结构：点云**不得**有拓扑 dataset（§3.1 的分型规则）
            CheckFileStructure(out, label);
            {
                const hid_t f = H5Fopen(out.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
                if (f >= 0) {
                    const hid_t g = H5Gopen2(f, "IGV", H5P_DEFAULT);
                    if (g >= 0) {
                        const bool conn = H5Lexists(g, "Connectivity", H5P_DEFAULT) > 0;
                        const bool offs = H5Lexists(g, "Offsets", H5P_DEFAULT) > 0;
                        const bool tys = H5Lexists(g, "Types", H5P_DEFAULT) > 0;
                        const bool ncell = H5Lexists(g, "NumberOfCells", H5P_DEFAULT) > 0;
                        if (c.expectTopology) {
                            Check(conn && offs && tys && ncell,
                                  label + ": topology datasets present (Connectivity/Offsets/"
                                          "Types/NumberOfCells)");
                        } else {
                            Check(!conn && !offs && !tys && !ncell,
                                  label + ": NO topology datasets on disk (PointSet branch)");
                        }
                        H5Gclose(g);
                    }
                    H5Fclose(f);
                }
            }

            IGVReader::Pointer r = IGVReader::New();
            DataObject::Pointer objB = r->ReadFile(out.string());
            if (!objB) {
                Check(false, label + ": read-back failed - " + r->GetLastError());
                continue;
            }
            const Snapshot sb = TakeSnapshot(objB);
            // 点云：读回也应是 PointSet（而不是被"提升"成别的类型）
            Check(objB->GetDataObjectType() == c.expectType,
                  label + ": read-back DataObjectType == " + std::to_string(c.expectType));
            if (!c.expectTopology) {
                Check(!sb.hasTopology, label + ": read-back kept the empty-topology branch");
            }
            CompareObjects(sa, sb, label);
        }
    }

    // =========================================================================
    // 汇总
    // =========================================================================
 std::cerr << "\n================================================================\n"
              << "checks: " << g_checks << " total, failures " << g_failed << "\n";
    if (g_failed == 0) {
  std::cerr << "ALL PASSED: the V1.1-V1.11 acceptance items for IGV P1 hold in this run.\n";
        std::cerr << "  (cell-type values round-tripped in V1.8: " << g_cellTypeValuesCovered
                  << "; upstream iGame VTK-reader gaps: " << g_cellTypeUpstreamGaps << ")\n";
    } else {
  std::cerr << "FAILURES PRESENT - see the [FAIL] lines above.\n";
    }
 std::cerr << "================================================================\n";
    return g_failed == 0 ? 0 : 1;
}
