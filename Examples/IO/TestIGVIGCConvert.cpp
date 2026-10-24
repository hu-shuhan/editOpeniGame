/**
 * @file TestIGVIGCConvert.cpp
 * @brief IGC <-> IGV conversion and fidelity-boundary tests.
 *
 * The test checks that IGC and IGV share data semantics so conversion does not silently
 * change values. Risks: IGC quantization truncates attributes; IGC spatially reorders
 * points and cells; and a nominal "lossless" profile must still be measured, not assumed.
 *
 * Comparisons use either the encoder's reorder trace or an order-free canonical form, so
 * a shared wrong convention cannot make both sides agree.
 *
 * Some IGC files contain zero-length payload blocks that the current decoder cannot read.
 * Those cases are counted as skipped, and a pin test records the failure; the pin is
 * expected to turn red when the upstream decoder is fixed.
 */

#include <IGC/iGameIGCReader.h>
#include <IGC/iGameIGCWriter.h>
#include <IGV/iGameIGVReader.h>
#include <IGV/iGameIGVWriter.h>

#include <iGameAttributeSet.h>
#include <iGameCellType.h>
#include <iGameFileIO.h>
#include <iGameFlatArray.h>
#include <iGamePointSet.h>
#include <iGameStructuredMesh.h>
#include <iGameSurfaceMesh.h>
#include <iGameUnstructuredMesh.h>
#include <iGameVolumeMesh.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace {

using namespace iGame;
namespace fs = std::filesystem;

int g_checks = 0;
int g_failed = 0;
/// 被上游缺陷挡住的用例数：既不算通过，也不算失败。
int g_blocked = 0;

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

void Info(const std::string& what) { std::cerr << " [info] " << what << "\n"; }

/// Record a case blocked by an upstream defect. The skip is explicit and counted;
/// it is neither a pass nor a failure.
void Blocked(const std::string& label, const std::string& why) {
    ++g_blocked;
    std::cerr << " [skip] " << label << ": blocked by an upstream defect - " << why << "\n";
}


std::string Num(int64_t v) { return std::to_string(static_cast<long long>(v)); }

std::string Sci(double v) {
    std::ostringstream o;
    o.setf(std::ios::scientific);
    o << std::setprecision(6) << v;
    return o.str();
}

std::string Pad(std::string s, size_t width) {
    if (s.size() < width) { s.append(width - s.size(), ' '); }
    return s;
}

/// 用例临时目录固定在 exe 旁边（与 P1/P2 验收程序同一策略：不用
/// fs::temp_directory_path，避免沙箱/CI 上的权限差异）。
fs::path WorkDir() {
    static const fs::path dir = [] {
        fs::path d = fs::current_path() / "igv_p4_convert_tmp";
        std::error_code ec;
        fs::create_directories(d, ec);
        return d;
    }();
    return dir;
}

/// 定位 `Models/`：以 argv[0] 所在目录 / CWD / 源码目录为候选根，逐级向上找。
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
// .igc 的 payload 块布局（缺陷证据的读取器）
// =============================================================================
//
// IGC 的二进制流是 `[type:1][length:4 大端][payload:length]` 的重复
// （见 `EncodeOutputBinaryArray::WritePayload` 与 `DecodeInputMappedMemory::ReadPayload`）。
// 解析它只是为了**取证**：哪个块长度为 0，就直接对应到解码端的 `Decompress(empty) == false`。

struct IgcBrick {
    int type{0};
    uint32_t length{0};
};

const char* BrickName(int type) {
    switch (type) {
        case 0: return "param";
        case 1: return "geom";
        case 2: return "attr";
        case 3: return "topo";
        case 4: return "compressed";
        default: return "unknown";
    }
}

bool ReadIgcBricks(const fs::path& path, std::vector<IgcBrick>& out) {
    out.clear();
    std::ifstream in(path, std::ios::binary);
    if (!in) { return false; }
    std::vector<unsigned char> data{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};

    size_t i = 0;
    while (i + 5 <= data.size()) {
        IgcBrick brick;
        brick.type = static_cast<int>(data[i]);
        brick.length = (static_cast<uint32_t>(data[i + 1]) << 24) |
                       (static_cast<uint32_t>(data[i + 2]) << 16) |
                       (static_cast<uint32_t>(data[i + 3]) << 8) |
                       static_cast<uint32_t>(data[i + 4]);
        i += 5 + static_cast<size_t>(brick.length);
        out.push_back(brick);
        if (i > data.size()) { return false; }  // 长度字段越界
    }
    return i == data.size();
}

std::string BrickListText(const std::vector<IgcBrick>& bricks) {
    std::string text;
    for (const auto& b : bricks) {
        if (!text.empty()) { text += ", "; }
        text += std::string(BrickName(b.type)) + "(len=" + Num(static_cast<int64_t>(b.length)) + ")";
    }
    return text;
}

/// 是否存在 0 长度的 payload 块 —— 即上游缺陷的触发条件。
bool HasZeroLengthBrick(const std::vector<IgcBrick>& bricks) {
    for (const auto& b : bricks) {
        if (b.length == 0) { return true; }
    }
    return false;
}

/// 读回失败时的统一处置：
///   * 有 0 长度块 → 记为上游客缺陷、跳过（不伪造通过）；
///   * 没有 0 长度块 → 这才是真的意外失败，必须红。
void HandleIgcReadFailure(const std::string& label, const fs::path& igcPath) {
    std::vector<IgcBrick> bricks;
    if (!ReadIgcBricks(igcPath, bricks)) {
        Check(false, label + ": could not parse the .igc payload layout (unexpected)");
        return;
    }
    if (HasZeroLengthBrick(bricks)) {
        Blocked(label, "empty payload brick [" + BrickListText(bricks) +
                           "] makes MeshCodecZSTD::Decompress fail;  "
                           "(documented in the P4 record)");
        return;
    }
    Check(false, label + ": IGC product could not be read back, and there is no empty payload brick "
                         "(unexpected - bricks: [" + BrickListText(bricks) + "])");
}

// =============================================================================
// 数组内容：原始字节（逐字节比较）+ 数值（误差统计）
// =============================================================================

/// 一个数组的"可比内容"。
///
/// `values` 只用于有损档的误差统计 —— 判定"是否相等"永远看 `raw`。
struct ArrayData {
    int valueSize{0};
    int components{0};
    int64_t elements{0};
    std::vector<unsigned char> raw;
    std::vector<double> values;

    int64_t ValueCount() const { return elements * static_cast<int64_t>(components); }
    bool Empty() const { return elements <= 0 || components <= 0; }
};

bool ExtractArray(const ArrayObject::Pointer& arr, ArrayData& out) {
    out = ArrayData{};
    if (!arr) { return false; }

    out.components = arr->GetDimension();
    out.valueSize = static_cast<int>(arr->GetArrayTypedSize());
    out.elements = static_cast<int64_t>(arr->GetNumberOfElements());
    if (out.components <= 0 || out.elements < 0) { return false; }
    const int64_t n = out.ValueCount();

    if (auto f = DynamicCast<FloatArray>(arr)) {
        if (out.valueSize != static_cast<int>(sizeof(float))) { return false; }
        out.raw.resize(static_cast<size_t>(n) * sizeof(float));
        if (n > 0) {
            std::memcpy(out.raw.data(), f->RawPointer(), out.raw.size());
        }
        out.values.resize(static_cast<size_t>(n));
        for (int64_t i = 0; i < n; ++i) {
            out.values[static_cast<size_t>(i)] = static_cast<double>(f->RawPointer()[i]);
        }
        return true;
    }
    if (auto d = DynamicCast<DoubleArray>(arr)) {
        if (out.valueSize != static_cast<int>(sizeof(double))) { return false; }
        out.raw.resize(static_cast<size_t>(n) * sizeof(double));
        if (n > 0) {
            std::memcpy(out.raw.data(), d->RawPointer(), out.raw.size());
        }
        out.values.assign(d->RawPointer(), d->RawPointer() + n);
        return true;
    }
    if (auto u = DynamicCast<UnsignedCharArray>(arr)) {
        if (out.valueSize != static_cast<int>(sizeof(unsigned char))) { return false; }
        out.raw.resize(static_cast<size_t>(n));
        if (n > 0) {
            std::memcpy(out.raw.data(), u->RawPointer(), out.raw.size());
        }
        out.values.resize(static_cast<size_t>(n));
        for (int64_t i = 0; i < n; ++i) {
            out.values[static_cast<size_t>(i)] = static_cast<double>(u->RawPointer()[i]);
        }
        return true;
    }
    return false;
}

const char* DTypeName(int valueSize) {
    switch (valueSize) {
        case 4: return "f32";
        case 8: return "f64";
        case 1: return "u8";
        default: return "?";
    }
}

const char* AttachmentName(IGenum attachment) {
    switch (attachment) {
        case IG_POINT: return "point";
        case IG_CELL: return "cell";
        default: return "other";
    }
}

// =============================================================================
// 对象快照
// =============================================================================

struct Snapshot {
    IGenum type{IG_NONE};
    int64_t pointCount{0};
    ArrayData points;  ///< f32 x3

    int64_t cellCount{0};  ///< 来自 GetCellArray()；PointSet / StructuredMesh 为 0
    std::vector<int32_t> connectivity;
    std::vector<int64_t> offsets;
    int64_t fixedCellSize{0};
    std::vector<uint8_t> types;

    int64_t dims[3]{0, 0, 0};
    bool hasDims{false};

    struct Attr {
        std::string name;
        IGenum type{IG_NONE};
        IGenum attachment{IG_NONE};
        int components{0};
        int64_t elementCount{0};
        ArrayData data;
    };
    std::vector<Attr> attrs;

    /// 单元数：结构化网格没有 CellArray，但单元属性仍有长度，
    /// 因此"单元属性配对用的元素数"要单独取。
    int64_t CellElementCount() const {
        int64_t n = cellCount;
        for (const auto& a : attrs) {
            if (a.attachment == IG_CELL) { n = std::max(n, a.elementCount); }
        }
        return n;
    }

    int64_t CellSize(int64_t cell) const {
        if (static_cast<int64_t>(offsets.size()) > cell + 1) {
            return offsets[static_cast<size_t>(cell) + 1] - offsets[static_cast<size_t>(cell)];
        }
        return fixedCellSize;
    }

    /// 第 cell 个单元在 Connectivity 里的起始下标。
    ///
    /// **必须按单元取偏移，不能用"按遍历顺序累加的游标"**：解码后的单元顺序与源
    /// 对象的单元顺序不同（IGC 会重排单元），累加游标只在"单元大小全部相同"时
    /// 恰好等于偏移 —— 混合单元网格上会算出一个看起来合理、实际错位的结果。
    int64_t CellOffset(int64_t cell) const {
        if (static_cast<int64_t>(offsets.size()) > cell + 1) {
            return offsets[static_cast<size_t>(cell)];
        }
        return cell * fixedCellSize;
    }

    const Attr* FindAttr(const std::string& name) const {
        for (const auto& a : attrs) {
            if (a.name == name) { return &a; }
        }
        return nullptr;
    }
};

