/**
 * @file TestIGVFileIO.cpp
 * @brief IGV FileIO integration tests.
 *
 * Verifies suffix dispatch and type names, write/read and memory read, existing-format
 * regression, and graceful failure when ENABLE_IGV_MODULE is off.
 *
 * Boundary cases: a renamed non-.igv file must not reach the IGV reader; a missing path
 * must fail cleanly; ReadIGVFromMemory(nullptr, 0) must be rejected before HDF5.
 *
 * Not covered here: the GUI file dialog and remote-package validation, which need a Qt
 * build and manual interaction.
 */

#include <iGameCellType.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGameStructuredMesh.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <iGameProgressObserver.h>

namespace {

using namespace iGame;
namespace fs = std::filesystem;

int g_checks = 0;
int g_failed = 0;

void Section(const std::string& name) {
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

/// 用例临时目录固定在 exe 旁边（与 P1 验收程序同一策略：不用
/// fs::temp_directory_path，避免沙箱/CI 上的权限差异）。
fs::path WorkDir() {
    static const fs::path dir = [] {
        fs::path d = fs::current_path() / "igv_fileio_tmp";
        std::error_code ec;
        fs::create_directories(d, ec);
        return d;
    }();
    return dir;
}

/// 定位 `Models/`（P2-8 需要一个真实的既有格式文件）。
/// 以 argv[0] 所在目录 / CWD / 源码目录为候选根，逐级向上找 ——
/// 与 P1 验收程序一致，不假设 "CWD 往上两级"。
fs::path FindModelsDir(const char* argv0) {
    std::vector<fs::path> roots;
    std::error_code ec;
    if (argv0 != nullptr && *argv0 != '\0') {
        const fs::path a0(argv0);
        if (a0.has_parent_path()) { roots.push_back(fs::absolute(a0, ec).parent_path()); }
    }
    roots.push_back(fs::current_path(ec));
#ifdef IGAME_IGV_SOURCE_DIR
    roots.emplace_back(IGAME_IGV_SOURCE_DIR);
#endif
    for (const fs::path& root : roots) {
        if (root.empty() || !fs::exists(root, ec)) { continue; }
        for (fs::path cur = root; !cur.empty(); cur = cur.parent_path()) {
            const fs::path candidate = cur / "Examples" / "Models";
            if (fs::is_directory(candidate, ec)) { return candidate; }
            if (cur == cur.parent_path()) { break; }
        }
    }
    return {};
}

// =============================================================================
// 夹具：一个带三种属性的 UnstructuredMesh
// =============================================================================
//
// 属性名刻意**避开** X/Y/Z 与 0/1/2 结尾：`FileIO::FinalizeLoadedObject()` 会把
// 这类名字的标量合并成向量（TransformScalars2VectorArray），那会让"往返前后属性
// 集合相同"的断言变成在测另一件事。这里要测的是分派层，不是属性合并。

UnstructuredMesh::Pointer MakeFixture() {
    auto mesh = UnstructuredMesh::New();

    Points::Pointer pts = Points::New();
    pts->Resize(5);
    const float coords[15] = {0.f,  0.f,  0.f,
                              1.f,  0.f,  0.f,
                              0.f,  1.f,  0.f,
                              0.f,  0.f,  1.f,
                              1.f,  1.f,  1.f};
    std::memcpy(pts->RawPointer(), coords, sizeof(coords));
    mesh->SetPoints(pts);

    // 两个四面体（IG_TETRA），共用一部分点：既覆盖多单元，也覆盖非定长无关路径。
    CellArray::Pointer cells = CellArray::New();
    igIndex tet0[4] = {0, 1, 2, 3};
    igIndex tet1[4] = {1, 2, 3, 4};
    cells->AddCellIds(tet0, 4);
    cells->AddCellIds(tet1, 4);
    UnsignedIntArray::Pointer types = UnsignedIntArray::New();
    types->AddValue(static_cast<unsigned int>(IG_TETRA));
    types->AddValue(static_cast<unsigned int>(IG_TETRA));
    mesh->SetCells(cells, types);

    AttributeSet* set = mesh->GetAttributeSet();

    // f32 点标量
    {
        FloatArray::Pointer f = FloatArray::New();
        f->SetDimension(1);
        f->SetName("p_scalar");
        f->Resize(5);
        const float v[5] = {0.5f, -1.25f, 3.f, 4.75f, -8.f};
        std::memcpy(f->RawPointer(), v, sizeof(v));
        set->AddAttribute(IG_SCALAR, IG_POINT, f);
    }
    // f64 点标量：取值刻意选在 float32 无法精确表示的位置，
    // 若封装层不小心把它窄化成 float，typedSize 与逐值比对会同时发现。
    {
        DoubleArray::Pointer d = DoubleArray::New();
        d->SetDimension(1);
        d->SetName("p_double");
        d->Resize(5);
        const double v[5] = {0.1, 0.1234567890123, 1.0 / 3.0, -2.718281828459045, 1e-30};
        std::memcpy(d->RawPointer(), v, sizeof(v));
        set->AddAttribute(IG_SCALAR, IG_POINT, d);
    }
    // 3 分量 f32 单元属性（2 个单元 → 6 个值）
    {
        FloatArray::Pointer f = FloatArray::New();
        f->SetDimension(3);
        f->SetName("cell_vec");
        f->Resize(2);
        const float v[6] = {1.5f, -2.25f, 0.f, 0.125f, 4.f, -1.f};
        std::memcpy(f->RawPointer(), v, sizeof(v));
        set->AddAttribute(IG_VECTOR, IG_CELL, f);
    }
    return mesh;
}

// =============================================================================
// 可比快照（逐值，不是近似）
// =============================================================================

struct Snapshot {
    IGenum type{IG_NONE};
    int64_t pointCount{0};
    int64_t cellCount{0};
    std::vector<float> points;
    std::vector<int32_t> connectivity;
    std::vector<uint8_t> cellTypes;

    /// 结构化网格的隐式拓扑（`StructuredMesh::GetCellArray()` 恒为 nullptr，
    /// 单元数由 Dimensions 隐式表达 —— 见 iGameStructuredMesh.h:25）。
    int64_t dims[3]{0, 0, 0};
    bool hasDims{false};

    struct Attr {
        std::string name;
        IGenum type{IG_NONE};
        IGenum attachment{IG_NONE};
        int components{1};
        IGsize typedSize{0};
        IGsize elementCount{0};
        std::vector<double> values;
    };
    std::vector<Attr> attrs;
};

Snapshot TakeSnapshot(DataObject::Pointer obj) {
    Snapshot s;
    if (!obj) { return s; }
    s.type = obj->GetDataObjectType();

    if (auto ps = DynamicCast<PointSet>(obj)) {
        if (Points::Pointer pts = ps->GetPoints()) {
            s.pointCount = static_cast<int64_t>(pts->GetNumberOfPoints());
            s.points.assign(pts->RawPointer(), pts->RawPointer() + s.pointCount * 3);
        }
    }

    if (CellArray::Pointer cells = obj->GetCellArray()) {
        s.cellCount = static_cast<int64_t>(cells->GetNumberOfCells());
        for (int64_t i = 0; i < s.cellCount; ++i) {
            const igIndex* ids = nullptr;
            const int n = cells->GetCellIds(static_cast<IGsize>(i), ids);
            for (int k = 0; k < n; ++k) {
                s.connectivity.push_back(static_cast<int32_t>(ids[k]));
            }
        }
    }
    if (auto um = DynamicCast<UnstructuredMesh>(obj)) {
        if (UnsignedIntArray* t = um->GetCellTypes()) {
            for (int64_t i = 0; i < s.cellCount; ++i) {
                s.cellTypes.push_back(
                        static_cast<uint8_t>(t->GetValue(static_cast<IGsize>(i))));
            }
        }
    }
    if (s.type == IG_STRUCTURED_MESH) {
        if (auto sm = DynamicCast<StructuredMesh>(obj)) {
            const igIndex* d = sm->GetDimensionSize();
            if (d != nullptr) {
                s.hasDims = true;
                s.dims[0] = d[0];
                s.dims[1] = d[1];
                s.dims[2] = d[2];
            }
        }
    }

    if (AttributeSet* set = obj->GetAttributeSet()) {
        const IGsize n = set->GetNumberOfAttributes();
        for (IGsize i = 0; i < n; ++i) {
            AttributeSet::Attribute& a = set->GetAttribute(i);
            Snapshot::Attr out;
            out.name = a.pointer ? a.pointer->GetName() : std::string();
            out.type = a.type;
            out.attachment = a.attachmentType;
            if (a.pointer) {
                out.components = a.pointer->GetDimension();
                out.typedSize = a.pointer->GetArrayTypedSize();
                out.elementCount = a.pointer->GetNumberOfElements();
                out.values.reserve(static_cast<size_t>(a.pointer->GetNumberOfValues()));
                for (IGsize k = 0; k < a.pointer->GetNumberOfValues(); ++k) {                    out.values.push_back(a.pointer->GetValue(k));
                }
            }
            s.attrs.push_back(std::move(out));
        }
    }
    return s;
}

bool SameFloats(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) { return false; }
    return a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0;
}

bool SameI32(const std::vector<int32_t>& a, const std::vector<int32_t>& b) {
    if (a.size() != b.size()) { return false; }
    return a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(int32_t)) == 0;
}

