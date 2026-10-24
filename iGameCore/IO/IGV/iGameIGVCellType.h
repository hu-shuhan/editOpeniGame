#ifndef iGameIGVCellType_h
#define iGameIGVCellType_h

/**
 * @file  iGameIGVCellType.h
 * @brief IGV Types whitelist/forbidden list and the iGameCellType-to-VTK code mapping.
 *
 * iGame and VTK disagree in the low code range. iGameCellType.h gives explicit values only
 * for IG_EMPTY_CELL, IG_VERTEX, and IG_LINE; later values are implicit, and VTK has no FACE:
 *
 *     iGame: FACE=3  TRIANGLE=4  QUAD=5  POLYGON=6  VOLUME=7  POLYHEDRON=8
 *            TETRA=9 HEXAHEDRON=10 PYRAMID=11 PRISM=12
 *     VTK  : (none)  TRIANGLE=5  QUAD=9  POLYGON=7  (none)    (none)
 *            TETRA=10 HEXAHEDRON=12 PYRAMID=13 WEDGE=14
 *
 * Writing raw iGame codes into a VTK reader therefore produces wrong geometry without an
 * error. This is why the file carries CellTypeEncoding="IGCellType".
 *
 * High codes (quadratic 21-37, Lagrange 68-74) match VTK.
 *
 * Rules:
 *  1. Values are written as IGCellType symbols, never as hand-written numbers.
 *  2. VTK codes are kept in kVtkStandardCodes and queried through VtkCodeOf().
 *  3. Compile-time static_asserts keep the whitelist values and the VTK mapping consistent.
 *
 * Pure static data; no HDF5 dependency.
 */

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "iGameCellType.h"
#include "iGameIGVFormat.h"

IGAME_NAMESPACE_BEGIN