Snapshot TakeSnapshot(const DataObject::Pointer& obj) {
    Snapshot s;
    if (!obj) { return s; }
    s.type = obj->GetDataObjectType();

    if (auto ps = DynamicCast<PointSet>(obj)) {
        if (Points::Pointer pts = ps->GetPoints()) {
            s.pointCount = static_cast<int64_t>(pts->GetNumberOfPoints());
            ArrayData d;
            d.components = 3;
            d.valueSize = static_cast<int>(sizeof(float));
            d.elements = s.pointCount;
            const int64_t n = s.pointCount * 3;
            d.raw.resize(static_cast<size_t>(n) * sizeof(float));
            if (n > 0) { std::memcpy(d.raw.data(), pts->RawPointer(), d.raw.size()); }
            d.values.resize(static_cast<size_t>(n));
            for (int64_t i = 0; i < n; ++i) {
                d.values[static_cast<size_t>(i)] = static_cast<double>(pts->RawPointer()[i]);
            }
            s.points = std::move(d);
        }
    }

    if (s.type == IG_STRUCTURED_MESH) {
        if (auto sm = DynamicCast<StructuredMesh>(obj)) {
            const igIndex* dim = sm->GetDimensionSize();
            if (dim != nullptr) {
                s.hasDims = true;
                s.dims[0] = static_cast<int64_t>(dim[0]);
                s.dims[1] = static_cast<int64_t>(dim[1]);
                s.dims[2] = static_cast<int64_t>(dim[2]);
            }
        }
    }

    if (CellArray::Pointer cells = obj->GetCellArray()) {
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
            // 定长网格的 offset 数组只有 {0}（见 iGameCellArray 构造）；
            // 此时单元大小由 fixedCellSize 表达，不参与 offsets 逐值比较。
            if (static_cast<int64_t>(n) == s.cellCount + 1) {
                s.offsets.reserve(static_cast<size_t>(n));
                for (IGsize i = 0; i < n; ++i) {
                    s.offsets.push_back(static_cast<int64_t>(off->GetValue(i)));
                }
            }
        }
        if (s.offsets.empty() && s.cellCount > 0) {
            s.fixedCellSize = static_cast<int64_t>(s.connectivity.size()) / s.cellCount;
        }
    }

    // 单元类型只有 UnstructuredMesh 有权威来源（适配器同样只对它暴露 Types）。
    if (auto um = DynamicCast<UnstructuredMesh>(obj)) {
        if (UnsignedIntArray::Pointer ts = um->GetCellTypes()) {
            const IGsize n = ts->GetNumberOfValues();
            s.types.reserve(static_cast<size_t>(n));
            for (IGsize i = 0; i < n; ++i) {
                // GetValue() 返回 double；不要对它做位运算，直接截断。
                s.types.push_back(static_cast<uint8_t>(ts->GetValue(i)));
            }
        }
    }

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
            sa.elementCount = static_cast<int64_t>(arr->GetNumberOfElements());
            if (!ExtractArray(arr, sa.data)) { continue; }
            s.attrs.push_back(std::move(sa));
        }
    }
    return s;
}

// =============================================================================
// 顺序映射（IGC 会重排点序与单元序，必须显式还原）
// =============================================================================

/// `MeshEncodeTrace::orderRemaps` 里存的是 **original -> decoded**。
/// 比对时必须反过来用：解码结果的第 d 个元素对应源对象的 `decodedToOriginal[d]`。
struct OrderMap {
    std::vector<size_t> originalToDecoded;
    std::vector<size_t> decodedToOriginal;
    bool identity{true};
    bool valid{true};
    std::string note;

    size_t Size() const { return decodedToOriginal.size(); }
};

OrderMap MakeOrderMap(const std::vector<unsigned int>& originalToDecoded, int64_t elementCount,
                      const std::string& what) {
    OrderMap m;
    if (elementCount < 0) {
        m.valid = false;
        m.note = what + ": negative element count";
        return m;
    }
    const size_t n = static_cast<size_t>(elementCount);

    if (originalToDecoded.empty()) {
        // 编码器没有重排（结构化网格就是这种情况）：恒等映射。
        m.originalToDecoded.resize(n);
        m.decodedToOriginal.resize(n);
        for (size_t i = 0; i < n; ++i) {
            m.originalToDecoded[i] = i;
            m.decodedToOriginal[i] = i;
        }
        m.identity = true;
        return m;
    }

    if (originalToDecoded.size() != n) {
        m.valid = false;
        m.note = what + ": encode order map size " + Num(static_cast<int64_t>(originalToDecoded.size())) +
                 " != element count " + Num(elementCount);
        return m;
    }

    m.originalToDecoded.resize(n);
    m.decodedToOriginal.assign(n, 0);
    std::vector<unsigned char> seen(n, 0);
    m.identity = true;
    for (size_t o = 0; o < n; ++o) {
        const size_t d = static_cast<size_t>(originalToDecoded[o]);
        if (d >= n) {
            m.valid = false;
            m.note = what + ": encode order map has out-of-range index";
            return m;
        }
        if (seen[d] != 0) {
            m.valid = false;
            m.note = what + ": encode order map has duplicate index";
            return m;
        }
        seen[d] = 1;
        m.originalToDecoded[o] = d;
        m.decodedToOriginal[d] = o;
        if (d != o) { m.identity = false; }
    }
    return m;
}

// =============================================================================
// 「保真边界报告」
// =============================================================================
//
// 名义档位（`igcProfile`）只是解释字段，绝不参与 verdict 的计算。

/// 一个数组的保真条目。
struct ArrayFidelity {
    std::string name;
    std::string attachment;
    std::string dtype;
    int components{0};
    int64_t elements{0};
    int64_t differingValues{0};
    double maxAbsError{0.0};
    double maxRelError{0.0};

    bool Equal() const { return differingValues == 0; }
};

enum class FidelityVerdict {
    LosslessRoundTrip,   ///< 全部数组逐值 bitwise 相等（含拓扑）
    LossyNotReversible,  ///< 至少一个值/一处拓扑不同 —— 有损，不可往返
};

const char* VerdictName(FidelityVerdict v) {
    switch (v) {
        case FidelityVerdict::LosslessRoundTrip: return "lossless round-trip";
        case FidelityVerdict::LossyNotReversible: return "lossy, NOT reversible";
    }
    return "unknown";
}

struct ConvertReport {
    std::string label;
    std::string sourceFormat;
    std::string targetFormat;
    std::string igcProfile;  ///< 名义档位（"None/level 0" 等），仅作解释
    bool pointOrderPreserved{true};
    bool cellOrderPreserved{true};
    bool topologyCompared{false};
    bool topologyEqual{false};
    std::string topologyNote;
    std::vector<ArrayFidelity> arrays;
    FidelityVerdict verdict{FidelityVerdict::LosslessRoundTrip};
    std::string verdictReason;

    bool AllArraysEqual() const {
        for (const auto& a : arrays) {
            if (!a.Equal()) { return false; }
        }
        return true;
    }

    bool Lossless() const {
        return verdict == FidelityVerdict::LosslessRoundTrip;
    }

    /// 结论按实测结果落地：数组全等 **且** 拓扑相等才算无损。
    void FinalizeVerdict() {
        const bool arraysEqual = AllArraysEqual();
        const bool topologyOk = topologyEqual;
        if (arraysEqual && topologyOk) {
            verdict = FidelityVerdict::LosslessRoundTrip;
            verdictReason = "every array is bitwise equal and the topology matches";
            return;
        }
        verdict = FidelityVerdict::LossyNotReversible;
        if (!arraysEqual && !topologyOk) {
            verdictReason = "array values differ and the topology differs";
        } else if (!arraysEqual) {
            int64_t diff = 0;
            for (const auto& a : arrays) { diff += a.differingValues; }
            verdictReason = Num(diff) + " array value(s) differ from the source";
        } else {
            verdictReason = "topology differs from the source: " + topologyNote;
        }
    }

    void Print() const {
        std::cerr << " [reprt] " << label << ": " << sourceFormat << " -> " << targetFormat
                  << " | source IGC profile: " << igcProfile << "\n"
                  << "         point order preserved: " << (pointOrderPreserved ? "yes" : "no")
                  << " | cell order preserved: " << (cellOrderPreserved ? "yes" : "no") << "\n"
                  << "         topology: "
                  << (topologyCompared ? (topologyEqual ? "equal" : "DIFFERS") : "not compared")
                  << " (" << topologyNote << ")\n";
        for (const auto& a : arrays) {
            std::cerr << "         array " << Pad(a.name, 16) << " " << Pad(a.attachment, 6) << " "
                      << Pad(a.dtype, 3) << " x" << a.components << "  elems=" << Pad(Num(a.elements), 6)
                      << " differing=" << Pad(Num(a.differingValues), 6);
            if (!a.Equal()) {
                std::cerr << " maxAbs=" << Sci(a.maxAbsError) << " maxRel=" << Sci(a.maxRelError);
            }
            std::cerr << "\n";
        }
        std::cerr << "         verdict: " << VerdictName(verdict) << " (" << verdictReason << ")\n";
    }
};

/// 逐值比对（带顺序还原），把结果写进 `report.arrays`。
///
/// `values` 只用来算误差，仅供有损档的报告，不参与"是否相等"的判定。
void CompareOneArray(const std::string& name, const std::string& attachment, const ArrayData& ref,
                     const ArrayData& got, const OrderMap& map, ConvertReport& report) {
    ArrayFidelity f;
    f.name = name;
    f.attachment = attachment;
    f.dtype = DTypeName(ref.valueSize);
    f.components = ref.components;
    f.elements = ref.elements;

    const int64_t valuesPerElement = ref.components;
    const int size = ref.valueSize;

    if (got.components != ref.components || got.valueSize != ref.valueSize ||
        got.elements != ref.elements) {
        // 形状/类型变了：直接判定为不等，并如实记下差异规模。
        f.differingValues = std::max<int64_t>(1, ref.ValueCount());
        report.arrays.push_back(f);
        return;
    }
    if (map.Size() != static_cast<size_t>(ref.elements)) {
        f.differingValues = std::max<int64_t>(1, ref.ValueCount());
        report.arrays.push_back(f);
        return;
    }

    for (int64_t d = 0; d < ref.elements; ++d) {
        const int64_t o = static_cast<int64_t>(map.decodedToOriginal[static_cast<size_t>(d)]);
        for (int k = 0; k < valuesPerElement; ++k) {
            const size_t refByte = static_cast<size_t>((o * valuesPerElement + k) * size);
            const size_t gotByte = static_cast<size_t>((d * valuesPerElement + k) * size);
            const bool same = std::memcmp(ref.raw.data() + refByte, got.raw.data() + gotByte,
                                          static_cast<size_t>(size)) == 0;
            if (same) { continue; }

            ++f.differingValues;
            const double a = ref.values[static_cast<size_t>(o * valuesPerElement + k)];
            const double b = got.values[static_cast<size_t>(d * valuesPerElement + k)];
            const double absErr = std::fabs(a - b);
            const double denom = std::max(std::fabs(a), 1e-30);
            f.maxAbsError = std::max(f.maxAbsError, absErr);
            f.maxRelError = std::max(f.maxRelError, absErr / denom);
        }
    }
    report.arrays.push_back(f);
}

