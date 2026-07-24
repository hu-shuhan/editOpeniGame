#ifndef DATACODEC_API_ADAPTER_ICELLTYPEMAPPING_H
#define DATACODEC_API_ADAPTER_ICELLTYPEMAPPING_H

#include "DataCodec/Codec/Topology/Common/CellTypeCodec.h"

#include <array>
#include <cstddef>
#include <cstdint>
namespace datacodec {

enum class CellTypeMappingMode : std::uint8_t {
    FamilyLocal = 0,
};

struct CellTypeNativeMappingEntry {
    CodecCellTypeId codecType{CodecCellTypeId::Count};
    CellTypeRaw rawType{0u};
};

template<std::size_t N>
inline constexpr const CellTypeNativeMappingEntry* FindCellTypeMappingByCodecType(
    const std::array<CellTypeNativeMappingEntry, N>& mappings,
    const CodecCellTypeId codecType) noexcept {
    for (const auto& mapping : mappings) {
        if (mapping.codecType == codecType) {
            return &mapping;
        }
    }
    return nullptr;
}

template<std::size_t N>
inline constexpr const CellTypeNativeMappingEntry* FindCellTypeMappingByRawType(
    const std::array<CellTypeNativeMappingEntry, N>& mappings,
    const CellTypeRaw rawType) noexcept {
    for (const auto& mapping : mappings) {
        if (mapping.rawType == rawType) {
            return &mapping;
        }
    }
    return nullptr;
}

template<std::size_t N>
inline constexpr bool IsCompleteCellTypeMapping(
    const std::array<CellTypeNativeMappingEntry, N>& mappings) noexcept {
    if constexpr (N != kCodecCellTypeCount) {
        return false;
    }

    std::array<bool, kCodecCellTypeCount> seen{};
    for (std::size_t index = 0u; index < mappings.size(); ++index) {
        const auto& mapping = mappings[index];
        const auto typeIndex = static_cast<std::size_t>(mapping.codecType);
        if (typeIndex >= seen.size() || seen[typeIndex]) {
            return false;
        }
        if (FindCodecCellTypeInfo(mapping.codecType) == nullptr) {
            return false;
        }
        for (std::size_t otherIndex = index + 1u; otherIndex < mappings.size(); ++otherIndex) {
            if (mapping.rawType == mappings[otherIndex].rawType) {
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

template<std::size_t N>
inline bool ResolveMappedCellType(
    const std::array<CellTypeNativeMappingEntry, N>& mappings,
    const CellTypeRaw rawType,
    CellTypeCodecEntry& entry) {
    entry = {};
    const auto* mapping = FindCellTypeMappingByRawType(mappings, rawType);
    if (mapping == nullptr) {
        return false;
    }

    const auto* info = FindCodecCellTypeInfo(mapping->codecType);
    if (info == nullptr) {
        return false;
    }

    entry.rawType = rawType;
    entry.normalizedType = mapping->rawType;
    entry.family = info->family;
    entry.fixedSize = info->fixedSize;
    return true;
}

template<std::size_t N>
inline bool ResolveMappedCellSizeFromPolynomialOrder(
    const std::array<CellTypeNativeMappingEntry, N>& mappings,
    const CellTypeRaw rawType,
    const std::uint16_t order,
    int& size) {
    size = -1;
    const auto* mapping = FindCellTypeMappingByRawType(mappings, rawType);
    if (mapping == nullptr) {
        return false;
    }
    return ResolveCodecCellSizeFromPolynomialOrder(mapping->codecType, order, size);
}

template<std::size_t N>
inline bool EncodeMappedCellTypeFamilyLocal(
    const std::array<CellTypeNativeMappingEntry, N>& mappings,
    const CellTypeRaw rawType,
    CellTypeFamilyCode& familyCode,
    CellTypeLocalCode& familyLocalCode) {
    familyCode = 0u;
    familyLocalCode = 0u;
    const auto* mapping = FindCellTypeMappingByRawType(mappings, rawType);
    if (mapping == nullptr) {
        return false;
    }

    const auto* info = FindCodecCellTypeInfo(mapping->codecType);
    if (info == nullptr) {
        return false;
    }

    familyCode = info->familyCode;
    familyLocalCode = info->localCode;
    return true;
}

template<std::size_t N>
inline bool DecodeMappedCellTypeFamilyLocal(
    const std::array<CellTypeNativeMappingEntry, N>& mappings,
    const CellTypeFamilyCode familyCode,
    const CellTypeLocalCode familyLocalCode,
    CellTypeRaw& rawType) {
    rawType = 0u;
    const auto* info = FindCodecCellTypeInfo(familyCode, familyLocalCode);
    if (info == nullptr) {
        return false;
    }

    const auto* mapping = FindCellTypeMappingByCodecType(mappings, info->id);
    if (mapping == nullptr) {
        return false;
    }

    rawType = mapping->rawType;
    return true;
}

struct ICellTypeMapping : public ICellTypeCodec {
    virtual ~ICellTypeMapping() = default;

    // 返回 adapter 使用的原生 cell type 到 DataCodec token 的映射模式
    virtual CellTypeMappingMode GetCellTypeMappingMode() const = 0;
};

} // namespace datacodec

#endif
