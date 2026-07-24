#ifndef DATACODEC_CODEC_TOPOLOGY_COMMON_CELLTYPECODEC_H
#define DATACODEC_CODEC_TOPOLOGY_COMMON_CELLTYPECODEC_H

#include "DataCodec/Common/DataCodecTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace datacodec {

// [DC防护:领域] cell type family/local token 的唯一公共布局契约
using CellTypeRaw = IndexType;
using CellTypeFamilyCode = IndexType;
using CellTypeLocalCode = IndexType;
using CellTypeToken = IndexType;

inline constexpr std::uint32_t kCellTypeLocalBits = 8u;
inline constexpr std::uint32_t kCellTypeFamilyShift = kCellTypeLocalBits;
inline constexpr CellTypeToken kCellTypeLocalMask =
    (CellTypeToken{1u} << kCellTypeLocalBits) - CellTypeToken{1u};
inline constexpr CellTypeFamilyCode kCellTypeMaxFamilyCode = kCellTypeLocalMask;
inline constexpr CellTypeLocalCode kCellTypeMaxLocalCode = kCellTypeLocalMask;

static_assert(std::numeric_limits<IndexType>::is_integer);
static_assert(!std::numeric_limits<IndexType>::is_signed);
static_assert(
    std::numeric_limits<IndexType>::digits >= kCellTypeFamilyShift + kCellTypeLocalBits,
    "IndexType must hold the DataCodec cell type family/local token layout");

enum class CellTypeFamily : std::uint8_t {
    FixedSize = 0,
    VariableSize = 1,
    PolynomialOrderDependent = 2,
};

enum class CodecCellTypeId : std::uint8_t {
    Vertex = 0,
    Triangle = 1,
    Quad = 2,
    Tetra = 3,
    Hexahedron = 4,
    Pyramid = 5,
    Prism = 6,
    QuadraticEdge = 7,
    QuadraticTriangle = 8,
    QuadraticQuad = 9,
    QuadraticTetra = 10,
    QuadraticHexahedron = 11,
    QuadraticPrism = 12,
    QuadraticPyramid = 13,
    Line = 14,
    Polygon = 15,
    PolyLine = 16,
    Polyhedron = 17,
    Face = 18,
    Volume = 19,
    LagrangeCurve = 20,
    LagrangeTriangle = 21,
    LagrangeQuadrilateral = 22,
    LagrangeTetrahedron = 23,
    LagrangeHexahedron = 24,
    LagrangePrism = 25,
    LagrangePyramid = 26,
    Count = 27,
};

enum class CodecCellPolynomialSizeKind : std::uint8_t {
    None = 0,
    Curve = 1,
    Triangle = 2,
    Quadrilateral = 3,
    Tetrahedron = 4,
    Hexahedron = 5,
    Prism = 6,
    Pyramid = 7,
};

inline constexpr std::size_t kCodecCellTypeCount = static_cast<std::size_t>(CodecCellTypeId::Count);

struct CodecCellTypeInfo {
    CodecCellTypeId id{CodecCellTypeId::Count};
    CellTypeFamily family{CellTypeFamily::VariableSize};
    int fixedSize{-1};
    CellTypeFamilyCode familyCode{0u};
    CellTypeLocalCode localCode{0u};
    CodecCellPolynomialSizeKind polynomialSizeKind{CodecCellPolynomialSizeKind::None};
};