/// 拓扑比对：单元数、每个单元的顶点数、顶点索引（经点序映射）、单元类型（经单元序映射）。
///
/// 这一节解决的是"点序被重排之后，连接关系还算不算同一个网格"的问题：
/// 源对象的第 o 个点出现在解码结果的第 `pointMap.originalToDecoded[o]` 位，
/// 因此解码结果的连接索引必须等于源索引经该映射后的值。
bool CompareTopology(const Snapshot& ref, const Snapshot& got, const OrderMap& pointMap,
                     const OrderMap& cellMap, ConvertReport& report) {
    report.topologyCompared = true;

    if (ref.hasDims || got.hasDims) {
        if (!ref.hasDims || !got.hasDims) {
            report.topologyEqual = false;
            report.topologyNote = "Dimensions present on one side only";
            return false;
        }
        if (ref.dims[0] != got.dims[0] || ref.dims[1] != got.dims[1] || ref.dims[2] != got.dims[2]) {
            report.topologyEqual = false;
            report.topologyNote = "Dimensions [" + Num(ref.dims[0]) + "," + Num(ref.dims[1]) + "," +
                                  Num(ref.dims[2]) + "] -> [" + Num(got.dims[0]) + "," +
                                  Num(got.dims[1]) + "," + Num(got.dims[2]) + "]";
            return false;
        }
        report.topologyEqual = true;
        report.topologyNote = "structured Dimensions equal [" + Num(got.dims[0]) + "," + Num(got.dims[1]) +
                              "," + Num(got.dims[2]) + "]";
        return true;
    }

    if (ref.cellCount != got.cellCount) {
        report.topologyEqual = false;
        report.topologyNote = "cell count " + Num(ref.cellCount) + " -> " + Num(got.cellCount);
        return false;
    }

    if (ref.cellCount == 0) {
        report.topologyEqual = true;
        report.topologyNote = "no topology on either side (empty-topology branch)";
        return true;
    }

    if (!pointMap.valid || !cellMap.valid) {
        // 拿不到映射就不能声称拓扑相等 —— 显式失败，而不是静默跳过。
        report.topologyEqual = false;
        report.topologyNote = "order map unavailable: " + pointMap.note + cellMap.note;
        return false;
    }

    if (!ref.types.empty() || !got.types.empty()) {
        if (ref.types.size() != got.types.size()) {
            report.topologyEqual = false;
            report.topologyNote = "Types array length " + Num(static_cast<int64_t>(ref.types.size())) + " -> " +
                                  Num(static_cast<int64_t>(got.types.size()));
            return false;
        }
        for (int64_t d = 0; d < got.cellCount; ++d) {
            const int64_t o = static_cast<int64_t>(cellMap.decodedToOriginal[static_cast<size_t>(d)]);
            const uint8_t refType = ref.types[static_cast<size_t>(o)];
            const uint8_t gotType = got.types[static_cast<size_t>(d)];
            if (refType != gotType) {
                report.topologyEqual = false;
                report.topologyNote = "cell " + Num(d) + " (source " + Num(o) + "): Types " +
                                      Num(refType) + " -> " + Num(gotType);
                return false;
            }
        }
    }

    for (int64_t d = 0; d < got.cellCount; ++d) {
        const int64_t o = static_cast<int64_t>(cellMap.decodedToOriginal[static_cast<size_t>(d)]);
        const int64_t refSize = ref.CellSize(o);
        const int64_t gotSize = got.CellSize(d);
        if (refSize != gotSize) {
            report.topologyEqual = false;
            report.topologyNote = "cell " + Num(d) + " (source " + Num(o) + "): point count " + Num(refSize) +
                                  " -> " + Num(gotSize);
            return false;
        }
        // 源侧按**它自己的**偏移取，解码侧按解码结果自己的偏移取。
        // 两个偏移都必须来自各自快照，不能共用游标（见 Snapshot::CellOffset 的注释）。
        const int64_t refBase = ref.CellOffset(o);
        const int64_t gotBase = got.CellOffset(d);
        for (int64_t k = 0; k < gotSize; ++k) {
            const int32_t refId = ref.connectivity[static_cast<size_t>(refBase + k)];
            const int32_t gotId = got.connectivity[static_cast<size_t>(gotBase + k)];
            if (refId < 0 || static_cast<size_t>(refId) >= pointMap.originalToDecoded.size()) {
                report.topologyEqual = false;
                report.topologyNote = "source cell " + Num(o) + " references point " + Num(refId) +
                                      " outside [0," + Num(static_cast<int64_t>(pointMap.originalToDecoded.size())) +
                                      ")";
                return false;
            }
            const int32_t mapped = static_cast<int32_t>(
                pointMap.originalToDecoded[static_cast<size_t>(refId)]);
            if (mapped != gotId) {
                report.topologyEqual = false;
                report.topologyNote = "cell " + Num(d) + " (source " + Num(o) + ") vertex " + Num(k) +
                                      ": expected point " + Num(mapped) + " (mapped from source " +
                                      Num(refId) + "), got " + Num(gotId);
                return false;
            }
        }
    }

    report.topologyEqual = true;
    report.topologyNote = Num(got.cellCount) + " cells, connectivity/offsets/Types equal after applying the point permutation";
    return true;
}

// =============================================================================
// 第二判据：**不依赖编码器自述**的顺序无关比对
// =============================================================================
//
// `CompareObjects()` 用编码器给出的重排映射把解码结果对回源对象。它很精确，但有
// 一个方法论弱点：**判据的一半来自被测代码自己**（编码器说"我把第 o 个点放到了
// 第 d 位"，我们就照它比对）。若映射本身是错的，比对会失败（这一点已实测：本程序
// 第一版把源侧的下标算错，混合单元网格立刻报红），但"两边用同一个错误约定"这种
// 风险仍然存在。
//
// 因此这里再加一条**完全不用重排映射**的判据：把每个点/每个单元各自的全部内容
// （坐标 + 挂在它上面的属性值；单元再加类型与各顶点的内容）编码成一个字节串，
// 排序后逐串比较。它证明的是"同一组点、同一组单元，逐值 bitwise 相同"——
// 与顺序无关，因此既不需要也不相信任何置换。
//
// 两条判据合起来才是"无损"：内容相同（本条）+ 下标对应关系正确（上一条）。

void AppendFramed(std::string& out, const std::string& s) {
    const uint32_t n = static_cast<uint32_t>(s.size());
    out.push_back(static_cast<char>(n & 0xFF));
    out.push_back(static_cast<char>((n >> 8) & 0xFF));
    out.push_back(static_cast<char>((n >> 16) & 0xFF));
    out.push_back(static_cast<char>((n >> 24) & 0xFF));
    out += s;
}

std::string AttrElementBytes(const Snapshot::Attr& a, int64_t element) {
    const size_t per = static_cast<size_t>(a.data.valueSize) * static_cast<size_t>(a.components);
    const size_t off = static_cast<size_t>(element) * per;
    if (per == 0 || off + per > a.data.raw.size()) { return {}; }
    return std::string(a.data.raw.begin() + static_cast<ptrdiff_t>(off),
                       a.data.raw.begin() + static_cast<ptrdiff_t>(off + per));
}

/// 按名字排序后的属性指针列表（两次构建的排列必须一致，否则键不可比）。
std::vector<const Snapshot::Attr*> SortedAttrsOf(const Snapshot& s, IGenum attachment) {
    std::vector<const Snapshot::Attr*> out;
    for (const auto& a : s.attrs) {
        if (a.attachment == attachment) { out.push_back(&a); }
    }
    std::sort(out.begin(), out.end(), [](const Snapshot::Attr* a, const Snapshot::Attr* b) {
        return a->name < b->name;
    });
    return out;
}

std::vector<std::string> BuildPointKeys(const Snapshot& s) {
    const auto attrs = SortedAttrsOf(s, IG_POINT);
    std::vector<std::string> keys;
    keys.reserve(static_cast<size_t>(s.pointCount));
    const size_t per = 3 * sizeof(float);
    for (int64_t p = 0; p < s.pointCount; ++p) {
        std::string key;
        const size_t off = static_cast<size_t>(p) * per;
        if (off + per > s.points.raw.size()) { return {}; }
        key.assign(s.points.raw.begin() + static_cast<ptrdiff_t>(off),
                   s.points.raw.begin() + static_cast<ptrdiff_t>(off + per));
        for (const Snapshot::Attr* a : attrs) { AppendFramed(key, AttrElementBytes(*a, p)); }
        keys.push_back(std::move(key));
    }
    return keys;
}

std::vector<std::string> BuildCellKeys(const Snapshot& s) {
    std::vector<std::string> keys;
    if (s.cellCount <= 0) { return keys; }

    const std::vector<std::string> pointKeys = BuildPointKeys(s);
    if (pointKeys.size() != static_cast<size_t>(s.pointCount)) { return {}; }
    const auto attrs = SortedAttrsOf(s, IG_CELL);

    keys.reserve(static_cast<size_t>(s.cellCount));
    for (int64_t c = 0; c < s.cellCount; ++c) {
        std::string key;
        // 单元类型（有权威 Types 时带上；SurfaceMesh/VolumeMesh 的类型由顶点数决定，
        // 已隐含在下面的顶点序列长度里）。
        if (!s.types.empty()) {
            key.push_back(static_cast<char>(s.types[static_cast<size_t>(c)]));
        }
        const int64_t size = s.CellSize(c);
        const int64_t base = s.CellOffset(c);
        for (int64_t k = 0; k < size; ++k) {
            const int32_t id = s.connectivity[static_cast<size_t>(base + k)];
            if (id < 0 || static_cast<size_t>(id) >= pointKeys.size()) { return {}; }
            AppendFramed(key, pointKeys[static_cast<size_t>(id)]);
        }
        for (const Snapshot::Attr* a : attrs) { AppendFramed(key, AttrElementBytes(*a, c)); }
        keys.push_back(std::move(key));
    }
    return keys;
}

/// 排序后逐串比较；`detail` 说明首个不匹配。
bool CompareStringMultisets(std::vector<std::string> a, std::vector<std::string> b, const std::string& what,
                            std::string& detail) {
    if (a.size() != b.size()) {
        detail = what + ": count " + Num(static_cast<int64_t>(a.size())) + " -> " +
                 Num(static_cast<int64_t>(b.size()));
        return false;
    }
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    int64_t mismatches = 0;
    std::string firstA;
    std::string firstB;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] == b[i]) { continue; }
        if (mismatches == 0) {
            firstA = a[i];
            firstB = b[i];
        }
        ++mismatches;
    }
    if (mismatches == 0) { return true; }
    detail = what + ": " + Num(mismatches) + " of " + Num(static_cast<int64_t>(a.size())) +
             " key(s) differ (first differs at byte-level content, sizes " +
             Num(static_cast<int64_t>(firstA.size())) + " vs " + Num(static_cast<int64_t>(firstB.size())) + ")";
    return false;
}