/// 逐值比较两个对象。返回 false 时由调用方打印原因（保持 Check 的粒度可见）。
void CompareObjects(const Snapshot& a, const Snapshot& b, const std::string& label) {
    Check(a.type == b.type, label + ": DataObjectType equal (" +
                                    std::to_string(static_cast<long long>(a.type)) + ")");
    Check(a.pointCount == b.pointCount, label + ": point count equal (" +
                                                std::to_string(a.pointCount) + ")");
    Check(a.cellCount == b.cellCount, label + ": cell count equal (" +
                                              std::to_string(a.cellCount) + ")");
    Check(SameFloats(a.points, b.points), label + ": Points bitwise equal (" +
                                                   std::to_string(a.points.size()) + " floats)");
    Check(SameI32(a.connectivity, b.connectivity),
          label + ": Connectivity equal and order preserved");
    Check(a.cellTypes == b.cellTypes, label + ": cell Types equal");
    Check(a.attrs.size() == b.attrs.size(), label + ": attribute count equal (" +
                                                       std::to_string(a.attrs.size()) + ")");
    if (a.attrs.size() != b.attrs.size()) { return; }
    for (size_t i = 0; i < a.attrs.size(); ++i) {
        const Snapshot::Attr& x = a.attrs[i];
        const Snapshot::Attr& y = b.attrs[i];
        const std::string tag = label + ": attribute '" + x.name + "'";
        Check(x.name == y.name, tag + " name");
        Check(x.type == y.type, tag + " type");
        Check(x.attachment == y.attachment, tag + " attachment");
        Check(x.components == y.components, tag + " components");
        Check(x.typedSize == y.typedSize, tag + " value size (no narrowing)");
        Check(x.elementCount == y.elementCount, tag + " element count");
        Check(x.values == y.values, tag + " values bitwise equal");
    }
}