inline constexpr std::array<CodecCellTypeInfo, kCodecCellTypeCount> kCodecCellTypeInfos{{
    {CodecCellTypeId::Vertex, CellTypeFamily::FixedSize, 1, 0u, 0u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::Triangle, CellTypeFamily::FixedSize, 3, 0u, 1u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::Quad, CellTypeFamily::FixedSize, 4, 0u, 2u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::Tetra, CellTypeFamily::FixedSize, 4, 0u, 3u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::Hexahedron, CellTypeFamily::FixedSize, 8, 0u, 4u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::Pyramid, CellTypeFamily::FixedSize, 5, 0u, 5u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::Prism, CellTypeFamily::FixedSize, 6, 0u, 6u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::QuadraticEdge, CellTypeFamily::FixedSize, 3, 0u, 7u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::QuadraticTriangle, CellTypeFamily::FixedSize, 6, 0u, 8u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::QuadraticQuad, CellTypeFamily::FixedSize, 8, 0u, 9u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::QuadraticTetra, CellTypeFamily::FixedSize, 10, 0u, 10u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::QuadraticHexahedron, CellTypeFamily::FixedSize, 20, 0u, 11u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::QuadraticPrism, CellTypeFamily::FixedSize, 15, 0u, 12u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::QuadraticPyramid, CellTypeFamily::FixedSize, 13, 0u, 13u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::Line, CellTypeFamily::VariableSize, -1, 1u, 0u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::Polygon, CellTypeFamily::VariableSize, -1, 1u, 1u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::PolyLine, CellTypeFamily::VariableSize, -1, 1u, 2u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::Polyhedron, CellTypeFamily::VariableSize, -1, 1u, 3u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::Face, CellTypeFamily::VariableSize, -1, 1u, 4u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::Volume, CellTypeFamily::VariableSize, -1, 1u, 5u, CodecCellPolynomialSizeKind::None},
    {CodecCellTypeId::LagrangeCurve, CellTypeFamily::PolynomialOrderDependent, -1, 2u, 0u, CodecCellPolynomialSizeKind::Curve},
    {CodecCellTypeId::LagrangeTriangle, CellTypeFamily::PolynomialOrderDependent, -1, 2u, 1u, CodecCellPolynomialSizeKind::Triangle},
    {CodecCellTypeId::LagrangeQuadrilateral, CellTypeFamily::PolynomialOrderDependent, -1, 2u, 2u, CodecCellPolynomialSizeKind::Quadrilateral},
    {CodecCellTypeId::LagrangeTetrahedron, CellTypeFamily::PolynomialOrderDependent, -1, 2u, 3u, CodecCellPolynomialSizeKind::Tetrahedron},
    {CodecCellTypeId::LagrangeHexahedron, CellTypeFamily::PolynomialOrderDependent, -1, 2u, 4u, CodecCellPolynomialSizeKind::Hexahedron},
    {CodecCellTypeId::LagrangePrism, CellTypeFamily::PolynomialOrderDependent, -1, 2u, 5u, CodecCellPolynomialSizeKind::Prism},
    {CodecCellTypeId::LagrangePyramid, CellTypeFamily::PolynomialOrderDependent, -1, 2u, 6u, CodecCellPolynomialSizeKind::Pyramid},
}};

inline constexpr bool IsCodecCellTypeIdInRange(const CodecCellTypeId id) noexcept {
    return static_cast<std::size_t>(id) < kCodecCellTypeCount;
}

inline constexpr const CodecCellTypeInfo* FindCodecCellTypeInfo(const CodecCellTypeId id) noexcept {
    for (const auto& info : kCodecCellTypeInfos) {
        if (info.id == id) {
            return &info;
        }
    }
    return nullptr;
}

inline constexpr const CodecCellTypeInfo* FindCodecCellTypeInfo(
    const CellTypeFamilyCode familyCode,
    const CellTypeLocalCode localCode) noexcept {
    for (const auto& info : kCodecCellTypeInfos) {
        if (info.familyCode == familyCode && info.localCode == localCode) {
            return &info;
        }
    }
    return nullptr;
}

inline constexpr bool IsCompleteCodecCellTypeInfoTable() noexcept {
    std::array<bool, kCodecCellTypeCount> seen{};
    for (std::size_t index = 0u; index < kCodecCellTypeInfos.size(); ++index) {
        const auto& info = kCodecCellTypeInfos[index];
        const auto typeIndex = static_cast<std::size_t>(info.id);
        if (typeIndex >= seen.size() || seen[typeIndex]) {
            return false;
        }
        if (info.familyCode > kCellTypeMaxFamilyCode || info.localCode > kCellTypeMaxLocalCode) {
            return false;
        }
        if (info.family == CellTypeFamily::FixedSize) {
            if (info.fixedSize <= 0 || info.polynomialSizeKind != CodecCellPolynomialSizeKind::None) {
                return false;
            }
        } else if (info.family == CellTypeFamily::VariableSize) {
            if (info.fixedSize != -1 || info.polynomialSizeKind != CodecCellPolynomialSizeKind::None) {
                return false;
            }
        } else if (info.family == CellTypeFamily::PolynomialOrderDependent) {
            if (info.fixedSize != -1 || info.polynomialSizeKind == CodecCellPolynomialSizeKind::None) {
                return false;
            }
        } else {
            return false;
        }
        for (std::size_t otherIndex = index + 1u; otherIndex < kCodecCellTypeInfos.size(); ++otherIndex) {
            const auto& other = kCodecCellTypeInfos[otherIndex];
            if (info.familyCode == other.familyCode && info.localCode == other.localCode) {
                return false;
            }
        }
        seen[typeIndex] = true;
    }
    for (const auto value : seen) {
        if (!value) {
            return false;
        }
    }
    return true;
}

static_assert(
    IsCompleteCodecCellTypeInfoTable(),
    "DataCodec cell type info table must cover every CodecCellTypeId exactly once");