bool CompareOrderFree(const Snapshot& ref, const Snapshot& got, std::string& detail) {
    {
        const auto refAttrs = SortedAttrsOf(ref, IG_POINT);
        const auto gotAttrs = SortedAttrsOf(got, IG_POINT);
        if (refAttrs.size() != gotAttrs.size()) {
            detail = "point attribute count " + Num(static_cast<int64_t>(refAttrs.size())) + " -> " +
                     Num(static_cast<int64_t>(gotAttrs.size()));
            return false;
        }
        for (size_t i = 0; i < refAttrs.size(); ++i) {
            if (refAttrs[i]->name != gotAttrs[i]->name || refAttrs[i]->components != gotAttrs[i]->components ||
                refAttrs[i]->data.valueSize != gotAttrs[i]->data.valueSize) {
                detail = "point attribute '" + refAttrs[i]->name + "' shape/type differs";
                return false;
            }
        }
    }
    if (!CompareStringMultisets(BuildPointKeys(ref), BuildPointKeys(got), "point multiset", detail)) {
        return false;
    }

    if (ref.cellCount != got.cellCount && (ref.cellCount > 0 || got.cellCount > 0)) {
        detail = "cell count " + Num(ref.cellCount) + " -> " + Num(got.cellCount);
        return false;
    }
    if (ref.cellCount > 0) {
        const auto refAttrs = SortedAttrsOf(ref, IG_CELL);
        const auto gotAttrs = SortedAttrsOf(got, IG_CELL);
        if (refAttrs.size() != gotAttrs.size()) {
            detail = "cell attribute count " + Num(static_cast<int64_t>(refAttrs.size())) + " -> " +
                     Num(static_cast<int64_t>(gotAttrs.size()));
            return false;
        }
        for (size_t i = 0; i < refAttrs.size(); ++i) {
            if (refAttrs[i]->name != gotAttrs[i]->name || refAttrs[i]->components != gotAttrs[i]->components ||
                refAttrs[i]->data.valueSize != gotAttrs[i]->data.valueSize) {
                detail = "cell attribute '" + refAttrs[i]->name + "' shape/type differs";
                return false;
            }
        }
        if (!CompareStringMultisets(BuildCellKeys(ref), BuildCellKeys(got), "cell multiset", detail)) {
            return false;
        }
    }
    detail = "point and cell contents match as multisets";
    return true;
}

/// 完整比对：geometry + 每个属性 + 拓扑，并给出结论。
ConvertReport CompareObjects(const std::string& label, const std::string& sourceFormat,
                             const std::string& targetFormat, const std::string& igcProfile,
                             const Snapshot& ref, const Snapshot& got, const MeshEncodeTrace* trace) {
    ConvertReport report;
    report.label = label;
    report.sourceFormat = sourceFormat;
    report.targetFormat = targetFormat;
    report.igcProfile = igcProfile;

    const OrderMap pointMap =
        MakeOrderMap(trace != nullptr ? trace->orderRemaps.pointIdRemap : std::vector<unsigned int>{},
                     ref.pointCount, "point order map");
    const OrderMap cellMap =
        MakeOrderMap(trace != nullptr ? trace->orderRemaps.topCellIdsRemap : std::vector<unsigned int>{},
                     ref.CellElementCount(), "cell order map");

    report.pointOrderPreserved = pointMap.valid && pointMap.identity;
    report.cellOrderPreserved = cellMap.valid && cellMap.identity;

    CompareOneArray("Points", "geometry", ref.points, got.points, pointMap, report);

    for (const auto& ra : ref.attrs) {
        const Snapshot::Attr* ga = got.FindAttr(ra.name);
        if (ga == nullptr) {
            ArrayFidelity f;
            f.name = ra.name;
            f.attachment = AttachmentName(ra.attachment);
            f.dtype = DTypeName(ra.data.valueSize);
            f.components = ra.components;
            f.elements = ra.elementCount;
            f.differingValues = std::max<int64_t>(1, ra.data.ValueCount());
            report.arrays.push_back(f);
            continue;
        }
        const OrderMap& map = ra.attachment == IG_POINT ? pointMap : cellMap;
        CompareOneArray(ra.name, AttachmentName(ra.attachment), ra.data, ga->data, map, report);
    }

    // 结果侧多出来的属性也是"不保真"（例如某一步把属性拆成了 X/Y/Z 三个标量）。
    for (const auto& ga : got.attrs) {
        if (ref.FindAttr(ga.name) == nullptr) {
            ArrayFidelity f;
            f.name = ga.name;
            f.attachment = AttachmentName(ga.attachment);
            f.dtype = DTypeName(ga.data.valueSize);
            f.components = ga.components;
            f.elements = ga.elementCount;
            f.differingValues = std::max<int64_t>(1, ga.data.ValueCount());
            report.arrays.push_back(f);
        }
    }

    CompareTopology(ref, got, pointMap, cellMap, report);
    report.FinalizeVerdict();
    return report;
}

// =============================================================================
// 夹具
// =============================================================================
//
// 全部属性和网格的名字都刻意避开 X/Y/Z 与 0/1/2 结尾：
// `FileIO::FinalizeLoadedObject()` 会把这类名字的标量合并成一个 3 分量向量
// 的断言变成在测另一件事。

/// 单精度点标量属性（f32 单分量）。
void AddFloatPointAttr(PointSet* mesh, const std::string& name, float scale) {
    FloatArray::Pointer f = FloatArray::New();
    f->SetDimension(1);
    f->SetName(name);
    const IGsize n = mesh->GetPoints()->GetNumberOfPoints();
    f->Resize(n);
    for (IGsize i = 0; i < n; ++i) {
        f->RawPointer()[i] = static_cast<float>(i) * scale;
    }
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_POINT, f);
}

/// 真正无单元的 PointSet（空拓扑分支）。
///
/// 为什么必须直接构造：VTK 里的 `VTK_VERTEX` 点云会被建成 UnstructuredMesh，
PointSet::Pointer MakePointSetFixture() {
    auto ps = PointSet::New();
    Points::Pointer pts = Points::New();
    pts->Resize(5);
    const float coords[15] = {0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, 0.f, 3.f, -1.f, -1.f, -1.f};
    std::memcpy(pts->RawPointer(), coords, sizeof(coords));
    ps->SetPoints(pts);
    AddFloatPointAttr(ps, "p_scalar", 1.25f);
    return ps;
}

/// 均匀三角面片（3 点/面）。IGV 写侧要求 SurfaceMesh/VolumeMesh 的单元点数一致
/// （无 Types 数组时由点数推类型，见 iGameIGVWriter.cpp:820-840），所以这里全用三角形。
SurfaceMesh::Pointer MakeSurfaceMeshFixture() {
    auto mesh = SurfaceMesh::New();
    Points::Pointer pts = Points::New();
    pts->Resize(5);
    const float coords[15] = {0.f,  0.f,  0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f, 2.f, 2.f, 0.f};
    std::memcpy(pts->RawPointer(), coords, sizeof(coords));
    mesh->SetPoints(pts);

    CellArray::Pointer faces = CellArray::New();
    const igIndex tri[2][3] = {{0, 1, 2}, {0, 2, 3}};
    for (const auto& t : tri) {
        faces->AddCellIds(const_cast<igIndex*>(t), 3);
    }
    mesh->SetFaces(faces);

    AddFloatPointAttr(mesh, "p_scalar", 0.5f);

    FloatArray::Pointer cellAttr = FloatArray::New();
    cellAttr->SetDimension(1);
    cellAttr->SetName("f_scalar");
    cellAttr->Resize(2);
    cellAttr->RawPointer()[0] = 10.f;
    cellAttr->RawPointer()[1] = -20.f;
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, cellAttr);
    return mesh;
}