namespace igv {

// =============================================================================
// VTK 标准码参照表（来源：VTK 官方 Common/DataModel/vtkCellType.h）
// =============================================================================
//
// 独立参照，不从 iGame 侧推导。用于自动判定 `vtkCompatible`，
// 以及给外部读者一份可核对的对照。
//
// 易错点（编写时已逐项确认）：
//   VTK 1..4 = VERTEX, POLY_VERTEX, LINE, POLY_LINE
//   VTK 6..8 = TRIANGLE_STRIP, POLYGON, PIXEL
//   VTK 跳过 35（VTK_CUBIC_LINE 占 35），QUADRATIC_POLYGON = 36
//   VTK_WEDGE = 13, VTK_PYRAMID = 14（与 iGame 的 PRISM/PYRAMID 顺序相反）
enum class VtkCellCode : int32_t {
    kNone = -1,
    Vertex = 1,
    PolyVertex = 2,
    Line = 3,
    PolyLine = 4,
    Triangle = 5,
    TriangleStrip = 6,
    Polygon = 7,
    Pixel = 8,
    Quad = 9,
    Tetra = 10,
    Voxel = 11,
    Hexahedron = 12,
    Wedge = 13,
    Pyramid = 14,
    QuadraticEdge = 21,
    QuadraticTriangle = 22,
    QuadraticQuad = 23,
    QuadraticTetra = 24,
    QuadraticHexahedron = 25,
    QuadraticWedge = 26,
    QuadraticPyramid = 27,
    BiquadraticQuad = 28,
    TriquadraticHexahedron = 29,
    QuadraticLinearQuad = 30,
    QuadraticLinearWedge = 31,
    BiquadraticQuadraticWedge = 32,
    BiquadraticQuadraticHexahedron = 33,
    BiquadraticTriangle = 34,
    CubicLine = 35,
    QuadraticPolygon = 36,
    TriquadraticPyramid = 37,
    ConvexPointSet = 41,
    Polyhedron = 42,
    LagrangeCurve = 68,
    LagrangeTriangle = 69,
    LagrangeQuadrilateral = 70,
    LagrangeTetrahedron = 71,
    LagrangeHexahedron = 72,
    LagrangeWedge = 73,
    LagrangePyramid = 74,
};

/// iGame `IGCellType` → 对应 VTK 标准码。返回 `VtkCellCode::kNone` 表示 VTK 无对应。
///
/// 注意这里返回的是 **VTK 语义上等价的码**，不是数值相等的码 ——
/// 数值是否相等由调用方比较（这正是 `vtkCompatible` 的含义）。
[[nodiscard]] constexpr VtkCellCode VtkCodeOf(int32_t igCellType) noexcept {
    switch (static_cast<IGCellType>(igCellType)) {
        case IG_VERTEX:                     return VtkCellCode::Vertex;
        case IG_LINE:                       return VtkCellCode::Line;
        case IG_TRIANGLE:                   return VtkCellCode::Triangle;
        case IG_QUAD:                       return VtkCellCode::Quad;
        case IG_POLYGON:                    return VtkCellCode::Polygon;
        case IG_TETRA:                      return VtkCellCode::Tetra;
        case IG_HEXAHEDRON:                 return VtkCellCode::Hexahedron;
        case IG_PYRAMID:                    return VtkCellCode::Pyramid;
        case IG_PRISM:                      return VtkCellCode::Wedge;
        case IG_QUADRATIC_EDGE:             return VtkCellCode::QuadraticEdge;
        case IG_QUADRATIC_TRIANGLE:         return VtkCellCode::QuadraticTriangle;
        case IG_QUADRATIC_QUAD:             return VtkCellCode::QuadraticQuad;
        case IG_QUADRATIC_TETRA:            return VtkCellCode::QuadraticTetra;
        case IG_QUADRATIC_HEXAHEDRON:       return VtkCellCode::QuadraticHexahedron;
        case IG_QUADRATIC_PRISM:            return VtkCellCode::QuadraticWedge;
        case IG_QUADRATIC_PYRAMID:          return VtkCellCode::QuadraticPyramid;
        case IG_BIQUADRATIC_QUAD:           return VtkCellCode::BiquadraticQuad;
        case IG_TRIQUADRATIC_HEXAHEDRON:    return VtkCellCode::TriquadraticHexahedron;
        case IG_QUADRATIC_LINEAR_QUAD:      return VtkCellCode::QuadraticLinearQuad;
        case IG_QUADRATIC_LINEAR_WEDGE:     return VtkCellCode::QuadraticLinearWedge;
        case IG_BIQUADRATIC_QUADRATIC_WEDGE:        return VtkCellCode::BiquadraticQuadraticWedge;
        case IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON:   return VtkCellCode::BiquadraticQuadraticHexahedron;
        case IG_BIQUADRATIC_TRIANGLE:       return VtkCellCode::BiquadraticTriangle;
        case IG_QUADRATIC_POLYGON:          return VtkCellCode::QuadraticPolygon;
        case IG_TRIQUADRATIC_PYRAMID:       return VtkCellCode::TriquadraticPyramid;
        case IG_POLY_LINE:                  return VtkCellCode::PolyLine;
        case IG_LAGRANGE_CURVE:             return VtkCellCode::LagrangeCurve;
        case IG_LAGRANGE_TRIANGLE:          return VtkCellCode::LagrangeTriangle;
        case IG_LAGRANGE_QUADRILATERAL:     return VtkCellCode::LagrangeQuadrilateral;
        case IG_LAGRANGE_TETRAHEDRON:       return VtkCellCode::LagrangeTetrahedron;
        case IG_LAGRANGE_HEXAHEDRON:        return VtkCellCode::LagrangeHexahedron;
        case IG_LAGRANGE_PRISM:             return VtkCellCode::LagrangeWedge;
        case IG_LAGRANGE_PYRAMID:           return VtkCellCode::LagrangePyramid;
        // IG_EMPTY_CELL / IG_FACE / IG_VOLUME / IG_POLYHEDRON 不属于可写单元，无对应
        default:                            return VtkCellCode::kNone;
    }
}

// =============================================================================
// 白名单
// =============================================================================

/// `Types` 可写出的单元类型条目。
struct CellTypeEntry {
    IGCellType value;      ///< 由编译器求值（不手写数字）
    std::string_view name; ///< iGame 侧名称，供错误信息与调试
    bool vtkCompatible;    ///< value 的**数值**是否等于其 VTK 等价码
};

/**
 * 白名单。只把"确定能正确表达为一级索引单元"的类型放进来。
 *
 * 期望项数 33（= iGameCellType.h 的 37 个具名项 − 4 个禁止项）。
 * 而本文件的编译期断言只能验证表内自洽）。
 *
 * `vtkCompatible` 的判定依据是**数值**是否与 VTK 等价码相同：
 *   - 高值段（二次型 / Lagrange）全部相同 → true
 *   - 低值段因 IG_FACE 占位而整体错位 → false
 */
inline constexpr CellTypeEntry kCellTypeWhitelist[] = {
    // ---- 0D / 1D（注意 IG_LINE=2 而 VTK_LINE=3）----
    {IG_VERTEX,               "Vertex",              true},   // 1 == VTK_VERTEX(1)
    {IG_LINE,                 "Edge",                false},  // 2 != VTK_LINE(3)
    {IG_POLY_LINE,            "PolyLine",            false},  // 35 != VTK_POLY_LINE(4)

    // ---- 2D 线性（因 IG_FACE 占 3，这一段整体错位一位）----
    {IG_TRIANGLE,             "Triangle",            false},  // 4 != VTK_TRIANGLE(5)
    {IG_POLYGON,              "Polygon",             false},  // 6 != VTK_POLYGON(7)
    {IG_QUAD,                 "Quad",                false},  // 5 != VTK_QUAD(9)

    // ---- 3D 线性（同样错位；且 PRISM/PYRAMID 顺序与 VTK 相反）----
    {IG_TETRA,                "Tetra",               false},  // 9  != VTK_TETRA(10)
    {IG_HEXAHEDRON,           "Hexahedron",          false},  // 10 != VTK_HEXAHEDRON(12)
    {IG_PYRAMID,              "Pyramid",             false},  // 11 != VTK_PYRAMID(14)
    {IG_PRISM,                "Prism",               false},  // 12 != VTK_WEDGE(13)

    // ---- 二次型 / 高阶：从 21 起与 VTK 完全一致 ----
    {IG_QUADRATIC_EDGE,             "QuadraticEdge",             true},  // 21
    {IG_QUADRATIC_TRIANGLE,         "QuadraticTriangle",         true},  // 22
    {IG_QUADRATIC_QUAD,             "QuadraticQuad",             true},  // 23
    {IG_QUADRATIC_TETRA,            "QuadraticTetra",            true},  // 24
    {IG_QUADRATIC_HEXAHEDRON,       "QuadraticHexahedron",       true},  // 25
    {IG_QUADRATIC_PRISM,            "QuadraticPrism",            true},  // 26
    {IG_QUADRATIC_PYRAMID,          "QuadraticPyramid",          true},  // 27
    {IG_BIQUADRATIC_QUAD,           "BiquadraticQuad",           true},  // 28
    {IG_TRIQUADRATIC_HEXAHEDRON,    "TriquadraticHexahedron",    true},  // 29
    {IG_QUADRATIC_LINEAR_QUAD,      "QuadraticLinearQuad",       true},  // 30
    {IG_QUADRATIC_LINEAR_WEDGE,     "QuadraticLinearWedge",      true},  // 31
    {IG_BIQUADRATIC_QUADRATIC_WEDGE,      "BiquadraticQuadraticWedge",       true},  // 32
    {IG_BIQUADRATIC_QUADRATIC_HEXAHEDRON, "BiquadraticQuadraticHexahedron",  true},  // 33
    {IG_BIQUADRATIC_TRIANGLE,       "BiquadraticTriangle",       true},  // 34
    {IG_QUADRATIC_POLYGON,          "QuadraticPolygon",          true},  // 36
    {IG_TRIQUADRATIC_PYRAMID,       "TriquadraticPyramid",       true},  // 37

    // ---- Lagrange：68..74 与 VTK 一致 ----
    {IG_LAGRANGE_CURVE,             "LagrangeCurve",         true},  // 68
    {IG_LAGRANGE_TRIANGLE,          "LagrangeTriangle",      true},  // 69
    {IG_LAGRANGE_QUADRILATERAL,     "LagrangeQuadrilateral", true},  // 70
    {IG_LAGRANGE_TETRAHEDRON,       "LagrangeTetrahedron",   true},  // 71
    {IG_LAGRANGE_HEXAHEDRON,        "LagrangeHexahedron",    true},  // 72
    {IG_LAGRANGE_PRISM,             "LagrangePrism",         true},  // 73
    {IG_LAGRANGE_PYRAMID,           "LagrangePyramid",       true},  // 74
};

// =============================================================================
// 禁止表
// =============================================================================

/**
 *
 * 期望项数 4（= 37 − 33）。
 */
struct ForbiddenCellType {
    IGCellType value;
    std::string_view name;
    std::string_view reason;
};

inline constexpr ForbiddenCellType kForbiddenCellTypes[] = {
    {IG_EMPTY_CELL, "EmptyCell",
     "placeholder value (0), not a real cell type; VTK 0 is undefined as well"},
    {IG_FACE, "Face",
     "iGame-internal intermediate-dimension marker (GetCellDimension returns -1), not a real cell;"
     " value 3 is VTK_LINE in VTK, so it would be silently mis-parsed as a line"},
    {IG_VOLUME, "Volume",
     "iGame-internal intermediate-dimension marker (GetCellDimension returns -1), not a real cell;"
     " value 7 is VTK_POLYGON in VTK, so it would be silently mis-parsed"},
    {IG_POLYHEDRON, "Polyhedron",
     "requires the secondary index path (volume->face->point). IGV 1.x CAN store it, but only"
     " when the file declares TopologyIndex=\"Secondary\" (then Connectivity holds face ids);"
     " in the default primary-index mode writing 8 would make face ids look like point ids."
     " value 8 is VTK_PIXEL in VTK, so it would also be silently mis-parsed outside IGV"},
};

// =============================================================================
// 查询辅助
// =============================================================================

/// 该值是否允许写入 `Types`。
[[nodiscard]] constexpr bool IsWritableCellType(int32_t value) noexcept {
    for (const auto& entry : kCellTypeWhitelist) {
        if (static_cast<int32_t>(entry.value) == value) { return true; }
    }
    return false;
}

/// 该值是否允许写入 `Types` —— **仅**在二级索引（多面体）模式下。
///
/// `IG_POLYHEDRON(8)` 在两级索引里是合法单元类型，但它**只有在**文件声明了
/// `TopologyIndex = "Secondary"`（即 `Connectivity` 存的是面 id）时才成立。
/// 一级索引模式下写 8 会得到一个"面 id 被当成点 id"的损坏文件，所以那里的
/// 禁止表仍然有效（见 `kForbiddenCellTypes`）。
[[nodiscard]] constexpr bool IsWritableCellTypeInSecondaryIndex(int32_t value) noexcept {
    return value == static_cast<int32_t>(IG_POLYHEDRON);
}

/// 命中禁止表则返回条目指针（用于给出精确错误信息）；否则返回 nullptr。
[[nodiscard]] constexpr const ForbiddenCellType* FindForbiddenCellType(int32_t value) noexcept {
    for (const auto& entry : kForbiddenCellTypes) {
        if (static_cast<int32_t>(entry.value) == value) { return &entry; }
    }
    return nullptr;
}

/// 是否属于"已知类型"（白名单 ∪ 禁止表）。用于区分"已知但禁止"与"完全未定义"。
[[nodiscard]] constexpr bool IsKnownCellType(int32_t value) noexcept {
    return IsWritableCellType(value) || FindForbiddenCellType(value) != nullptr;
}

/// 该类型写入 `Types` 后，其数值是否与 VTK 等价码相同。
[[nodiscard]] constexpr bool IsVtkNumericallyCompatible(int32_t igCellType) noexcept {
    const VtkCellCode vtk = VtkCodeOf(igCellType);
    if (vtk == VtkCellCode::kNone) { return false; }
    return static_cast<int32_t>(vtk) == igCellType;
}

inline constexpr size_t kCellTypeWhitelistSize =
    sizeof(kCellTypeWhitelist) / sizeof(kCellTypeWhitelist[0]);
inline constexpr size_t kForbiddenCellTypeSize =
    sizeof(kForbiddenCellTypes) / sizeof(kForbiddenCellTypes[0]);

// =============================================================================
// 编译期断言
// =============================================================================
//
// 这里做两件确定能做的事：
//   (1) 表内无重复、两表互斥
//   (2) 每条白名单的 vtkCompatible 标记与「值 == VTK 等价码」一致
//       —— 这把 IGCellType ↔ VTK 对照关系固化为编译期事实。
//
// **不**在这里断言"白名单 + 禁止表 == 37"这类总数：
//   总数断言只能固化一个数字，无法发现"漏了某个新单元类型"。
//   覆盖性必须由能解析 iGameCellType.h 的工具验证 ——

namespace detail {

[[nodiscard]] constexpr bool WhitelistForbiddenDisjoint() noexcept {
    for (const auto& w : kCellTypeWhitelist) {
        for (const auto& f : kForbiddenCellTypes) {
            if (w.value == f.value) { return false; }
        }
    }
    return true;
}

[[nodiscard]] constexpr bool WhitelistHasNoDuplicates() noexcept {
    for (size_t i = 0; i < kCellTypeWhitelistSize; ++i) {
        for (size_t j = i + 1; j < kCellTypeWhitelistSize; ++j) {
            if (kCellTypeWhitelist[i].value == kCellTypeWhitelist[j].value) { return false; }
        }
    }
    return true;
}

[[nodiscard]] constexpr bool ForbiddenHasNoDuplicates() noexcept {
    for (size_t i = 0; i < kForbiddenCellTypeSize; ++i) {
        for (size_t j = i + 1; j < kForbiddenCellTypeSize; ++j) {
            if (kForbiddenCellTypes[i].value == kForbiddenCellTypes[j].value) { return false; }
        }
    }
    return true;
}

/// 返回第一条 vtkCompatible 标记与实际不符的条目索引；全部相符时返回 -1。
[[nodiscard]] constexpr int FirstVtkCompatibleMismatch() noexcept {
    for (size_t i = 0; i < kCellTypeWhitelistSize; ++i) {
        const auto& e = kCellTypeWhitelist[i];
        if (e.vtkCompatible != IsVtkNumericallyCompatible(static_cast<int32_t>(e.value))) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace detail

static_assert(kCellTypeWhitelistSize > 0 && kForbiddenCellTypeSize > 0,
              "both tables must be non-empty");
static_assert(detail::WhitelistForbiddenDisjoint(),
              "the same IGCellType must not appear in both the whitelist and the forbidden table");
static_assert(detail::WhitelistHasNoDuplicates(), "duplicate IGCellType value in the whitelist");
static_assert(detail::ForbiddenHasNoDuplicates(), "duplicate IGCellType value in the forbidden table");

// 诊断信息给出**表内索引**（不是符号名 —— 符号名在 C++17 下不是常量表达式）。
static_assert(detail::FirstVtkCompatibleMismatch() < 0,
              "a whitelist entry's vtkCompatible flag disagrees with 'value == equivalent VTK code'. "
              "The index is in the compiler's constant-evaluation backtrace; "
              " prints the per-value table.");

// =============================================================================
// 关键值断言：这些数字是 IGV 与外部格式互操作的全部依据，必须钉死。
// =============================================================================
static_assert(static_cast<int32_t>(IG_EMPTY_CELL) == 0, "IG_EMPTY_CELL must be 0");
static_assert(static_cast<int32_t>(IG_VERTEX) == 1, "IG_VERTEX must be 1");
static_assert(static_cast<int32_t>(IG_LINE) == 2, "IG_LINE must be 2");
static_assert(static_cast<int32_t>(IG_FACE) == 3,
              "IG_FACE must be 3 - it occupies VTK's VTK_LINE slot and is why the low range is shifted");
static_assert(static_cast<int32_t>(IG_TRIANGLE) == 4,
              "IG_TRIANGLE must be 4 (note: VTK_TRIANGLE = 5, the values differ)");
static_assert(static_cast<int32_t>(IG_QUAD) == 5,
              "IG_QUAD must be 5 (note: VTK_QUAD = 9)");
static_assert(static_cast<int32_t>(IG_POLYGON) == 6,
              "IG_POLYGON must be 6 (note: VTK_POLYGON = 7)");
static_assert(static_cast<int32_t>(IG_VOLUME) == 7, "IG_VOLUME must be 7");
static_assert(static_cast<int32_t>(IG_POLYHEDRON) == 8, "IG_POLYHEDRON must be 8");
static_assert(static_cast<int32_t>(IG_TETRA) == 9,
              "IG_TETRA must be 9 (note: VTK_TETRA = 10)");
static_assert(static_cast<int32_t>(IG_HEXAHEDRON) == 10,
              "IG_HEXAHEDRON must be 10 (note: VTK_HEXAHEDRON = 12)");
static_assert(static_cast<int32_t>(IG_PYRAMID) == 11, "IG_PYRAMID must be 11");
static_assert(static_cast<int32_t>(IG_PRISM) == 12, "IG_PRISM must be 12");
static_assert(static_cast<int32_t>(IG_QUADRATIC_EDGE) == 21, "IG_QUADRATIC_EDGE must be 21");
static_assert(static_cast<int32_t>(IG_BIQUADRATIC_TRIANGLE) == 34, "IG_BIQUADRATIC_TRIANGLE must be 34");
static_assert(static_cast<int32_t>(IG_POLY_LINE) == 35,
              "IG_POLY_LINE implicitly becomes 35 (IG_BIQUADRATIC_TRIANGLE(34)+1), "
              "while VTK_POLY_LINE = 4");
static_assert(static_cast<int32_t>(IG_QUADRATIC_POLYGON) == 36, "IG_QUADRATIC_POLYGON must be 36");
static_assert(static_cast<int32_t>(IG_TRIQUADRATIC_PYRAMID) == 37, "IG_TRIQUADRATIC_PYRAMID must be 37");
static_assert(static_cast<int32_t>(IG_LAGRANGE_CURVE) == 68, "IG_LAGRANGE_CURVE must be 68");
static_assert(static_cast<int32_t>(IG_LAGRANGE_PYRAMID) == 74, "IG_LAGRANGE_PYRAMID must be 74");

/// `Types` 以 u8 存储，所有可写单元的码值必须落在 u8 值域。
static_assert(static_cast<int32_t>(IG_LAGRANGE_PYRAMID) <= 255,
              "an IGCellType value exceeds u8, so the Types dtype must be widened (see kTypesDtypeBytes in iGameIGVFormat.h)");

} // namespace igv

IGAME_NAMESPACE_END

#endif // iGameIGVCellType_h