inline constexpr bool ResolveCodecCellSizeFromPolynomialOrder(
    const CodecCellTypeId type,
    const std::uint16_t order,
    int& size) noexcept {
    size = -1;
    if (order == 0u) {
        return false;
    }

    const auto* info = FindCodecCellTypeInfo(type);
    if (info == nullptr || info->family != CellTypeFamily::PolynomialOrderDependent) {
        return false;
    }

    const auto p = static_cast<std::uint64_t>(order);
    switch (info->polynomialSizeKind) {
        case CodecCellPolynomialSizeKind::Curve:
            size = static_cast<int>(p + 1u);
            return true;
        case CodecCellPolynomialSizeKind::Triangle:
            size = static_cast<int>((p + 1u) * (p + 2u) / 2u);
            return true;
        case CodecCellPolynomialSizeKind::Quadrilateral:
            size = static_cast<int>((p + 1u) * (p + 1u));
            return true;
        case CodecCellPolynomialSizeKind::Tetrahedron:
            size = static_cast<int>((p + 1u) * (p + 2u) * (p + 3u) / 6u);
            return true;
        case CodecCellPolynomialSizeKind::Hexahedron:
            size = static_cast<int>((p + 1u) * (p + 1u) * (p + 1u));
            return true;
        case CodecCellPolynomialSizeKind::Prism:
            size = static_cast<int>((p + 1u) * (p + 1u) * (p + 2u) / 2u);
            return true;
        case CodecCellPolynomialSizeKind::Pyramid:
            size = static_cast<int>((p + 1u) * (p + 2u) * (2u * p + 3u) / 6u);
            return true;
        case CodecCellPolynomialSizeKind::None:
            break;
    }

    return false;
}

struct CellTypeCodecEntry {
    CellTypeRaw rawType{0};
    CellTypeRaw normalizedType{0};
    CellTypeFamily family{CellTypeFamily::VariableSize};
    int fixedSize{-1};
};

struct ICellTypeCodec {
    virtual ~ICellTypeCodec() = default;

    virtual bool ResolveCellType(CellTypeRaw rawType, CellTypeCodecEntry& entry) const = 0;
    virtual bool ResolveCellSizeFromPolynomialOrder(CellTypeRaw rawType, std::uint16_t order, int& size) const = 0;
    virtual bool EncodeCellTypeFamilyLocal(
        CellTypeRaw rawType,
        CellTypeFamilyCode& familyCode,
        CellTypeLocalCode& familyLocalCode) const {
        (void)rawType;
        familyCode = 0u;
        familyLocalCode = 0u;
        return false;
    }
    virtual bool DecodeCellTypeFamilyLocal(
        CellTypeFamilyCode familyCode,
        CellTypeLocalCode familyLocalCode,
        CellTypeRaw& rawType) const {
        (void)familyCode;
        (void)familyLocalCode;
        rawType = 0u;
        return false;
    }
    virtual bool EncodeCellPolynomialOrderLocal(
        CellTypeRaw rawType,
        std::uint16_t order,
        CellTypeLocalCode& localOrder) const {
        (void)rawType;
        (void)order;
        localOrder = 0u;
        return false;
    }
    virtual bool DecodeCellPolynomialOrderLocal(
        CellTypeRaw rawType,
        CellTypeLocalCode localOrder,
        std::uint16_t& order) const {
        (void)rawType;
        (void)localOrder;
        order = 0u;
        return false;
    }
};

inline constexpr bool IsCellTypeFamilyLocalCodePackable(
    const CellTypeFamilyCode familyCode,
    const CellTypeLocalCode localCode) noexcept {
    return familyCode <= kCellTypeMaxFamilyCode && localCode <= kCellTypeMaxLocalCode;
}

inline constexpr CellTypeToken PackCellTypeFamilyLocalToken(
    const CellTypeFamilyCode familyCode,
    const CellTypeLocalCode localCode) noexcept {
    return static_cast<CellTypeToken>(
        (static_cast<CellTypeToken>(familyCode) << kCellTypeFamilyShift) |
        static_cast<CellTypeToken>(localCode));
}

inline constexpr bool TryPackCellTypeFamilyLocalToken(
    const CellTypeFamilyCode familyCode,
    const CellTypeLocalCode localCode,
    CellTypeToken& token) noexcept {
    if (!IsCellTypeFamilyLocalCodePackable(familyCode, localCode)) {
        token = 0u;
        return false;
    }
    token = PackCellTypeFamilyLocalToken(familyCode, localCode);
    return true;
}

inline constexpr void UnpackCellTypeFamilyLocalToken(
    const CellTypeToken token,
    CellTypeFamilyCode& familyCode,
    CellTypeLocalCode& localCode) noexcept {
    familyCode = static_cast<CellTypeFamilyCode>(
        (static_cast<CellTypeToken>(token) >> kCellTypeFamilyShift) & kCellTypeLocalMask);
    localCode = static_cast<CellTypeLocalCode>(
        static_cast<CellTypeToken>(token) & kCellTypeLocalMask);
}

inline constexpr bool CellTypeFamilyHasImplicitSize(const CellTypeFamily family) {
    return family != CellTypeFamily::VariableSize;
}

inline constexpr bool CellTypeFamilyNeedsExplicitPolynomialOrder(const CellTypeFamily family) {
    return family == CellTypeFamily::PolynomialOrderDependent;
}

} // namespace datacodec

#endif