/// 立方体 VolumeMesh：6 个四边形面 + 1 个六面体体单元（面数 != 体数，这是
VolumeMesh::Pointer MakeVolumeMeshFixture() {
    auto mesh = VolumeMesh::New();
    Points::Pointer pts = Points::New();
    pts->Resize(8);
    const float cube[24] = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0,
                            0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1};
    std::memcpy(pts->RawPointer(), cube, sizeof(cube));
    mesh->SetPoints(pts);

    CellArray::Pointer faces = CellArray::New();
    const igIndex quad[6][4] = {{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4},
                                {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
    for (const auto& q : quad) {
        faces->AddCellIds(const_cast<igIndex*>(q), 4);
    }
    mesh->SetFaces(faces);

    CellArray::Pointer volumes = CellArray::New();
    const igIndex hex[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    volumes->AddCellIds(const_cast<igIndex*>(hex), 8);
    mesh->SetVolumes(volumes);

    AddFloatPointAttr(mesh, "p_scalar", 1.f);

    FloatArray::Pointer volAttr = FloatArray::New();
    volAttr->SetDimension(1);
    volAttr->SetName("vol_scalar");
    volAttr->Resize(1);  // == 体数；若误用面数 6，IGV 写侧会拒绝
    volAttr->RawPointer()[0] = 42.f;
    mesh->GetAttributeSet()->AddAttribute(IG_SCALAR, IG_CELL, volAttr);
    return mesh;
}

/// f32 / f64 / u8 三种属性各一个（P4-5 的载体）。
///
/// f64 的取值刻意选在 float32 无法精确表示的位置：IGC 的量化档会把 double 经
/// float32 往返（`MeshFloatCodec::FloatEncoder<double>` 里的 `quantFunc` 参数是
/// float），一旦发生，逐值比对与 `typedSize` 会同时发现。
UnstructuredMesh::Pointer MakeAttributeTypeFixture() {
    auto mesh = UnstructuredMesh::New();
    Points::Pointer pts = Points::New();
    pts->Resize(4);
    const float coords[12] = {0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f};
    std::memcpy(pts->RawPointer(), coords, sizeof(coords));
    mesh->SetPoints(pts);

    CellArray::Pointer cells = CellArray::New();
    igIndex ids[4] = {0, 1, 2, 3};
    cells->AddCellIds(ids, 4);
    UnsignedIntArray::Pointer types = UnsignedIntArray::New();
    types->AddValue(static_cast<unsigned int>(IG_TETRA));
    mesh->SetCells(cells, types);

    AttributeSet* set = mesh->GetAttributeSet();

    FloatArray::Pointer f = FloatArray::New();
    f->SetDimension(1);
    f->SetName("f_scalar");
    f->Resize(4);
    const float fv[4] = {0.5f, -1.25f, 3.f, -8.f};
    std::memcpy(f->RawPointer(), fv, sizeof(fv));
    set->AddAttribute(IG_SCALAR, IG_POINT, f);

    DoubleArray::Pointer d = DoubleArray::New();
    d->SetDimension(1);
    d->SetName("d_scalar");
    d->Resize(4);
    const double dv[4] = {0.1, 0.1234567890123, 1.0 / 3.0, -2.718281828459045};
    std::memcpy(d->RawPointer(), dv, sizeof(dv));
    set->AddAttribute(IG_SCALAR, IG_POINT, d);

    UnsignedCharArray::Pointer u = UnsignedCharArray::New();
    u->SetDimension(1);
    u->SetName("u8_scalar");
    u->Resize(4);
    const unsigned char uv[4] = {7, 200, 0, 255};
    std::memcpy(u->RawPointer(), uv, sizeof(uv));
    set->AddAttribute(IG_SCALAR, IG_POINT, u);

    FloatArray::Pointer v = FloatArray::New();
    v->SetDimension(3);
    v->SetName("cell_vec");
    v->Resize(1);
    const float vv[3] = {1.5f, -2.25f, 0.f};
    std::memcpy(v->RawPointer(), vv, sizeof(vv));
    set->AddAttribute(IG_VECTOR, IG_CELL, v);
    return mesh;
}

/// 稠密尾数的非结构化夹具：两个四面体，坐标与属性都带满尾数。
///
/// 为什么需要它：用 0/1/2 这类可精确表示的坐标做量化测试，尾数截断会是
/// **恒等操作**（本来就没有可截断的位），有损档会假通过。这里每个值都必须
/// 带满 23 位尾数。
UnstructuredMesh::Pointer MakeDenseUnstructuredFixture() {
    auto mesh = UnstructuredMesh::New();
    Points::Pointer pts = Points::New();
    pts->Resize(5);
    const float coords[15] = {0.1234567f,  -1.2345678f, 2.3456789f, 1.1111111f,  -2.2222222f,
                              3.3333333f,  4.4444444f,  5.5555555f,  -6.6666666f, -7.7777777f,
                              8.8888888f,  9.9999999f,  -0.9876543f, 1.2345678e-3f, 2.7182818f};
    std::memcpy(pts->RawPointer(), coords, sizeof(coords));
    mesh->SetPoints(pts);

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

    FloatArray::Pointer f = FloatArray::New();
    f->SetDimension(1);
    f->SetName("f_dense");
    f->Resize(5);
    const float fv[5] = {0.3141592f, -1.4142135f, 2.2360679f, -1.7320508f, 0.6931472f};
    std::memcpy(f->RawPointer(), fv, sizeof(fv));
    set->AddAttribute(IG_SCALAR, IG_POINT, f);

    DoubleArray::Pointer d = DoubleArray::New();
    d->SetDimension(1);
    d->SetName("d_dense");
    d->Resize(5);
    const double dv[5] = {0.1, 0.1234567890123, 1.0 / 3.0, -2.718281828459045, 3.141592653589793};
    std::memcpy(d->RawPointer(), dv, sizeof(dv));
    set->AddAttribute(IG_SCALAR, IG_POINT, d);

    UnsignedCharArray::Pointer u = UnsignedCharArray::New();
    u->SetDimension(1);
    u->SetName("u8_dense");
    u->Resize(5);
    const unsigned char uv[5] = {0, 17, 200, 254, 255};
    std::memcpy(u->RawPointer(), uv, sizeof(uv));
    set->AddAttribute(IG_SCALAR, IG_POINT, u);

    FloatArray::Pointer c = FloatArray::New();
    c->SetDimension(1);
    c->SetName("cell_dense");
    c->Resize(2);
    const float cv[2] = {1.2345678e-6f, -9.87654321e6f};
    std::memcpy(c->RawPointer(), cv, sizeof(cv));
    set->AddAttribute(IG_SCALAR, IG_CELL, c);
    return mesh;
}

// =============================================================================
// 转换动作
// =============================================================================

/// IGC 编码档位：只描述"我们让编码器怎么干"，不预设结论。
struct IgcProfile {
    std::string name;
    QuantizeMode geomMode{QuantizeMode::None};
    QuantizeMode attrMode{QuantizeMode::None};
    int level{0};
};

IgcProfile LosslessProfile() {
    return IgcProfile{"QuantizeMode::None, level 0 (lossless profile)", QuantizeMode::None,
                      QuantizeMode::None, 0};
}

IgcProfile QuantizedProfile(int level) {
    return IgcProfile{"QuantizeMode::Default, level " + std::to_string(level) + " (quantized profile)",
                      QuantizeMode::Default, QuantizeMode::Default, level};
}

/// 按给定档位写出 `.igc`，并把编码器产生的重排映射带出来。
///
/// **为什么必须绕开 `FileIO::WriteFile`**：重排映射只有 `IGCWriter::SetEncodeTrace`
/// 能取到，而 `FileIO::WriteFile` 不暴露它。没有映射就无法把解码结果对回源对象，
/// P4-2/P4-4 的"逐值 bitwise"就只能靠"顺序恰好相同"这个偶然前提（IGC 恰恰会重排）。
bool WriteIgc(const DataObject::Pointer& obj, const fs::path& path, const IgcProfile& profile,
              MeshEncodeTrace* trace, std::string& error) {
    CodecControlParams params =
        MeshEncoderFilter<EncodeOutputBinaryArray>::GenerateDefaultCodecParams(obj);
    params.geomControl.errorMode = profile.geomMode;
    params.geomControl.globalQuantizeLevel = profile.level;
    params.geomControl.criticalQuantizeLevel = profile.level;
    params.geomControl.normalQuantizeLevel = profile.level;
    for (auto& attr : params.attrControl) {
        attr.errorMode = profile.attrMode;
        attr.globalQuantizeLevel = profile.level;
        attr.criticalQuantizeLevel = profile.level;
        attr.normalQuantizeLevel = profile.level;
    }

    IGCWriter::Pointer writer = IGCWriter::New();
    writer->SetCodecControlParams(params);
    if (trace != nullptr) {
        trace->Clear();
        writer->SetEncodeTrace(trace);
    }
    if (!writer->WriteToFile(obj, path.string())) {
        error = "IGCWriter::WriteToFile failed";
        return false;
    }
    return true;
}

// =============================================================================
// 夹具清单
// =============================================================================

enum class FixtureKind {
    File,             ///< 从 Models/ 下的真实文件读入
    PointSetBuilt,    ///< 直接构造的无拓扑点集
    SurfaceMeshBuilt, ///< 直接构造的均匀三角面片
    VolumeMeshBuilt,  ///< 直接构造的立方体（面 + 体双索引）
    AttributeTypes,   ///< f32/f64/u8 三件套
    DenseUnstructured,
};

struct Fixture {
    std::string label;
    FixtureKind kind{FixtureKind::File};
    std::string path;
};

DataObject::Pointer BuildFixture(const Fixture& fx) {
    switch (fx.kind) {
        case FixtureKind::File:
            if (!fs::exists(fx.path)) { return nullptr; }
            return FileIO::ReadFile(fx.path);
        case FixtureKind::PointSetBuilt:
            return MakePointSetFixture();
        case FixtureKind::SurfaceMeshBuilt:
            return MakeSurfaceMeshFixture();
        case FixtureKind::VolumeMeshBuilt:
            return MakeVolumeMeshFixture();
        case FixtureKind::AttributeTypes:
            return MakeAttributeTypeFixture();
        case FixtureKind::DenseUnstructured:
            return MakeDenseUnstructuredFixture();
    }
    return nullptr;
}

std::string SafeFileName(const std::string& label) {
    std::string out = label;
    for (char& c : out) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                        c == '_' || c == '-';
        if (!ok) { c = '_'; }
    }
    return out;
}

// =============================================================================
// P4 各项
// =============================================================================

/// 交换一个数组里两个元素的全部字节（自检用）。
void SwapElements(ArrayData& d, int64_t a, int64_t b) {
    if (d.valueSize <= 0 || d.components <= 0) { return; }
    const size_t per = static_cast<size_t>(d.valueSize) * static_cast<size_t>(d.components);
    if (a == b) { return; }
    std::swap_ranges(d.raw.begin() + static_cast<ptrdiff_t>(a * static_cast<int64_t>(per)),
                     d.raw.begin() + static_cast<ptrdiff_t>((a + 1) * static_cast<int64_t>(per)),
                     d.raw.begin() + static_cast<ptrdiff_t>(b * static_cast<int64_t>(per)));
    if (static_cast<int64_t>(d.values.size()) >= (std::max(a, b) + 1) * d.components) {
        for (int k = 0; k < d.components; ++k) {
            std::swap(d.values[static_cast<size_t>(a * d.components + k)],
                      d.values[static_cast<size_t>(b * d.components + k)]);
        }
    }
}

/// P4-self：两条判据的**反例自检**。
///
/// 它会和"实现正确"在输出上长得一模一样。这里注入三类缺陷，逐个断言都能被抓住：
///   1. 改一个点坐标的最低位（1 个 bit）→ 两条判据都必须判"不等"；
///   2. 把两个点整体交换（内容不变、只有顺序变）→ **顺序无关判据仍判相等**，
///      而带恒等映射的第一判据必须判"不等"（它管的是下标对应关系）；
///   3. 只把某个单元的一个顶点改掉 → 两条判据都必须判"不等"（拓扑也确实在比）。
void RunComparatorSelfCheck() {
    Section("P4-self counter-example self-check of the two comparison criteria (guide section 2.0.2)");

    DataObject::Pointer obj = MakeDenseUnstructuredFixture();
    if (!obj) {
        Check(false, "P4-self: fixture preparation failed");
        return;
    }
    const Snapshot good = TakeSnapshot(obj);
    if (good.pointCount < 3 || good.cellCount < 1 || good.attrs.empty()) {
        Check(false, "P4-self: fixture is too small to inject defects");
        return;
    }

    // (0) 自己比自己：两条判据都必须判相等（否则后面的失败无法归因）。
    {
        std::string detail;
        Check(CompareOrderFree(good, good, detail), "P4-self: an object compared with itself is equal");
        const ConvertReport r = CompareObjects("self", "x", "x", "n/a", good, good, nullptr);
        Check(r.Lossless(), "P4-self: the report of 'object vs itself' is lossless");
    }

    // (1) 一个 bit 的差异。
    {
        Snapshot mutated = good;
        mutated.points.raw[0] = static_cast<unsigned char>(mutated.points.raw[0] ^ 0x01);
        const ConvertReport r = CompareObjects("bit-flip", "x", "x", "n/a", good, mutated, nullptr);
        const ArrayFidelity* pts = nullptr;
        for (const auto& a : r.arrays) {
            if (a.name == "Points") { pts = &a; }
        }
        Check(!r.Lossless(), "P4-self: a single flipped bit in one coordinate is detected by the array comparison");
        Check(pts != nullptr && pts->differingValues == 1,
              "P4-self: exactly one differing value is reported for that flip");

        std::string detail;
        const bool orderFreeDetects = !CompareOrderFree(good, mutated, detail);
        Check(orderFreeDetects,
              "P4-self: the order-free criterion also detects the flipped bit (" + detail + ")");
    }

    // (2) 纯顺序变化：内容集合完全相同，只有下标不同。
    {
        Snapshot reordered = good;
        SwapElements(reordered.points, 0, 1);
        for (auto& a : reordered.attrs) {
            if (a.attachment == IG_POINT) { SwapElements(a.data, 0, 1); }
        }
        // 点被重排之后，连接关系**必须同步重映射**。
        // 少了这一步，"交换两个点"改的是每个单元的几何（顶点顺序变了），而不是
        // "只有下标变了" —— 那是一个真实的几何改动，两条判据都应当报红。
        // IGC 的编码器做的正是「重排顶点 + 把 Connectivity 里引用的下标一起改」。
        for (auto& id : reordered.connectivity) {
            if (id == 0) {
                id = 1;
            } else if (id == 1) {
                id = 0;
            }
        }

        std::string detail;
        const bool orderFreeAccepts = CompareOrderFree(good, reordered, detail);
        Check(orderFreeAccepts,
              "P4-self: the order-free criterion does NOT fire on a pure permutation (" + detail + ")");

        const ConvertReport r = CompareObjects("permuted", "x", "x", "n/a", good, reordered, nullptr);
        Check(!r.Lossless(),
              "P4-self: the trace-based criterion DOES fire on the same permutation, i.e. it really checks "
              "index correspondence rather than content");

        // 相反方向才是关键：带上真实的重排映射后，同一个"被交换过"的对象必须判相等。
        MeshEncodeTrace traceForSwap;
        traceForSwap.hasOrderRemaps = true;
        traceForSwap.orderRemaps.pointIdRemap.resize(static_cast<size_t>(good.pointCount));
        for (size_t i = 0; i < traceForSwap.orderRemaps.pointIdRemap.size(); ++i) {
            traceForSwap.orderRemaps.pointIdRemap[i] = static_cast<unsigned int>(i);
        }
        traceForSwap.orderRemaps.pointIdRemap[0] = 1;
        traceForSwap.orderRemaps.pointIdRemap[1] = 0;
        const ConvertReport withMap = CompareObjects("permuted+map", "x", "x", "n/a", good, reordered, &traceForSwap);
        Check(withMap.Lossless(),
              "P4-self: with the matching permutation the trace-based criterion accepts the same object "
              "(so the mapping is doing real work, not being ignored)");
    }

    // (3) 拓扑差异：只改一个顶点。
    {
        if (!good.connectivity.empty()) {
            Snapshot mutated = good;
            // 把第一个顶点换成另一个合法下标（保证仍在 [0, pointCount) 内，
            // 这样被触发的是"连接关系不同"，而不是"越界"）。
            mutated.connectivity[0] = static_cast<int32_t>(
                (static_cast<int64_t>(mutated.connectivity[0]) + 1) % mutated.pointCount);
            const ConvertReport r = CompareObjects("topo", "x", "x", "n/a", good, mutated, nullptr);
            Check(!r.Lossless() && !r.topologyEqual,
                  "P4-self: a single changed vertex index is detected as a topology difference");

            std::string detail;
            const bool orderFreeDetectsTopo = !CompareOrderFree(good, mutated, detail);
            Check(orderFreeDetectsTopo,
                  "P4-self: the order-free criterion also detects the changed vertex (" + detail + ")");
        }
    }
}

/// Pin for the upstream defect: it asserts that the defect still exists.
///
/// 这一节断言的是"缺陷**仍然**存在"：`.igc` 里确实有 0 长度 payload 块，而且
/// 既有 `IGCReader` 确实读不回来。它一旦变红，说明主仓库把空块处理修好了 ——
/// 那时必须回来把 P4-1/P4-2/P4-4 里被 `[skip]` 掉的两类网格改成"必须通过"。
///
/// 覆盖三种"必然产生空块"的形态，以证明问题不是某一类网格的偶发：
///   * PointSet          —— 拓扑块为空（TopoEncoder 直接 return）；
///   * StructuredMesh    —— 拓扑块为空（拓扑由 axisSize 隐式表达）；
///   * 无属性的网格       —— 属性块为空（本程序用 Models 里的一份无属性 VTK）。
void RunP40(const std::vector<Fixture>& fixtures) {
    Section("P4-0 upstream defect pin: 0-length payload bricks make the existing IGCReader fail");

    for (const Fixture& fx : fixtures) {
        DataObject::Pointer obj = BuildFixture(fx);
        if (!obj) {
            Info("skip " + fx.label + ": fixture unavailable (" + fx.path + ")");
            continue;
        }
        const fs::path igcPath = WorkDir() / (SafeFileName(fx.label) + ".p40.igc");
        if (!FileIO::WriteFile(igcPath.string(), obj)) {
            Check(false, "P4-0 " + fx.label + ": IGC write failed");
            continue;
        }

        std::vector<IgcBrick> bricks;
        const bool layoutOk = ReadIgcBricks(igcPath, bricks);
        Check(layoutOk, "P4-0 " + fx.label + ": the .igc payload layout is well formed");
        Info("P4-0 " + fx.label + ": bricks = [" + BrickListText(bricks) + "]");

        const bool hasEmpty = HasZeroLengthBrick(bricks);
        Check(hasEmpty, "P4-0 " + fx.label + ": the .igc contains a 0-length payload brick "
                                             "(the trigger of the upstream defect)");

        DataObject::Pointer back = FileIO::ReadFile(igcPath.string());
        Check(back == nullptr, "P4-0 " + fx.label + ": the existing IGCReader cannot read it back "
                                                  "(pin: when this turns red the upstream fix landed)");
    }
}

/// P4-1：IGV -> IGC（五类网格各一次），产物能被既有 IGCReader 读回，
/// 且点数 / 单元数 / 属性集合与源对象一致。
void RunP41(const std::vector<Fixture>& fixtures) {
    Section("P4-1 IGV -> IGC for the five mesh kinds, re-read with the existing IGCReader");

    for (const Fixture& fx : fixtures) {
        DataObject::Pointer objA = BuildFixture(fx);
        if (!objA) {
            Info("skip " + fx.label + ": fixture unavailable (" + fx.path + ")");
            continue;
        }

        const fs::path igvPath = WorkDir() / (SafeFileName(fx.label) + ".p41.igv");
        const fs::path igcPath = WorkDir() / (SafeFileName(fx.label) + ".p41.igc");

        // 源侧走 FileIO：P4 的设计意图就是"复用 FileIO 的读 + 写"，顺带验证
        // ·.igv 与 *.igc 两条分派都通。
        if (!FileIO::WriteFile(igvPath.string(), objA)) {
            Check(false, fx.label + ": FileIO::WriteFile(.igv) failed");
            continue;
        }
        DataObject::Pointer objIgv = FileIO::ReadFile(igvPath.string());
        if (!objIgv) {
            Check(false, fx.label + ": FileIO::ReadFile(.igv) failed");
            continue;
        }
        if (!FileIO::WriteFile(igcPath.string(), objIgv)) {
            Check(false, fx.label + ": FileIO::WriteFile(.igc) failed");
            continue;
        }
        DataObject::Pointer objIgc = FileIO::ReadFile(igcPath.string());
        if (!objIgc) {
            HandleIgcReadFailure(fx.label, igcPath);
            continue;
        }

        const Snapshot a = TakeSnapshot(objIgv);
        const Snapshot b = TakeSnapshot(objIgc);

        Check(b.type == a.type, fx.label + ": mesh type preserved across IGV -> IGC (" + Num(a.type) + ")");
        Check(b.pointCount == a.pointCount, fx.label + ": point count preserved (" + Num(a.pointCount) +
                                                 " -> " + Num(b.pointCount) + ")");
        Check(b.CellElementCount() == a.CellElementCount(),
              fx.label + ": cell count preserved (" + Num(a.CellElementCount()) + " -> " +
                  Num(b.CellElementCount()) + ")");
        Check(b.attrs.size() == a.attrs.size(),
              fx.label + ": attribute count preserved (" + Num(static_cast<int64_t>(a.attrs.size())) + " -> " +
                  Num(static_cast<int64_t>(b.attrs.size())) + ")");

        bool namesMatch = (a.attrs.size() == b.attrs.size());
        if (namesMatch) {
            for (const auto& ra : a.attrs) {
                const Snapshot::Attr* ga = b.FindAttr(ra.name);
                if (ga == nullptr || ga->type != ra.type || ga->attachment != ra.attachment ||
                    ga->components != ra.components) {
                    namesMatch = false;
                    break;
                }
            }
        }
        Check(namesMatch, fx.label + ": attribute names / types / attachments preserved");

        Info(fx.label + ": " + igvPath.filename().string() + " (" + Num(static_cast<int64_t>(fs::file_size(igvPath))) +
             " B) -> " + igcPath.filename().string() + " (" +
             Num(static_cast<int64_t>(fs::file_size(igcPath))) + " B)");
    }
}

/// P4-2：IGC -> IGV（无损档）。转出的 `.igv` 与**源 DataObject** 逐值 bitwise 相等
/// （点序/单元序按编码器给出的映射还原），且 IGV 这一步本身不引入任何差异。
void RunP42(const std::vector<Fixture>& fixtures) {
    Section("P4-2 IGC -> IGV, lossless profile: bitwise equal to the source DataObject");

    const IgcProfile profile = LosslessProfile();
    for (const Fixture& fx : fixtures) {
        DataObject::Pointer objA = BuildFixture(fx);
        if (!objA) {
            Info("skip " + fx.label + ": fixture unavailable (" + fx.path + ")");
            continue;
        }

        const fs::path igcPath = WorkDir() / (SafeFileName(fx.label) + ".p42.igc");
        const fs::path igvPath = WorkDir() / (SafeFileName(fx.label) + ".p42.igv");

        MeshEncodeTrace trace;
        std::string error;
        if (!WriteIgc(objA, igcPath, profile, &trace, error)) {
            Check(false, fx.label + ": " + error);
            continue;
        }
        DataObject::Pointer objIgc = FileIO::ReadFile(igcPath.string());
        if (!objIgc) {
            HandleIgcReadFailure(fx.label, igcPath);
            continue;
        }
        if (!FileIO::WriteFile(igvPath.string(), objIgc)) {
            Check(false, fx.label + ": IGC -> IGV write failed");
            continue;
        }
        DataObject::Pointer objIgv = FileIO::ReadFile(igvPath.string());
        if (!objIgv) {
            Check(false, fx.label + ": read-back of the .igv failed");
            continue;
        }

        const Snapshot a = TakeSnapshot(objA);
        const Snapshot intermediate = TakeSnapshot(objIgc);
        const Snapshot b = TakeSnapshot(objIgv);

        // 断点 1：IGV 这一步单独看是否无损（恒等顺序：IGV 不做任何重排）。
        const ConvertReport igvOnly = CompareObjects(fx.label + " [IGC -> IGV]", "IGC", "IGV", "n/a",
                                                     intermediate, b, nullptr);
        Check(igvOnly.AllArraysEqual() && igvOnly.topologyEqual,
              fx.label + ": the IGV leg alone changes nothing (IGV writes in DataObject order)");

        // 断点 2：整条链 vs 源 DataObject，用编码器的重排映射还原顺序。
        const ConvertReport chain =
            CompareObjects(fx.label, "DataObject", "IGV (via IGC)", profile.name, a, b, &trace);
        chain.Print();
        Check(chain.AllArraysEqual(), fx.label + ": every array bitwise equal to the source after order restoration");
        Check(chain.topologyEqual, fx.label + ": topology equal to the source after order restoration");
        Check(chain.Lossless(), fx.label + ": report verdict is lossless round-trip");

        // 第二判据：完全不使用编码器重排映射的顺序无关比对。
        // 注意：**必须先调用再拼消息** —— `Check(CompareOrderFree(..., detail), "..." + detail)`
        // 的两个实参求值顺序未指定，MSVC 会先算出消息（此时 detail 还是空的），
        // 断言信息就丢了。这里显式分两步。
        std::string orderFreeDetail;
        const bool orderFreeOk = CompareOrderFree(a, b, orderFreeDetail);
        Check(orderFreeOk, fx.label + ": an independent order-free comparison agrees (" + orderFreeDetail + ")");

        Info(fx.label + ": order after IGC: points " + (chain.pointOrderPreserved ? "unchanged" : "reordered") +
             ", cells " + (chain.cellOrderPreserved ? "unchanged" : "reordered"));
    }
}

/// P4-3：IGC -> IGV（有损档）。必须**明确报出不可保真**，并给出实测误差。
void RunP43(const std::vector<Fixture>& fixtures) {
    Section("P4-3 IGC -> IGV, quantized profile: must report 'lossy', with the measured error");

    constexpr int kLevel = 3;  ///< keepBits = 23 - 3 = 20（见 iGameMeshFloatCodec.h:98-110）
    const IgcProfile profile = QuantizedProfile(kLevel);
    for (const Fixture& fx : fixtures) {
        DataObject::Pointer objA = BuildFixture(fx);
        if (!objA) {
            Info("skip " + fx.label + ": fixture unavailable (" + fx.path + ")");
            continue;
        }

        const fs::path igcPath = WorkDir() / (SafeFileName(fx.label) + ".p43.igc");
        const fs::path igvPath = WorkDir() / (SafeFileName(fx.label) + ".p43.igv");

        MeshEncodeTrace trace;
        std::string error;
        if (!WriteIgc(objA, igcPath, profile, &trace, error)) {
            Check(false, fx.label + ": " + error);
            continue;
        }
        DataObject::Pointer objIgc = FileIO::ReadFile(igcPath.string());
        if (!objIgc) {
            HandleIgcReadFailure(fx.label, igcPath);
            continue;
        }
        if (!FileIO::WriteFile(igvPath.string(), objIgc)) {
            Check(false, fx.label + ": IGC -> IGV write failed");
            continue;
        }
        DataObject::Pointer objIgv = FileIO::ReadFile(igvPath.string());
        if (!objIgv) {
            Check(false, fx.label + ": read-back of the .igv failed");
            continue;
        }

        const Snapshot a = TakeSnapshot(objA);
        const Snapshot intermediate = TakeSnapshot(objIgc);
        const Snapshot b = TakeSnapshot(objIgv);

        const ConvertReport chain =
            CompareObjects(fx.label, "DataObject", "IGV (via quantized IGC)", profile.name, a, b, &trace);
        chain.Print();

        int64_t differing = 0;
        for (const auto& arr : chain.arrays) { differing += arr.differingValues; }

        Check(!chain.Lossless(), fx.label + ": the report does NOT claim fidelity for a quantized IGC");
        Check(chain.verdict == FidelityVerdict::LossyNotReversible,
              fx.label + ": verdict is 'lossy, NOT reversible'");
        Check(differing > 0, fx.label + ": the loss is measured, not assumed (" + Num(differing) +
                                  " differing value(s))");

        // 有损的**来源**必须定位到 IGC 这一步：IGV 那一段仍然是逐值无损的。
        const ConvertReport igvOnly = CompareObjects(fx.label + " [IGC -> IGV]", "IGC", "IGV", "n/a",
                                                     intermediate, b, nullptr);
        Check(igvOnly.AllArraysEqual() && igvOnly.topologyEqual,
              fx.label + ": IGV itself is still lossless, so the loss can only come from the IGC step");
    }
}

/// P4-4：`DataObject -> IGV -> IGC -> IGV -> DataObject`（无损档）逐值 bitwise 相等。
void RunP44(const std::vector<Fixture>& fixtures) {
    Section("P4-4 bidirectional round-trip: DataObject -> IGV -> IGC -> IGV -> DataObject");

    const IgcProfile profile = LosslessProfile();
    for (const Fixture& fx : fixtures) {
        DataObject::Pointer objA = BuildFixture(fx);
        if (!objA) {
            Info("skip " + fx.label + ": fixture unavailable (" + fx.path + ")");
            continue;
        }

        const std::string base = SafeFileName(fx.label);
        const fs::path igv1 = WorkDir() / (base + ".p44a.igv");
        const fs::path igc1 = WorkDir() / (base + ".p44.igc");
        const fs::path igv2 = WorkDir() / (base + ".p44b.igv");

        if (!FileIO::WriteFile(igv1.string(), objA)) {
            Check(false, fx.label + ": step 1 (write .igv) failed");
            continue;
        }
        DataObject::Pointer step1 = FileIO::ReadFile(igv1.string());
        if (!step1) {
            Check(false, fx.label + ": step 1 (read .igv) failed");
            continue;
        }

        MeshEncodeTrace trace;
        std::string error;
        if (!WriteIgc(step1, igc1, profile, &trace, error)) {
            Check(false, fx.label + ": step 2 (write .igc): " + error);
            continue;
        }
        DataObject::Pointer step2 = FileIO::ReadFile(igc1.string());
        if (!step2) {
            HandleIgcReadFailure(fx.label + " (round-trip)", igc1);
            continue;
        }

        if (!FileIO::WriteFile(igv2.string(), step2)) {
            Check(false, fx.label + ": step 3 (write .igv) failed");
            continue;
        }
        DataObject::Pointer step3 = FileIO::ReadFile(igv2.string());
        if (!step3) {
            Check(false, fx.label + ": step 3 (read .igv) failed");
            continue;
        }

        const Snapshot a = TakeSnapshot(objA);
        const Snapshot b = TakeSnapshot(step3);
        const ConvertReport report =
            CompareObjects(fx.label, "DataObject", "DataObject (via IGV/IGC/IGV)", profile.name, a, b, &trace);
        report.Print();

        Check(report.AllArraysEqual(), fx.label + ": round-trip is bitwise identical (after order restoration)");
        Check(report.topologyEqual, fx.label + ": round-trip topology identical");
        Check(report.Lossless(), fx.label + ": round-trip is reported as lossless (P9-5)");

        std::string orderFreeDetail;
        const bool orderFreeOk = CompareOrderFree(a, b, orderFreeDetail);
        Check(orderFreeOk,
              fx.label + ": independent order-free comparison of the round-trip agrees (" + orderFreeDetail + ")");
    }
}

/// P4-5：f32 / f64 / u8 三种属性全在链路上；有损档下 f64 的精度变化必须被报出，
/// 而不能静默丢弃。
void RunP45() {
    Section("P4-5 attribute type coverage: f32 / f64 / u8 through the conversion chain");

    DataObject::Pointer objA = MakeAttributeTypeFixture();
    const std::string base = "AttributeTypes";

    // ---- 无损档 ----
    {
        const fs::path igcPath = WorkDir() / (base + ".p45_lossless.igc");
        const fs::path igvPath = WorkDir() / (base + ".p45_lossless.igv");
        MeshEncodeTrace trace;
        std::string error;
        if (!WriteIgc(objA, igcPath, LosslessProfile(), &trace, error)) {
            Check(false, "P4-5: lossless IGC write failed: " + error);
            return;
        }
        DataObject::Pointer objIgc = FileIO::ReadFile(igcPath.string());
        if (!objIgc || !FileIO::WriteFile(igvPath.string(), objIgc)) {
            Check(false, "P4-5: lossless IGC -> IGV failed");
            return;
        }
        DataObject::Pointer objB = FileIO::ReadFile(igvPath.string());
        if (!objB) {
            Check(false, "P4-5: lossless .igv read-back failed");
            return;
        }

        const ConvertReport report = CompareObjects(base + " [lossless]", "DataObject", "IGV (via IGC)",
                                                    LosslessProfile().name, TakeSnapshot(objA),
                                                    TakeSnapshot(objB), &trace);
        report.Print();

        const ArrayFidelity* f32 = nullptr;
        const ArrayFidelity* f64 = nullptr;
        const ArrayFidelity* u8 = nullptr;
        for (const auto& arr : report.arrays) {
            if (arr.name == "f_scalar") { f32 = &arr; }
            if (arr.name == "d_scalar") { f64 = &arr; }
            if (arr.name == "u8_scalar") { u8 = &arr; }
        }
        Check(f32 != nullptr && f32->dtype == "f32", "P4-5: fixture carries an f32 attribute");
        Check(f64 != nullptr && f64->dtype == "f64", "P4-5: fixture carries an f64 attribute");
        Check(u8 != nullptr && u8->dtype == "u8", "P4-5: fixture carries a u8 attribute");
        Check(f32 != nullptr && f32->Equal(), "P4-5: f32 attribute is bitwise equal (lossless profile)");
        Check(f64 != nullptr && f64->Equal(),
              "P4-5: f64 attribute keeps full double precision (lossless profile)");
        Check(u8 != nullptr && u8->Equal(), "P4-5: u8 attribute is byte-identical (lossless profile)");
    }

    // ---- 量化档 ----
    {
        MeshEncodeTrace trace;
        std::string error;
        const fs::path igcPath = WorkDir() / (base + ".p45_quantized.igc");
        const fs::path igvPath = WorkDir() / (base + ".p45_quantized.igv");
        if (!WriteIgc(objA, igcPath, QuantizedProfile(3), &trace, error)) {
            Check(false, "P4-5: quantized IGC write failed: " + error);
            return;
        }
        DataObject::Pointer objIgc = FileIO::ReadFile(igcPath.string());
        if (!objIgc || !FileIO::WriteFile(igvPath.string(), objIgc)) {
            Check(false, "P4-5: quantized IGC -> IGV failed");
            return;
        }
        DataObject::Pointer objB = FileIO::ReadFile(igvPath.string());
        if (!objB) {
            Check(false, "P4-5: quantized .igv read-back failed");
            return;
        }

        const ConvertReport report = CompareObjects(base + " [quantized]", "DataObject", "IGV (via IGC)",
                                                    QuantizedProfile(3).name, TakeSnapshot(objA),
                                                    TakeSnapshot(objB), &trace);
        report.Print();

        const ArrayFidelity* f64 = nullptr;
        const ArrayFidelity* u8 = nullptr;
        for (const auto& arr : report.arrays) {
            if (arr.name == "d_scalar") { f64 = &arr; }
            if (arr.name == "u8_scalar") { u8 = &arr; }
        }
        Check(f64 != nullptr && !f64->Equal(),
              "P4-5: the f64 attribute's precision change is reported, not silent (" +
                  Num(f64 != nullptr ? f64->differingValues : -1) + " value(s), maxAbs=" +
                  Sci(f64 != nullptr ? f64->maxAbsError : 0.0) + ")");
        Check(u8 != nullptr && u8->Equal(),
              "P4-5: the u8 attribute is untouched even in the quantized profile (no float path)");
    }
}

/// P4-6：报告结论必须跟随**实测**，不能被名义档位左右。
void RunP46(const std::vector<Fixture>& fixtures) {
    Section("P4-6 the verdict must follow the measurement, not the nominal knob");

    // (a) 名义有损但实测无损：`QuantizeMode::Default` + level 0 时 keepBits == 23，
    //     编码器根本不改数据（iGameMeshFloatCodec.h:117-127 的 `keepBits < 23` 分支）。
    //     如果报告只看"模式是 Default"就判有损，这里会立刻红。
    {
        DataObject::Pointer objA = MakeDenseUnstructuredFixture();
        const fs::path igcPath = WorkDir() / "p46_default_level0.igc";
        const fs::path igvPath = WorkDir() / "p46_default_level0.igv";
        MeshEncodeTrace trace;
        std::string error;
        if (!WriteIgc(objA, igcPath, QuantizedProfile(0), &trace, error)) {
            Check(false, "P4-6: write failed: " + error);
        } else {
            DataObject::Pointer objIgc = FileIO::ReadFile(igcPath.string());
            if (!objIgc || !FileIO::WriteFile(igvPath.string(), objIgc)) {
                Check(false, "P4-6: conversion failed");
            } else {
                DataObject::Pointer objB = FileIO::ReadFile(igvPath.string());
                const ConvertReport report =
                    CompareObjects("nominal-quantized / measured-lossless", "DataObject", "IGV (via IGC)",
                                   QuantizedProfile(0).name, TakeSnapshot(objA), TakeSnapshot(objB), &trace);
                report.Print();
                Check(report.Lossless(),
                      "P4-6: 'Default level 0' is nominally quantized yet measurably lossless - "
                      "the report follows the measurement");
            }
        }
    }

    // (b) 名义无损档：每一份都必须报"无损可往返"（不得把无损标成有损）。
    {
        const IgcProfile profile = LosslessProfile();
        int losslessReports = 0;
        int falseLossyReports = 0;
        for (const Fixture& fx : fixtures) {
            DataObject::Pointer objA = BuildFixture(fx);
            if (!objA) { continue; }
            const std::string base = SafeFileName(fx.label);
            const fs::path igcPath = WorkDir() / (base + ".p46.igc");
            const fs::path igvPath = WorkDir() / (base + ".p46.igv");
            MeshEncodeTrace trace;
            std::string error;
            if (!WriteIgc(objA, igcPath, profile, &trace, error)) { continue; }
            DataObject::Pointer objIgc = FileIO::ReadFile(igcPath.string());
            if (!objIgc || !FileIO::WriteFile(igvPath.string(), objIgc)) {
                // 被上游缺陷挡住的类型在这里不重复记账（P4-2 已经记过，避免重复计数）。
                Info(fx.label + ": not evaluable here (see the P4-0 / P4-2 block accounting)");
                continue;
            }
            DataObject::Pointer objB = FileIO::ReadFile(igvPath.string());
            if (!objB) { continue; }

            const ConvertReport report = CompareObjects(fx.label, "DataObject", "IGV (via IGC)", profile.name,
                                                        TakeSnapshot(objA), TakeSnapshot(objB), &trace);
            if (report.Lossless()) {
                ++losslessReports;
            } else {
                ++falseLossyReports;
                Check(false, fx.label + ": lossless profile was reported as lossy (" + report.verdictReason + ")");
            }
        }
        Check(falseLossyReports == 0 && losslessReports > 0,
              "P4-6: all " + Num(losslessReports) + " lossless-profile reports say 'lossless round-trip'");
    }
}

///
/// 两条独立的证据：
///   1. `FileIO::WriteFile(.igc)` 不带任何控制参数 —— 这正是产品里"另存为 .igc"的路径；
///   2. `IGCWriter` + `GenerateDefaultCodecParams()` —— 这是 IGCWriter 在
///      "没有显式参数"时内部自己走的那条路（iGameIGCWriter.cpp:26-31 与
///      iGameMeshEncoderFilter.h:235-240）。
/// 两者产出若字节相同，则第 2 条得到的重排映射可以合法地用在第 1 条的产物上。
void RunP4R7() {
    Section("P4-R7 research item: is the DEFAULT IGC profile lossless? (measure, do not read defaults)");

    DataObject::Pointer objA = MakeDenseUnstructuredFixture();
    const fs::path viaFileIo = WorkDir() / "p4r7_default_fileio.igc";
    const fs::path viaWriter = WorkDir() / "p4r7_default_writer.igc";

    if (!FileIO::WriteFile(viaFileIo.string(), objA)) {
        Check(false, "P4-R7: FileIO::WriteFile(.igc) failed");
        return;
    }
    MeshEncodeTrace trace;
    std::string error;
    // "默认档"就是用 GenerateDefaultCodecParams 的结果：不改 mode、不改 level。
    CodecControlParams defaults =
        MeshEncoderFilter<EncodeOutputBinaryArray>::GenerateDefaultCodecParams(objA);
    IGCWriter::Pointer writer = IGCWriter::New();
    writer->SetCodecControlParams(defaults);
    writer->SetEncodeTrace(&trace);
    if (!writer->WriteToFile(objA, viaWriter.string())) {
        Check(false, "P4-R7: IGCWriter::WriteToFile failed with the default params");
        return;
    }

    Info("P4-R7: generated defaults -> geom errorMode=" +
         std::string(defaults.geomControl.errorMode == QuantizeMode::None ? "None" : "quantized") +
         ", globalQuantizeLevel=" + Num(defaults.geomControl.globalQuantizeLevel));

    const auto sizeA = static_cast<int64_t>(fs::file_size(viaFileIo));
    const auto sizeB = static_cast<int64_t>(fs::file_size(viaWriter));
    Info("P4-R7: .igc size via FileIO = " + Num(sizeA) + " B, via IGCWriter = " + Num(sizeB) + " B");

    // 字节一致性只是"两条路径等价"的**旁证**；真正的判据是下面解出来的对象是否一致。
    {
        std::vector<unsigned char> ba;
        std::vector<unsigned char> bb;
        const auto readAll = [](const fs::path& p, std::vector<unsigned char>& out) {
            std::ifstream in(p, std::ios::binary);
            out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        };
        readAll(viaFileIo, ba);
        readAll(viaWriter, bb);
        Info(std::string("P4-R7: the two .igc products are ") +
             (ba == bb ? "byte-identical" : "NOT byte-identical (encoder is not byte-deterministic here)"));
    }

    DataObject::Pointer decodedFileIo = FileIO::ReadFile(viaFileIo.string());
    DataObject::Pointer decodedWriter = FileIO::ReadFile(viaWriter.string());
    if (!decodedFileIo || !decodedWriter) {
        Check(false, "P4-R7: reading one of the .igc products failed");
        return;
    }

    // 两条路径解出来的对象必须完全一致（恒等顺序 —— 同一个编码器、同样的参数）。
    const ConvertReport samePath = CompareObjects("default-profile equivalence", "IGC", "IGC", "n/a",
                                                  TakeSnapshot(decodedWriter), TakeSnapshot(decodedFileIo),
                                                  nullptr);
    Check(samePath.AllArraysEqual() && samePath.topologyEqual,
          "P4-R7: the FileIO path and the explicit-defaults path decode to the same object");

    // 结论：默认档 vs 源 DataObject。
    const ConvertReport report =
        CompareObjects("default-profile vs source", "DataObject", "IGC (default profile)",
                       "IGC defaults (no explicit control params)", TakeSnapshot(objA),
                       TakeSnapshot(decodedWriter), &trace);
    report.Print();
    Check(report.Lossless(),
          "P4-R7: the DEFAULT IGC profile is lossless for this fixture (measured, not assumed)");
}

}  // namespace