/// 一个最小可用的 fixture：把 .igv 写到 @p path 并返回源对象；失败返回 nullptr。
DataObject::Pointer WriteFixture(const fs::path& path, std::string& error) {
    UnstructuredMesh::Pointer obj = MakeFixture();
    if (!FileIO::WriteFile(path.string(), obj)) {
        error = "FileIO::WriteFile returned false";
        return nullptr;
    }
    return obj;
}

} // namespace

int main(int argc, char** argv) {
    const fs::path work = WorkDir();
    const fs::path modelsDir = FindModelsDir(argc > 0 ? argv[0] : nullptr);

    std::cerr << "================================================================\n"
              << "IGV acceptance: FileIO integration\n"
              << "================================================================\n"
              << "Working dir : " << fs::current_path().string() << "\n"
              << "Temp dir    : " << work.string() << "\n"
              << "Models dir  : " << (modelsDir.empty() ? std::string("<not found>")
                                                       : modelsDir.string())
              << "\n"
              << "IGV_ENABLE  : "
#if defined(IGV_ENABLE)
              << "1 (reader/writer compiled in)\n";
#else
              << "0 (module disabled: expecting graceful degradation, P2-7)\n";
#endif

    // =========================================================================
    // P2-1 — 类型识别
    // =========================================================================
    Section("P2-1 FileIO::GetFileType recognizes .igv in any letter case");
    {
        Check(FileIO::GetFileType("a.igv") == FileIO::IGV, "GetFileType(\"a.igv\") == IGV");
        Check(FileIO::GetFileType("a.IGV") == FileIO::IGV, "GetFileType(\"a.IGV\") == IGV");
        Check(FileIO::GetFileType("a.Igv") == FileIO::IGV, "GetFileType(\"a.Igv\") == IGV");
        Check(FileIO::GetFileType("C:/tmp/dir/Model.IGV") == FileIO::IGV,
              "GetFileType of a full Windows-style path with .IGV == IGV");
        Check(FileIO::GetFileType("no_suffix_at_all") == FileIO::NONE,
              "GetFileType(\"no_suffix_at_all\") == NONE (unchanged)");
    }

    // =========================================================================
    // P2-2 — 类型名
    // =========================================================================
    Section("P2-2 FileIO::GetFileTypeAsString(IGV) returns \"IGV\"");
    {
        Check(FileIO::GetFileTypeAsString(FileIO::IGV) == "IGV",
              "GetFileTypeAsString(IGV) == \"IGV\"");
        Check(FileIO::GetFileTypeAsString(FileIO::NONE) == "NONE",
              "GetFileTypeAsString(NONE) == \"NONE\" (unchanged)");
        // IGV 必须是 FILETYPE_COUNT 之前的最后一个取值：这样它既被 switch 覆盖，
        Check(FileIO::IGV == FileIO::FILETYPE_COUNT - 1,
              "IGV is the last enumerator before FILETYPE_COUNT");
    }

#if defined(IGV_ENABLE)

    // =========================================================================
    // P2-4 — 主路径写
    // =========================================================================
    Section("P2-4 FileIO::WriteFile writes a readable .igv (main path)");
    const fs::path igvPath = work / "p2_fileio.igv";
    DataObject::Pointer objA;
    {
        std::string error;
        objA = WriteFixture(igvPath, error);
        Check(objA != nullptr, "FileIO::WriteFile(.igv) returned true" +
                                       (error.empty() ? std::string() : " - " + error));
        Check(fs::exists(igvPath), "the .igv file exists on disk");
        if (fs::exists(igvPath)) {
            Check(fs::file_size(igvPath) > 0, "the .igv file is not empty (" +
                                                      std::to_string(fs::file_size(igvPath)) +
                                                      " bytes)");
            Check(FileIO::GetFileType(igvPath.string()) == FileIO::IGV,
                  "GetFileType of the written path == IGV");
        }
    }

    // =========================================================================
    // P2-3 — 主路径读
    // =========================================================================
    Section("P2-3 FileIO::ReadFile reads the .igv back losslessly (main path)");
    DataObject::Pointer objFile;
    {
        objFile = FileIO::ReadFile(igvPath.string());
        Check(objFile != nullptr, "FileIO::ReadFile(.igv) returned a non-null object");
        if (objFile && objA) {
            CompareObjects(TakeSnapshot(objA), TakeSnapshot(objFile),
                           "P2-3 FileIO write -> FileIO read");
        }
        // 名字由 FileIO 统一设置为文件名（去扩展名），证明走的是通用收尾流程。
        Check(objFile && objFile->GetName() == "p2_fileio",
              "loaded object name is the file base name (\"p2_fileio\"), set by FileIO");
    }

    // =========================================================================
    // P2-5 — 内存读
    // =========================================================================
    Section("P2-5 FileIO::ReadIGVFromMemory equals the file path (same as V1.5)");
    {
        std::vector<char> image;
        {
            std::ifstream in(igvPath, std::ios::binary);
            image.assign(std::istreambuf_iterator<char>(in),
                         std::istreambuf_iterator<char>());
        }
        Check(!image.empty(), "the .igv bytes were read into memory (" +
                                      std::to_string(image.size()) + " bytes)");

        DataObject::Pointer objMem =
                image.empty() ? nullptr : FileIO::ReadIGVFromMemory(image.data(), image.size());
        Check(objMem != nullptr, "FileIO::ReadIGVFromMemory returned a non-null object");
        if (objMem && objFile) {
            CompareObjects(TakeSnapshot(objFile), TakeSnapshot(objMem),
                           "P2-5 memory vs file path");
        }

        // 入参守卫：封装层自己拒绝空指针 / 零长度，而不是把它交给 HDF5。
        Check(FileIO::ReadIGVFromMemory(nullptr, 0) == nullptr,
              "ReadIGVFromMemory(nullptr, 0) returns nullptr");
        Check(FileIO::ReadIGVFromMemory(image.data(), 0) == nullptr,
              "ReadIGVFromMemory(data, 0) returns nullptr");

        // 截断缓冲：Reader 必须失败而不是崩溃（HDF5 会拒绝，异常不外溢）。
        if (image.size() > 64) {
            DataObject::Pointer objTrunc =
                    FileIO::ReadIGVFromMemory(image.data(), image.size() / 2);
            Check(objTrunc == nullptr, "ReadIGVFromMemory(truncated image) returns nullptr");
        }
    }

    // =========================================================================
    // P2-11 — 读取进度事件（进度条驱动）
    // =========================================================================
    //
    // 为什么需要这一节：加进度条之前 IGV 的读取**一次都没有**回调过
    // `Filter::UpdateProgress`，所以 UI 上的进度条永远不动 —— 这不是"值不对"，
    // 而是"根本没有事件"。因此只断言"进度最终是 1.0"没有意义，必须断言
    // **中间确实有多个递增事件**，否则把 5 行 UpdateProgress 删掉测试也照样过。
    Section("P2-11 IGV reading reports incremental progress (0 -> 1) for the progress bar");
    {
        std::vector<double> reported;
        ProgressObserver* observer = ProgressObserver::Instance();
        const unsigned long tag = observer->AddObserver(
                Command::ProgressEvent,
                [&reported](Object*, unsigned long, void* data) -> void {
                    if (data != nullptr) {
                        reported.push_back(*static_cast<double*>(data));
                    }
                });

        DataObject::Pointer obj = FileIO::ReadFile(igvPath.string());
        observer->RemoveObserver(tag);

        Check(obj != nullptr, "the .igv file still reads back while a progress observer is attached");

        // 忽略末尾的复位 0.0：FileReader::Execute() 结束时会 resetProgressUI，
        // UI 靠它把进度条收起来（与 VTU/VTP/CGNS 等既有 Reader 完全一致）。
        size_t peakIndex = 0;
        for (size_t i = 0; i < reported.size(); ++i) {
            if (reported[i] >= 1.0) { peakIndex = i; break; }
            peakIndex = i;
        }
        bool monotonic = true;
        for (size_t i = 1; i <= peakIndex && i < reported.size(); ++i) {
            if (reported[i] < reported[i - 1]) { monotonic = false; }
        }
        const bool reachedOne = std::find(reported.begin(), reported.end(), 1.0) != reported.end();

        Info("progress events: " + std::to_string(reported.size()) +
             " (first " + (reported.empty() ? std::string("-")
                                            : std::to_string(reported.front())) +
             ", peak reached 1.0: " + (reachedOne ? "yes" : "no") + ")");
        Check(reported.size() >= 5,
              "reading emitted at least 5 progress events (" +
                      std::to_string(reported.size()) +
                      "), i.e. the bar actually moves instead of being frozen");
        Check(monotonic, "progress values never go backwards before reaching 1.0");
        Check(reachedOne, "reading finally reaches progress 1.0");
    }

#else // !defined(IGV_ENABLE)

    // =========================================================================
    // P2-7 — 宏关闭时的降级（在 ENABLE_IGV_MODULE=OFF 的构建上执行）
    // =========================================================================
    Section("P2-7 with ENABLE_IGV_MODULE=OFF: suffix still recognized, I/O declines");
    {
        const fs::path igvPath = work / "p2_off_mode.igv";
        {
            std::ofstream out(igvPath, std::ios::binary | std::ios::trunc);
            out << "not a real igv file";
        }

        Info("This build has no IGV reader/writer: the checks below verify the "
             "degradation path, not the format itself.");

        Check(FileIO::GetFileType(igvPath.string()) == FileIO::IGV,
              "GetFileType still returns IGV when the module is disabled");
        Check(FileIO::ReadFile(igvPath.string()) == nullptr,
              "ReadFile(.igv) returns nullptr (no crash) when the module is disabled");
        Check(FileIO::ReadIGVFromMemory("abc", 3) == nullptr,
              "ReadIGVFromMemory returns nullptr when the module is disabled");
        Check(!FileIO::WriteFile(igvPath.string(), MakeFixture()),
              "WriteFile(.igv) returns false when the module is disabled");
        Check(FileIO::GetFileTypeAsString(FileIO::IGV) == "IGV",
              "GetFileTypeAsString(IGV) == \"IGV\" in the disabled build too");

        std::error_code ec;
        fs::remove(igvPath, ec);
    }

#endif // IGV_ENABLE

    // =========================================================================
    // P2-6 — 后缀无关性（改名后走 NONE 分支）
    // =========================================================================
    Section("P2-6 a non-.igv suffix is not dispatched to the IGV reader");
    {
        const fs::path binPath = work / "p2_not_igv.bin";
        std::error_code ec;
        fs::remove(binPath, ec);
        if (fs::exists(work / "p2_fileio.igv")) {
            fs::copy_file(work / "p2_fileio.igv", binPath,
                          fs::copy_options::overwrite_existing, ec);
        } else {
            std::ofstream out(binPath, std::ios::binary | std::ios::trunc);
            out << "placeholder";
        }
        Check(fs::exists(binPath), "the renamed copy exists on disk");
        Check(FileIO::GetFileType(binPath.string()) == FileIO::NONE,
              "GetFileType(\"p2_not_igv.bin\") == NONE");
        Check(FileIO::ReadFile(binPath.string()) == nullptr,
              "ReadFile(\"p2_not_igv.bin\") returns nullptr and does not crash");

        // 不存在的 .igv 路径：后缀对，但文件读不到。分派不能因此崩溃。
        const fs::path missing = work / "p2_does_not_exist.igv";
        fs::remove(missing, ec);
        Check(FileIO::ReadFile(missing.string()) == nullptr,
              "ReadFile of a missing .igv path returns nullptr and does not crash");
    }

    // =========================================================================
    // P2-8 — 既有格式零回归
    // =========================================================================
    Section("P2-8 no regression on existing formats (suffix table + a real read/write)");
    {
        struct SuffixCase {
            const char* file;
            IGenum expected;
        };
        const SuffixCase cases[] = {
                {"a.vtk", FileIO::VTK},        {"a.vtu", FileIO::VTU},
                {"a.vtp", FileIO::VTP},        {"a.vts", FileIO::VTS},
                {"a.vtr", FileIO::VTR},        {"a.vtm", FileIO::VTM},
                {"a.pvd", FileIO::PVD},        {"a.obj", FileIO::OBJ},
                {"a.off", FileIO::OFF},        {"a.stl", FileIO::STL},
                {"a.ply", FileIO::PLY},        {"a.mesh", FileIO::MESH},
                {"a.igc", FileIO::IGC},        {"a.igcm", FileIO::IGCM},
                {"a.inp", FileIO::INP},        {"a.cas", FileIO::CAS},
                {"a.ccm", FileIO::CCM},        {"a.rst", FileIO::RST},
                {"a.rth", FileIO::RTH},        {"a.bdf", FileIO::BDF},
                {"a.e", FileIO::EX2},          {"a.ex2", FileIO::EX2},
                {"a.d3plot", FileIO::D3PLOT},  {"a.dat", FileIO::SPECTRAL_DAT},
                {"a.fld", FileIO::SPECTRAL_NEKTAR},
        };
        bool allSuffixesOk = true;
        std::string firstBad;
        for (const SuffixCase& c : cases) {
            const IGenum got = FileIO::GetFileType(c.file);
            if (got != c.expected) {
                allSuffixesOk = false;
                if (firstBad.empty()) {
                    firstBad = std::string(c.file) + " -> " + FileIO::GetFileTypeAsString(got);
                }
            }
        }
        Check(allSuffixesOk, "all " + std::to_string(sizeof(cases) / sizeof(cases[0])) +
                                     " pre-existing suffixes still map to their own types" +
                                     (firstBad.empty() ? std::string() : " (first bad: " + firstBad + ")"));

        // 8.2 类型 → 名字：名字与枚举同名的那些逐条比对；两个历史特例单独固定。
        struct NameCase {
            IGenum type;
            const char* expected;
        };
        const NameCase names[] = {
                {FileIO::VTK, "VTK"},         {FileIO::IGC, "IGC"},
                {FileIO::IGCM, "IGCM"},       {FileIO::OBJ, "OBJ"},
                {FileIO::STL, "STL"},         {FileIO::PLY, "PLY"},
                {FileIO::VTU, "VTU"},         {FileIO::VTP, "VTP"},
                {FileIO::VTM, "VTM"},         {FileIO::VTS, "VTS"},
                {FileIO::VTR, "VTR"},         {FileIO::PVD, "PVD"},
                {FileIO::CGNS, "CGNS"},       {FileIO::INP, "INP"},
                {FileIO::CAS, "CAS"},         {FileIO::CCM, "CCM"},
                {FileIO::RST, "RST"},         {FileIO::RTH, "RTH"},
                {FileIO::BDF, "BDF"},         {FileIO::D3PLOT, "D3PLOT"},
                // 这两个是既有实现里的"不叫枚举名"的历史特例，必须显式钉住：
                // 一旦有人把它们改成枚举名，日志与既有脚本的可读性会变。
                {FileIO::SPECTRAL_DAT, "Jacobi DAT"},
                {FileIO::SPECTRAL_NEKTAR, "Nektar++ XML/FLD"},
                {FileIO::IGV, "IGV"},
        };
        bool allNamesOk = true;
        std::string firstBadName;
        for (const NameCase& c : names) {
            const std::string got = FileIO::GetFileTypeAsString(c.type);
            if (got != c.expected) {
                allNamesOk = false;
                if (firstBadName.empty()) {
                    firstBadName = std::string(c.expected) + " != \"" + got + "\"";
                }
            }
        }
        Check(allNamesOk, "GetFileTypeAsString is unchanged for all " +
                                  std::to_string(sizeof(names) / sizeof(names[0])) +
                                  " checked types (22 pre-existing + IGV)" +
                                  (firstBadName.empty() ? std::string() : " (" + firstBadName + ")"));

        // 8.3 真跑一个既有格式：写 .vtk → 读回，再读一个仓库里的真实模型。
        //     这条是"分派表没被写坏"的行为级证据，比查表更硬。
        const fs::path vtkOut = work / "p2_regression.vtk";
        {
            UnstructuredMesh::Pointer obj = MakeFixture();
            const bool wrote = FileIO::WriteFile(vtkOut.string(), obj);
            Check(wrote, "FileIO::WriteFile(.vtk) still succeeds for an existing format");
            if (wrote) {
                DataObject::Pointer back = FileIO::ReadFile(vtkOut.string());
                Check(back != nullptr, "FileIO::ReadFile(.vtk) still reads that file back");
                if (back) {
                    const Snapshot sa = TakeSnapshot(obj);
                    const Snapshot sb = TakeSnapshot(back);
                    Check(sb.type == IG_UNSTRUCTURED_MESH,
                          "read-back .vtk is still an UnstructuredMesh");
                    Check(sb.pointCount == sa.pointCount && sb.cellCount == sa.cellCount,
                          "read-back .vtk keeps point/cell counts (" +
                                  std::to_string(sb.pointCount) + "/" +
                                  std::to_string(sb.cellCount) + ")");
                }
            }
        }

        // 真实模型：断言"仍能读、类型没变成 NONE、点数与单元数非零"。
        //
        // 为什么**不**逐条钉死 `GetDataObjectType()` 的具体取值：
        // 那是既有 VTK Reader 的映射结果（例如 `CellCenter_surface_mixed.vtk` 的
        // 头部是 `DATASET UNSTRUCTURED_GRID`，因此得到 `UnstructuredMesh` ——
        // P1 记录里把它当作 SurfaceMesh 夹具，指的是它在夹具里的角色，不是
        // `GetDataObjectType()`）。P2 完全不碰 VTK 读取，把它钉成某个具体值只会
        // 把"既有 Reader 的映射"当成 P2 的规范，反而掩盖真正的分派回归。
        // 这里真正要防的是"分派把 .vtk 交给了 IGV" —— 那会得到 nullptr 或
        // 类型 NONE，下面两条断言都能抓到，并打印实际类型供人工核对。
        struct ModelCase {
            const char* fileName;
        };
        const ModelCase models[] = {
                {"CellCenter_surface_mixed.vtk"},
                {"LinearExtrusion_ClosedCube.vtk"},
                {"Structured_Volume_Test.vtk"},
        };
        if (modelsDir.empty()) {
            Info("skip: Models directory not found; the real-model regression checks are "
                 "NOT covered in this run");
        }
        for (const ModelCase& mc : models) {
            const fs::path model = modelsDir.empty() ? fs::path() : modelsDir / mc.fileName;
            if (!fs::exists(model)) {
                Info(std::string("skip (missing): ") + mc.fileName);
                continue;
            }
            Check(FileIO::GetFileType(model.string()) == FileIO::VTK,
                  std::string("GetFileType still says VTK for ") + mc.fileName);
            DataObject::Pointer obj = FileIO::ReadFile(model.string());
            Check(obj != nullptr, std::string("an existing model still loads: ") + mc.fileName);
            if (!obj) { continue; }
            const Snapshot s = TakeSnapshot(obj);
            Info(std::string(mc.fileName) + ": DataObjectType=" +
                 std::to_string(static_cast<long long>(s.type)) + " points=" +
                 std::to_string(s.pointCount) + " cells=" + std::to_string(s.cellCount));
            Check(s.type != IG_NONE,
                  std::string(mc.fileName) + ": read result has a real mesh type (not NONE)");
            // 结构化网格没有 CellArray（拓扑由 Dimensions 隐式表达），所以对它的
            // 正确断言是「Dimensions 乘积 == 点数」，而不是「单元数 > 0」。
            if (s.type == IG_STRUCTURED_MESH) {
                Check(s.hasDims && s.dims[0] * s.dims[1] * s.dims[2] == s.pointCount,
                      std::string(mc.fileName) +
                              ": structured mesh keeps Dimensions * == point count (" +
                              std::to_string(s.dims[0]) + "x" + std::to_string(s.dims[1]) +
                              "x" + std::to_string(s.dims[2]) + " == " +
                              std::to_string(s.pointCount) + ")");
            } else {
                Check(s.pointCount > 0 && s.cellCount > 0,
                      std::string(mc.fileName) + ": still has points and cells (" +
                              std::to_string(s.pointCount) + "/" +
                              std::to_string(s.cellCount) + ")");
            }
        }
    }

    // =========================================================================
    // P2-9 / P2-10 — 明确声明不在本程序范围内
    // =========================================================================
    Section("P2-9 / P2-10 scope statement (not covered by this program)");
    {
        Info("P2-9 (IGV visible in the Qt open dialog) and P2-10 (remote package "
             "validation unit test) are Qt-side items: they need a Qt build plus a "
             "manual dialog check. Their source-level changes are verified by "
             "; this program does NOT claim to cover them.");
    }

    std::cerr << "\n================================================================\n"
              << "checks: " << g_checks << " total, failures " << g_failed << "\n";
    if (g_failed == 0) {
        std::cerr << "ALL PASSED: the P2 acceptance items reachable from this program hold.\n";
#if !defined(IGV_ENABLE)
        std::cerr << "  (this run used the ENABLE_IGV_MODULE=OFF build: P2-3/4/5 were "
                     "skipped by design, P2-7 was verified instead)\n";
#endif
    } else {
        std::cerr << "FAILURES: " << g_failed << " check(s) did not pass.\n";
    }
    std::cerr << "================================================================\n";
    return g_failed == 0 ? 0 : 1;
}