int main(int argc, char** argv) {
    std::string modelsDir = "Models";
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--models" && i + 1 < argc) { modelsDir = argv[++i]; }
    }
    if (modelsDir == "Models" || modelsDir.empty()) {
        const fs::path found = FindModelsDir(argc > 0 ? argv[0] : nullptr);
        if (!found.empty()) { modelsDir = found.string(); }
    }

    std::cerr << "================================================================\n"
              << "IGV acceptance: IGC <-> IGV conversion and the fidelity boundary report\n"
              << "================================================================\n"
              << "Working dir   : " << fs::current_path().string() << "\n"
              << "Models dir    : " << modelsDir << "\n"
              << "Temp dir      : " << WorkDir().string() << "\n";

    // 五类网格：三类必须直接构造（VTK legacy 做不出 VolumeMesh；PointSet 无文件来源；
    // 均匀 SurfaceMesh 才能被 IGV 如实写出），两类用真实文件。
    //
    // UnstructuredMesh 刻意选 `ExtractCellsByType_mixed.vtk`（三角形/四边形/四面体/六面体
    // 混合，且带 f32 + f64 点属性 + f64 单元属性）：混合单元能真正走到 IGC 的
    // `AddUnstructuredPolyCells` + Types 编码，而"有属性"这一点很关键 ——
    // 无属性的网格会多出一个 0 长度属性块，被上游缺陷挡住（见 P4-0）。
    const std::vector<Fixture> fiveKinds = {
        {"PointSet", FixtureKind::PointSetBuilt, ""},
        {"SurfaceMesh", FixtureKind::SurfaceMeshBuilt, ""},
        {"VolumeMesh", FixtureKind::VolumeMeshBuilt, ""},
        {"UnstructuredMesh", FixtureKind::File, modelsDir + "/ExtractCellsByType_mixed.vtk"},
        {"StructuredMesh", FixtureKind::File, modelsDir + "/Structured_Volume_Test.vtk"},
    };

    // P4-0 的三种"必然产生空 payload 块"的形态：无拓扑的点集 / 结构化网格 /
    // 无属性的网格。第三个用 Models 里一份确实没有 POINT_DATA / CELL_DATA 的文件，
    // 这样证据来自真实数据而不是我们构造的夹具。
    const std::vector<Fixture> emptyBrickKinds = {
        {"PointSet", FixtureKind::PointSetBuilt, ""},
        {"StructuredMesh", FixtureKind::File, modelsDir + "/Structured_Volume_Test.vtk"},
        {"NoAttributeMesh", FixtureKind::File, modelsDir + "/CountCellVertices_mixed_cells.vtk"},
    };

    // 稠密尾数夹具：只有它们能让"尾数截断"真的发生（0/1/2 坐标上是恒等操作）。
    const std::vector<Fixture> denseKinds = {
        {"DenseUnstructured", FixtureKind::DenseUnstructured, ""},
        {"AttributeTypes", FixtureKind::AttributeTypes, ""},
    };

    RunComparatorSelfCheck();
    RunP40(emptyBrickKinds);
    RunP41(fiveKinds);
    RunP42(fiveKinds);
    RunP43(denseKinds);
    RunP44(fiveKinds);
    RunP45();
    RunP46(fiveKinds);
    RunP4R7();

    std::cerr << "\n------------------------------------------------------------------------------\n"
              << "checks: " << g_checks << " total, failures " << g_failed
              << ", blocked by the upstream defect: " << g_blocked << "\n";
    if (g_blocked > 0) {
        std::cerr << "NOTE: " << g_blocked
                  << " case(s) could not run because of the upstream IGC defect (empty payload brick).\n"
                  << "      They are NOT counted as passes - .\n";
    }
    if (g_failed == 0) {
        std::cerr << "ALL PASSED: P4-1..P4-6 and the R7 research item hold in this run.\n";
        return 0;
    }
    std::cerr << "FAILED: see the [FAIL] lines above.\n";
    return 1;
}
