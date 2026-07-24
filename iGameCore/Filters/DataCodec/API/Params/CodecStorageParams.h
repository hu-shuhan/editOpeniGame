#ifndef DATACODEC_API_PARAMS_CODECSTORAGEPARAMS_H
#define DATACODEC_API_PARAMS_CODECSTORAGEPARAMS_H

#include "DataCodec/API/Params/NumericArrayParams.h"
#include "DataCodec/API/Params/ParamSerialization.h"
#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <numeric>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace datacodec {

struct StructuredMeshStorageParams {
    // 结构化网格的 X/Y/Z 逻辑轴尺寸
    std::array<std::int32_t, 3> axisSize{0, 0, 0};

    template<class Archive>
    void serialize(Archive& archive) {
        archive(axisSize);
    }
};

struct SpatialBlockStorageParams {
    // Point 与 Cell 的空间块边界由元素总量和固定块大小唯一确定
    std::uint32_t pointElementCount{262144u};
    std::uint32_t cellElementCount{262144u};
    ParamSize pointElementTotal{0u};
    ParamSize cellElementTotal{0u};
    std::uint32_t pointBlockCount{0u};
    std::uint32_t cellBlockCount{0u};

    [[nodiscard]] std::uint32_t ElementCount(
        const AttrAttachment attachment) const noexcept {
        return attachment == AttrAttachment::Point
            ? pointElementCount
            : cellElementCount;
    }

    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            pointElementCount,
            cellElementCount,
            pointElementTotal,
            cellElementTotal,
            pointBlockCount,
            cellBlockCount);
    }
};

struct GeometryStorageParams : public NumericArrayStorageParams {
    template<class Archive>
    void save(Archive& archive) const {
        archive(cereal::base_class<const NumericArrayStorageParams>(this));
    }

    template<class Archive>
    void load(Archive& archive) {
        archive(cereal::base_class<NumericArrayStorageParams>(this));
    }
};

struct AttrDecodeScheduleHint {
    ParamSize estimatedDecodeCost{0u};
    ParamSize criticalPathCost{0u};
    AttributeDecodeScheduleClass scheduleClass{AttributeDecodeScheduleClass::Unknown};

    template<class Archive>
    void save(Archive& archive) const {
        archive(estimatedDecodeCost, criticalPathCost);
        detail::SaveEnum(archive, scheduleClass);
    }

    template<class Archive>
    void load(Archive& archive) {
        archive(estimatedDecodeCost, criticalPathCost);
        detail::LoadEnum(archive, scheduleClass);
    }
};

struct AttrStorageParams : public NumericArrayStorageParams {
    // 源网格中的原始属性名
    std::string name;
    // 源属性的语义角色
    AttrRole type{AttrRole::Unknown};
    // 该属性附着在 point 还是 cell 上
    AttrAttachment attachmentType{AttrAttachment::Point};
    // 该属性 section 最终编码后的字节数
    ParamSize binaryCount{0};
    AttrDecodeScheduleHint decodeScheduleHint;

    template<class Archive>
    void save(Archive& archive) const {
        archive(
            cereal::base_class<const NumericArrayStorageParams>(this),
            name,
            binaryCount,
            decodeScheduleHint);
        detail::SaveEnum(archive, type);
        detail::SaveEnum(archive, attachmentType);
    }

    template<class Archive>
    void load(Archive& archive) {
        archive(
            cereal::base_class<NumericArrayStorageParams>(this),
            name,
            binaryCount,
            decodeScheduleHint);
        detail::LoadEnum(archive, type);
        detail::LoadEnum(archive, attachmentType);
    }
};

struct TopologyConnectivityBlockLayoutParams {
    ParamSize cellOffset{0u};
    ParamSize cellCount{0u};
    ParamSize connectivityOffset{0u};
    ParamSize connectivityCount{0u};
    ParamSize connectivityByteCount{0u};
    ParamSize cellSizeByteCount{0u};
    ParamSize cellPolynomialOrderByteCount{0u};
    ParamSize cellTypeByteCount{0u};

    [[nodiscard]] ParamSize PayloadByteCount() const noexcept {
        return connectivityByteCount +
            cellSizeByteCount +
            cellPolynomialOrderByteCount +
            cellTypeByteCount;
    }

    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            cellOffset,
            cellCount,
            connectivityOffset,
            connectivityCount,
            connectivityByteCount,
            cellSizeByteCount,
            cellPolynomialOrderByteCount,
            cellTypeByteCount);
    }
};

struct TopologyConnectivityLayoutParams {
    ParamSize connectivityByteCount{0};
    ParamSize cellSizeByteCount{0};
    ParamSize cellPolynomialOrderByteCount{0};
    ParamSize cellTypeByteCount{0};
    std::vector<TopologyConnectivityBlockLayoutParams> blockLayouts;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            connectivityByteCount,
            cellSizeByteCount,
            cellPolynomialOrderByteCount,
            cellTypeByteCount,
            blockLayouts);
    }
};

struct TopologyStreamLayoutParams {
    std::uint32_t kind{0};
    std::uint32_t codec{0};
    std::uint32_t flags{0};
    std::uint32_t meshIndexPadding{0};
    ParamSize elementCount{0};
    ParamSize auxiliaryByteSize{0};
    ParamSize encodedByteLength{0};

    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            kind,
            codec,
            flags,
            meshIndexPadding,
            elementCount,
            auxiliaryByteSize,
            encodedByteLength);
    }
};

struct TopoStorageParams {
    // 解码时用来重建拓扑模式的标记
    std::uint8_t isStructured{0};
    std::uint8_t isPolyhedron{0};
    std::uint8_t hasCellTypes{0};
    // 非 polyhedron 网格的通用拓扑规模信息
    std::int32_t fixedCellSize{0};
    ParamSize cellCount{0};
    ParamSize cellBufferSize{0};
    ParamSize cellTypeCount{0};
    // 多面体专用的规模汇总，用于校验和分配
    ParamSize polyhedronVertexCount{0};
    ParamSize polyhedronFaceVertexCount{0};
    ParamSize polyhedronStreamCount{0};
    ParamSize binaryCount{0};
    TopologyConnectivityLayoutParams connectivityLayout;
    std::vector<TopologyStreamLayoutParams> polyhedronStreamLayouts;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(
            isStructured,
            isPolyhedron,
            hasCellTypes,
            fixedCellSize,
            cellCount,
            cellBufferSize,
            cellTypeCount,
            polyhedronVertexCount,
            polyhedronFaceVertexCount,
            polyhedronStreamCount,
            binaryCount,
            connectivityLayout,
            polyhedronStreamLayouts);
    }
};

struct CodecStorageParams {
    // 编解码两侧统一使用的网格类别
    MeshType meshType{MeshType::PointSet};
    // Geometry、Topology、Attribute 和 Key Frame 共享的空间块布局
    SpatialBlockStorageParams spatialBlockParams;
    // 写入 params 的 structured、geometry、topology 和 attribute 元数据
    StructuredMeshStorageParams structuredMeshParams;
    GeometryStorageParams geomParams;
    TopoStorageParams topoParams;
    std::vector<AttrStorageParams> attrParams;
    // 编码侧运行期映射，元素值指向源 adapter 内同 attachment 的 attribute 下标
    std::vector<std::size_t> attrSourceIndices;
    // attribute section 中 payload 的读取顺序，元素值指向 attrParams 下标
    std::vector<ParamSize> attrPayloadOrder;

    template<class Archive>
    void save(Archive& archive) const {
        detail::SaveEnum(archive, meshType);
        archive(
            spatialBlockParams,
            structuredMeshParams,
            geomParams,
            topoParams,
            attrParams,
            attrPayloadOrder);
    }

    template<class Archive>
    void load(Archive& archive) {
        detail::LoadEnum(archive, meshType);
        archive(
            spatialBlockParams,
            structuredMeshParams,
            geomParams,
            topoParams,
            attrParams,
            attrPayloadOrder);
        attrSourceIndices.clear();
    }
};

inline std::vector<ParamSize> MakeDefaultAttributePayloadOrder(const std::size_t attrCount) {
    std::vector<ParamSize> order(attrCount);
    std::iota(order.begin(), order.end(), 0u);
    return order;
}

inline std::size_t CountAttrStorageParams(
    const CodecStorageParams& params,
    const AttrAttachment attachment) noexcept {
    std::size_t count = 0u;
    for (const auto& meta : params.attrParams) {
        if (meta.attachmentType == attachment) {
            ++count;
        }
    }
    return count;
}

inline std::size_t ResolveAttrMetaIndex(
    const CodecStorageParams& params,
    const AttrAttachment attachment,
    const std::size_t localAttrIndex) noexcept {
    std::size_t localIndex = 0u;
    for (std::size_t metaIndex = 0u; metaIndex < params.attrParams.size(); ++metaIndex) {
        if (params.attrParams[metaIndex].attachmentType != attachment) {
            continue;
        }
        if (localIndex == localAttrIndex) {
            return metaIndex;
        }
        ++localIndex;
    }
    return kInvalidTransferCacheIndex;
}

inline std::size_t ResolveAttrLocalIndex(
    const CodecStorageParams& params,
    const std::size_t metaIndex) noexcept {
    if (metaIndex >= params.attrParams.size()) {
        return kInvalidTransferCacheIndex;
    }
    const auto attachment = params.attrParams[metaIndex].attachmentType;
    std::size_t localIndex = 0u;
    for (std::size_t index = 0u; index < metaIndex; ++index) {
        if (params.attrParams[index].attachmentType == attachment) {
            ++localIndex;
        }
    }
    return localIndex;
}

inline std::size_t ResolveAttrSourceIndex(
    const CodecStorageParams& params,
    const std::size_t metaIndex) noexcept {
    if (metaIndex >= params.attrParams.size()) {
        return kInvalidTransferCacheIndex;
    }
    if (params.attrSourceIndices.size() == params.attrParams.size()) {
        return params.attrSourceIndices[metaIndex];
    }
    return ResolveAttrLocalIndex(params, metaIndex);
}

inline bool ResolveAttributePayloadOrder(
    const CodecStorageParams& params,
    std::vector<std::size_t>& order,
    std::string* error = nullptr) {
    order.clear();
    if (params.attrPayloadOrder.size() != params.attrParams.size()) {
        return validation::AssignError(error, "attribute payload order size does not match attribute params");
    }
    order.reserve(params.attrPayloadOrder.size());
    std::vector<std::uint8_t> seen(params.attrPayloadOrder.size(), 0u);
    for (const auto attrIndex : params.attrPayloadOrder) {
        if (attrIndex > static_cast<ParamSize>(std::numeric_limits<std::size_t>::max())) {
            return validation::AssignError(error, "attribute payload order index exceeds this platform size limit");
        }
        const auto localIndex = static_cast<std::size_t>(attrIndex);
        if (localIndex >= params.attrPayloadOrder.size() || seen[localIndex] != 0u) {
            return validation::AssignError(error, "attribute payload order is not a valid permutation");
        }
        seen[localIndex] = 1u;
        order.push_back(localIndex);
    }
    return true;
}

inline ParamSize SaturatingParamSizeAdd(const ParamSize left, const ParamSize right) noexcept {
    if (right > std::numeric_limits<ParamSize>::max() - left) {
        return std::numeric_limits<ParamSize>::max();
    }
    return left + right;
}

inline ParamSize SaturatingParamSizeMultiply(const ParamSize left, const ParamSize right) noexcept {
    if (left != 0u && right > std::numeric_limits<ParamSize>::max() / left) {
        return std::numeric_limits<ParamSize>::max();
    }
    return left * right;
}

inline ParamSize NumericArrayRawValueBytes(const NumericArrayStorageParams& meta) noexcept {
    const auto componentCount = static_cast<ParamSize>(std::max(meta.dimension, 0));
    return SaturatingParamSizeMultiply(
        SaturatingParamSizeMultiply(meta.elementCount, componentCount),
        meta.valueSize);
}

inline ParamSize EstimateAttributeDecodeCost(const AttrStorageParams& meta) noexcept {
    const auto rawBytes = NumericArrayRawValueBytes(meta);
    ParamSize encodedBlockBytes = 0u;
    std::size_t componentLayoutCount = 0u;
    std::size_t regionLayerCount = 0u;
    bool hasReferenceBlock = false;
    bool hasWaveletBlock = false;
    bool hasPredictorBlock = false;
    bool hasLayeredResidualBlock = false;

    for (const auto& layout : meta.blockLayouts) {
        encodedBlockBytes = SaturatingParamSizeAdd(encodedBlockBytes, layout.encodedByteLength);
        encodedBlockBytes = SaturatingParamSizeAdd(encodedBlockBytes, layout.backgroundEncodedByteLength);
        componentLayoutCount += layout.componentLayouts.size();
        regionLayerCount += layout.regionLayers.size();
        hasReferenceBlock = hasReferenceBlock || layout.referenceKind != NumericArrayReferenceKind::None;
        hasWaveletBlock = hasWaveletBlock || layout.mode == NumericArrayBlockMode::WaveletReference;
        hasPredictorBlock = hasPredictorBlock || layout.mode == NumericArrayBlockMode::PredictorReference;
        hasLayeredResidualBlock = hasLayeredResidualBlock || layout.mode == NumericArrayBlockMode::LayeredResidual;
    }

    ParamSize cost = meta.binaryCount;
    cost = SaturatingParamSizeAdd(cost, encodedBlockBytes / 2u);
    cost = SaturatingParamSizeAdd(cost, rawBytes / 4u);
    cost = SaturatingParamSizeAdd(cost, static_cast<ParamSize>(meta.blockLayouts.size()) * 256u * 1024u);
    cost = SaturatingParamSizeAdd(cost, static_cast<ParamSize>(componentLayoutCount) * 128u * 1024u);
    cost = SaturatingParamSizeAdd(cost, static_cast<ParamSize>(regionLayerCount) * 512u * 1024u);
    if (hasReferenceBlock) {
        cost = SaturatingParamSizeAdd(cost, meta.binaryCount / 2u);
        cost = SaturatingParamSizeAdd(cost, rawBytes / 8u);
    }
    if (hasWaveletBlock || hasPredictorBlock) {
        cost = SaturatingParamSizeAdd(cost, rawBytes / 8u);
    }
    if (hasLayeredResidualBlock) {
        cost = SaturatingParamSizeAdd(cost, meta.binaryCount / 4u);
    }
    if (cost == 0u && rawBytes != 0u) {
        cost = rawBytes;
    }
    return cost;
}

inline bool AttributeHasReferenceBlocks(const AttrStorageParams& meta) noexcept {
    for (const auto& layout : meta.blockLayouts) {
        if (layout.referenceKind != NumericArrayReferenceKind::None) {
            return true;
        }
    }
    return false;
}

inline AttributeDecodeScheduleClass ResolveAttributeDecodeScheduleClass(
    const AttrStorageParams& meta,
    const bool hasDependent) noexcept {
    const auto hasReference = AttributeHasReferenceBlocks(meta);
    if (hasDependent && hasReference) {
        return AttributeDecodeScheduleClass::ReferenceChain;
    }
    if (hasDependent) {
        return AttributeDecodeScheduleClass::ReferenceParent;
    }
    if (hasReference) {
        return AttributeDecodeScheduleClass::ReferenceChild;
    }
    constexpr ParamSize kLargeIndependentCost = 32u * 1024u * 1024u;
    return EstimateAttributeDecodeCost(meta) >= kLargeIndependentCost
        ? AttributeDecodeScheduleClass::LargeIndependent
        : AttributeDecodeScheduleClass::SmallField;
}

inline void RefreshAttributeDecodeScheduleHints(CodecStorageParams& params) {
    const auto attrCount = params.attrParams.size();
    std::vector<std::vector<std::size_t>> dependents(attrCount);
    for (std::size_t attrIndex = 0u; attrIndex < attrCount; ++attrIndex) {
        for (const auto& layout : params.attrParams[attrIndex].blockLayouts) {
            if (layout.referenceKind != NumericArrayReferenceKind::IntraArray) {
                continue;
            }
            const auto parentIndex = static_cast<std::size_t>(layout.localParentFieldIndex);
            if (parentIndex < attrCount && parentIndex != attrIndex) {
                dependents[parentIndex].push_back(attrIndex);
            }
        }
    }

    std::vector<ParamSize> ownCosts(attrCount, 0u);
    std::vector<ParamSize> criticalCosts(attrCount, 0u);
    std::vector<std::uint8_t> visitState(attrCount, 0u);
    for (std::size_t attrIndex = 0u; attrIndex < attrCount; ++attrIndex) {
        ownCosts[attrIndex] = EstimateAttributeDecodeCost(params.attrParams[attrIndex]);
    }

    std::function<ParamSize(std::size_t)> resolveCriticalCost = [&](const std::size_t attrIndex) -> ParamSize {
        if (attrIndex >= attrCount) {
            return 0u;
        }
        if (visitState[attrIndex] == 2u) {
            return criticalCosts[attrIndex];
        }
        if (visitState[attrIndex] == 1u) {
            return ownCosts[attrIndex];
        }
        visitState[attrIndex] = 1u;
        ParamSize maxChildCost = 0u;
        for (const auto childIndex : dependents[attrIndex]) {
            maxChildCost = std::max(maxChildCost, resolveCriticalCost(childIndex));
        }
        criticalCosts[attrIndex] = SaturatingParamSizeAdd(ownCosts[attrIndex], maxChildCost);
        visitState[attrIndex] = 2u;
        return criticalCosts[attrIndex];
    };

    for (std::size_t attrIndex = 0u; attrIndex < attrCount; ++attrIndex) {
        params.attrParams[attrIndex].decodeScheduleHint.estimatedDecodeCost = ownCosts[attrIndex];
        params.attrParams[attrIndex].decodeScheduleHint.criticalPathCost = resolveCriticalCost(attrIndex);
        params.attrParams[attrIndex].decodeScheduleHint.scheduleClass =
            ResolveAttributeDecodeScheduleClass(params.attrParams[attrIndex], !dependents[attrIndex].empty());
    }
}

inline constexpr std::uint32_t kCodecStorageParamsMagic = 0x50434744u;
inline constexpr std::uint32_t kCodecStorageParamsVersion = 1u;

struct CodecStorageParamsHeader {
    // DataCodec params 魔数: DGCP
    std::uint32_t magic{kCodecStorageParamsMagic};
    std::uint32_t flags{0u};
    std::uint32_t version{kCodecStorageParamsVersion};

    static constexpr std::uint32_t kRequires64BitSize = 1u << 0;
    static constexpr std::uint32_t kKnownFlags = kRequires64BitSize;

    void SetRequires64BitSize(const bool value) noexcept {
        if (value) {
            flags |= kRequires64BitSize;
        } else {
            flags &= ~kRequires64BitSize;
        }
    }

    [[nodiscard]] bool Requires64BitSize() const noexcept {
        return (flags & kRequires64BitSize) != 0u;
    }

    template<class Archive>
    void serialize(Archive& archive) {
        archive(magic, flags, version);
    }
};

inline bool SetParamsError(std::string* error, const std::string_view message) {
    return validation::AssignError(error, std::string(message));
}

inline bool SetParamsFieldError(
    std::string* error,
    const std::string_view fieldName,
    const std::string_view message) {
    return validation::AssignError(error, std::string(fieldName) + " " + std::string(message));
}

inline bool IsBooleanParamFlag(const std::uint8_t value) noexcept {
    return value == 0u || value == 1u;
}

inline void MarkRequires64Bit(const ParamSize value, bool& requires64Bit) noexcept {
    if (value > static_cast<ParamSize>(std::numeric_limits<std::uint32_t>::max())) {
        requires64Bit = true;
    }
}

inline bool CheckedParamSizeAdd(
    const ParamSize left,
    const ParamSize right,
    ParamSize& output,
    std::string* error,
    const std::string_view fieldName) {
    return validation::CheckedAddU64(left, right, output, fieldName, error);
}

inline bool CheckedParamSizeMultiply(
    const ParamSize left,
    const ParamSize right,
    ParamSize& output,
    std::string* error,
    const std::string_view fieldName) {
    return validation::CheckedMulU64(left, right, output, fieldName, error);
}

inline bool CheckedSizeT(
    const ParamSize value,
    const std::string_view fieldName,
    std::size_t& output,
    std::string* error) {
    if (value > static_cast<ParamSize>(std::numeric_limits<std::size_t>::max())) {
        return validation::AssignError(
            error,
            "encoded DataCodec params require a 64-bit decoder: " +
                std::string(fieldName) +
                " exceeds this platform size limit");
    }
    output = static_cast<std::size_t>(value);
    return true;
}

inline bool ValidateMeshTypeForParams(const MeshType value) noexcept {
    switch (value) {
        case MeshType::PointSet:
        case MeshType::SurfaceMesh:
        case MeshType::VolumeMesh:
        case MeshType::StructuredMesh:
        case MeshType::UnstructuredMesh:
        case MeshType::PolyhedronMesh:
            return true;
    }
    return false;
}

inline bool ValidateEncodedFieldCodecTypeForParams(const EncodedFieldCodecType value) noexcept {
    switch (value) {
        case EncodedFieldCodecType::Unknown:
        case EncodedFieldCodecType::Params:
        case EncodedFieldCodecType::Value:
        case EncodedFieldCodecType::Topology:
        case EncodedFieldCodecType::Raw:
        case EncodedFieldCodecType::Delta:
        case EncodedFieldCodecType::NumericArrayBlocks:
            return true;
    }
    return false;
}

inline bool ValidateNumericArrayBlockModeForParams(const NumericArrayBlockMode value) noexcept {
    switch (value) {
        case NumericArrayBlockMode::NonReference:
        case NumericArrayBlockMode::AffineReference:
        case NumericArrayBlockMode::WaveletReference:
        case NumericArrayBlockMode::PredictorReference:
        case NumericArrayBlockMode::LayeredResidual:
            return true;
    }
    return false;
}

inline bool ValidateNumericArrayBytesCodecForParams(const NumericArrayBytesCodec value) noexcept {
    switch (value) {
        case NumericArrayBytesCodec::RawBytes:
        case NumericArrayBytesCodec::NumericArrayCodec:
        case NumericArrayBytesCodec::IntegerDeltaRunVarint:
        case NumericArrayBytesCodec::IntegerDeltaLiteralRunVarint:
            return true;
    }
    return false;
}

inline bool ValidateAttrRoleForParams(const AttrRole value) noexcept {
    switch (value) {
        case AttrRole::Scalar:
        case AttrRole::Vector:
        case AttrRole::Normal:
        case AttrRole::TexCoord:
        case AttrRole::Tensor:
        case AttrRole::Color:
        case AttrRole::Unknown:
            return true;
    }
    return false;
}

inline bool ValidateAttrAttachmentForParams(const AttrAttachment value) noexcept {
    switch (value) {
        case AttrAttachment::Point:
        case AttrAttachment::Cell:
            return true;
    }
    return false;
}

inline bool ValidateAttributeDecodeScheduleClassForParams(const AttributeDecodeScheduleClass value) noexcept {
    switch (value) {
        case AttributeDecodeScheduleClass::Unknown:
        case AttributeDecodeScheduleClass::SmallField:
        case AttributeDecodeScheduleClass::LargeIndependent:
        case AttributeDecodeScheduleClass::ReferenceParent:
        case AttributeDecodeScheduleClass::ReferenceChild:
        case AttributeDecodeScheduleClass::ReferenceChain:
            return true;
    }
    return false;
}

inline bool ValidateCompressorConfigForParams(
    const CompressorConfig& config,
    const std::string_view fieldName,
    bool& requires64Bit,
    std::string* error) {
    MarkRequires64Bit(static_cast<ParamSize>(config.options.size()), requires64Bit);
    const bool hasAbsoluteError = config.options.contains("pressio:abs");
    const bool hasRelativeError = config.options.contains("pressio:rel");
    if (hasAbsoluteError && hasRelativeError) {
        return SetParamsFieldError(
            error,
            fieldName,
            "compressor cannot define both absolute and relative error bounds");
    }
    for (const auto& option : config.options) {
        MarkRequires64Bit(static_cast<ParamSize>(option.first.size()), requires64Bit);
        if ((option.first == "pressio:abs" || option.first == "pressio:rel") &&
            (!std::isfinite(option.second) || option.second < 0.0)) {
            return SetParamsFieldError(
                error,
                fieldName,
                "compressor error bound must be finite and non-negative");
        }
    }
    return true;
}

inline bool ValidateNumericArrayRegionControlForParams(
    const NumericArrayRegionControlParams& control,
    const std::string_view fieldName,
    bool& requires64Bit,
    std::string* error) {
    MarkRequires64Bit(static_cast<ParamSize>(control.regions.size()), requires64Bit);
    MarkRequires64Bit(static_cast<ParamSize>(control.defaultPrecision.name.size()), requires64Bit);
    if (!control.defaultPrecision.hasCompressor) {
        return SetParamsFieldError(error, fieldName, "region control is missing default precision compressor");
    }
    if (!ValidateCompressorConfigForParams(
            control.defaultPrecision.compressor,
            std::string(fieldName) + ".defaultPrecision.compressor",
            requires64Bit,
            error)) {
        return false;
    }
    if (control.regions.size() > control.runPolicy.maxRegionCount) {
        return SetParamsFieldError(error, fieldName, "region precision count exceeds maxRegionCount");
    }
    if (control.runPolicy.maxRunsPerRegion == 0u) {
        return SetParamsFieldError(error, fieldName, "maxRunsPerRegion is zero");
    }
    if (control.runPolicy.maxRefinedElementRatio < 0.0 ||
        control.runPolicy.maxRefinedElementRatio < 0.0 ||
        control.runPolicy.maxFragmentElementRatio < 0.0 ||
        control.runPolicy.maxFragmentElementRatio > 1.0 ||
        control.runPolicy.maxExpansionRatio < 0.0 ||
        control.runPolicy.maxExpansionRatio > 1.0) {
        return SetParamsFieldError(error, fieldName, "region ratio policy is invalid");
    }
    for (std::size_t index = 0u; index < control.regions.size(); ++index) {
        const auto& region = control.regions[index];
        MarkRequires64Bit(static_cast<ParamSize>(region.name.size()), requires64Bit);
        if (!region.hasCompressor) {
            return SetParamsFieldError(
                error,
                std::string(fieldName) + ".regions[" + std::to_string(index) + "]",
                "is missing compressor");
        }
        if (!ValidateCompressorConfigForParams(
                region.compressor,
                std::string(fieldName) + ".regions[" + std::to_string(index) + "].compressor",
                requires64Bit,
                error)) {
            return false;
        }
    }
    return true;
}

inline bool ValidateNumericArrayComponentLayoutsForParams(
    const std::span<const NumericArrayComponentLayoutParams> componentLayouts,
    const std::size_t componentCount,
    ParamSize& componentPayloadBytes,
    const std::string_view fieldName,
    bool& requires64Bit,
    std::string* error) {
    componentPayloadBytes = 0u;
    if (componentLayouts.size() != componentCount) {
        return SetParamsFieldError(error, fieldName, "component layout count does not match dimension");
    }
    std::vector<std::uint8_t> seen(componentCount, 0u);
    for (const auto& component : componentLayouts) {
        if (!ValidateNumericArrayBytesCodecForParams(component.bytesCodec)) {
            return SetParamsFieldError(error, fieldName, "component bytes codec is invalid");
        }
        if (component.componentIndex >= componentCount || seen[component.componentIndex] != 0u) {
            return SetParamsFieldError(error, fieldName, "component layout index is invalid");
        }
        seen[component.componentIndex] = 1u;
        MarkRequires64Bit(component.encodedByteLength, requires64Bit);
        if (!CheckedParamSizeAdd(
                componentPayloadBytes,
                component.encodedByteLength,
                componentPayloadBytes,
                error,
                std::string(fieldName) + ".componentPayloadBytes")) {
            return false;
        }
    }
    return true;
}

inline bool ValidateNumericArrayRegionLayerForParams(
    const NumericArrayRegionLayerLayoutParams& layer,
    const NumericArrayBlockLayoutParams& block,
    const std::size_t componentCount,
    const std::string_view fieldName,
    bool& requires64Bit,
    ParamSize& residualPayloadBytes,
    std::string* error) {
    residualPayloadBytes = 0u;
    if (layer.regionId == 0u) {
        return SetParamsFieldError(error, fieldName, "region id is invalid");
    }
    if (!ValidateNumericArrayBytesCodecForParams(layer.residualBytesCodec) ||
        layer.residualBytesCodec != NumericArrayBytesCodec::NumericArrayCodec) {
        return SetParamsFieldError(error, fieldName, "residual bytes codec is invalid");
    }
    if (!ValidateCompressorConfigForParams(
            layer.refineCompressor,
            std::string(fieldName) + ".refineCompressor",
            requires64Bit,
            error)) {
        return false;
    }
    MarkRequires64Bit(layer.originalElementCount, requires64Bit);
    MarkRequires64Bit(layer.refinedElementCount, requires64Bit);
    MarkRequires64Bit(layer.expandedBackgroundCount, requires64Bit);
    MarkRequires64Bit(layer.residualEncodedByteLength, requires64Bit);
    MarkRequires64Bit(static_cast<ParamSize>(layer.runs.size()), requires64Bit);
    MarkRequires64Bit(static_cast<ParamSize>(layer.componentLayouts.size()), requires64Bit);

    ParamSize consumedElements = 0u;
    ParamSize previousEnd = 0u;
    bool hasPrevious = false;
    for (const auto& run : layer.runs) {
        if (run.count == 0u) {
            return SetParamsFieldError(error, fieldName, "region run is empty");
        }
        if (run.begin > block.elementCount || run.count > block.elementCount - run.begin) {
            return SetParamsFieldError(error, fieldName, "region run exceeds block range");
        }
        if (hasPrevious && run.begin < previousEnd) {
            return SetParamsFieldError(error, fieldName, "region runs overlap");
        }
        previousEnd = run.begin + run.count;
        hasPrevious = true;
        MarkRequires64Bit(run.begin, requires64Bit);
        MarkRequires64Bit(run.count, requires64Bit);
        if (!CheckedParamSizeAdd(
                consumedElements,
                run.count,
                consumedElements,
                error,
                std::string(fieldName) + ".runElements")) {
            return false;
        }
    }
    if (consumedElements != layer.refinedElementCount) {
        return SetParamsFieldError(error, fieldName, "region run elements do not match refined element count");
    }
    if (!ValidateNumericArrayComponentLayoutsForParams(
            std::span<const NumericArrayComponentLayoutParams>(
                layer.componentLayouts.data(),
                layer.componentLayouts.size()),
            componentCount,
            residualPayloadBytes,
            fieldName,
            requires64Bit,
            error)) {
        return false;
    }
    if (residualPayloadBytes != layer.residualEncodedByteLength) {
        return SetParamsFieldError(error, fieldName, "component layout bytes do not match residual payload bytes");
    }
    return true;
}

inline bool ValidateNumericArrayBlockLayoutsForParams(
    const NumericArrayStorageParams& meta,
    const std::string_view fieldName,
    bool& requires64Bit,
    std::string* error) {
    MarkRequires64Bit(static_cast<ParamSize>(meta.blockLayouts.size()), requires64Bit);
    ParamSize consumedElements = 0u;
    const auto componentCount = static_cast<std::size_t>(std::max<std::int32_t>(meta.dimension, 0));
    for (std::size_t blockIndex = 0; blockIndex < meta.blockLayouts.size(); ++blockIndex) {
        const auto& block = meta.blockLayouts[blockIndex];
        if (block.elementCount == 0u || block.elementOffset != consumedElements) {
            return SetParamsFieldError(error, fieldName, "block layout range is not contiguous");
        }
        if (block.elementCount > meta.elementCount - consumedElements) {
            return SetParamsFieldError(error, fieldName, "block layout range exceeds field element count");
        }
        MarkRequires64Bit(block.elementOffset, requires64Bit);
        MarkRequires64Bit(block.elementCount, requires64Bit);
        MarkRequires64Bit(block.encodedByteLength, requires64Bit);
        MarkRequires64Bit(block.backgroundEncodedByteLength, requires64Bit);
        MarkRequires64Bit(static_cast<ParamSize>(block.componentLayouts.size()), requires64Bit);
        MarkRequires64Bit(static_cast<ParamSize>(block.regionLayers.size()), requires64Bit);
        MarkRequires64Bit(static_cast<ParamSize>(block.alpha.size()), requires64Bit);
        MarkRequires64Bit(static_cast<ParamSize>(block.beta.size()), requires64Bit);
        if (!ValidateNumericArrayBlockModeForParams(block.mode)) {
            return SetParamsFieldError(error, fieldName, "block mode is invalid");
        }
        if (!ValidateNumericArrayBytesCodecForParams(block.bytesCodec)) {
            return SetParamsFieldError(error, fieldName, "block bytes codec is invalid");
        }
        if (block.mode == NumericArrayBlockMode::AffineReference &&
            (block.alpha.size() != componentCount || block.beta.size() != componentCount)) {
            return SetParamsFieldError(error, fieldName, "affine block layout coefficient count is invalid");
        }
        if (block.bytesCodec == NumericArrayBytesCodec::NumericArrayCodec) {
            ParamSize componentPayloadBytes = 0u;
            if (!ValidateNumericArrayComponentLayoutsForParams(
                    std::span<const NumericArrayComponentLayoutParams>(
                        block.componentLayouts.data(),
                        block.componentLayouts.size()),
                    componentCount,
                    componentPayloadBytes,
                    fieldName,
                    requires64Bit,
                    error)) {
                return false;
            }
            if (block.mode == NumericArrayBlockMode::LayeredResidual) {
                if (!ValidateCompressorConfigForParams(
                        block.backgroundCompressor,
                        std::string(fieldName) + ".backgroundCompressor",
                        requires64Bit,
                        error)) {
                    return false;
                }
                if (componentPayloadBytes != block.backgroundEncodedByteLength) {
                    return SetParamsFieldError(
                        error,
                        fieldName,
                        "component layout bytes do not match background payload bytes");
                }
                ParamSize totalLayerBytes = 0u;
                for (std::size_t layerIndex = 0u; layerIndex < block.regionLayers.size(); ++layerIndex) {
                    ParamSize layerBytes = 0u;
                    if (!ValidateNumericArrayRegionLayerForParams(
                            block.regionLayers[layerIndex],
                            block,
                            componentCount,
                            std::string(fieldName) + ".regionLayers[" + std::to_string(layerIndex) + "]",
                            requires64Bit,
                            layerBytes,
                            error)) {
                        return false;
                    }
                    if (!CheckedParamSizeAdd(
                            totalLayerBytes,
                            layerBytes,
                            totalLayerBytes,
                            error,
                            std::string(fieldName) + ".regionPayloadBytes")) {
                        return false;
                    }
                }
                ParamSize totalPayloadBytes = 0u;
                if (!CheckedParamSizeAdd(
                        block.backgroundEncodedByteLength,
                        totalLayerBytes,
                        totalPayloadBytes,
                        error,
                        std::string(fieldName) + ".layeredPayloadBytes")) {
                    return false;
                }
                if (totalPayloadBytes != block.encodedByteLength) {
                    return SetParamsFieldError(error, fieldName, "layered payload bytes do not match block payload bytes");
                }
            } else if (componentPayloadBytes != block.encodedByteLength) {
                return SetParamsFieldError(error, fieldName, "component layout bytes do not match block payload bytes");
            }
        } else if (block.mode == NumericArrayBlockMode::LayeredResidual) {
            return SetParamsFieldError(error, fieldName, "layered residual block requires numeric array codec bytes");
        }
        if (block.mode != NumericArrayBlockMode::LayeredResidual) {
            if (block.backgroundEncodedByteLength != 0u || !block.regionLayers.empty()) {
                return SetParamsFieldError(error, fieldName, "non-layered block carries region residual layout");
            }
        }
        consumedElements += block.elementCount;
    }
    if (meta.elementCount == 0u) {
        if (!meta.blockLayouts.empty()) {
            return SetParamsFieldError(error, fieldName, "empty numeric array has block layouts");
        }
    } else if (consumedElements != meta.elementCount) {
        return SetParamsFieldError(error, fieldName, "block layouts do not cover the full field");
    }
    return true;
}

inline bool ValidateNumericArrayStorageParamsForParams(
    const NumericArrayStorageParams& meta,
    const std::string_view fieldName,
    bool& requires64Bit,
    std::string* error) {
    if (!ValidateEncodedFieldCodecTypeForParams(meta.codecType)) {
        return SetParamsFieldError(error, fieldName, "codec type is invalid");
    }
    const auto expectedValueSize = DataTypeSize(meta.dataType);
    if (expectedValueSize == 0u) {
        return SetParamsFieldError(error, fieldName, "data type is unsupported");
    }
    if (meta.valueSize != static_cast<ParamSize>(expectedValueSize)) {
        return SetParamsFieldError(error, fieldName, "value size does not match data type");
    }
    if (meta.elementCount > 0u && meta.dimension <= 0) {
        return SetParamsFieldError(error, fieldName, "dimension is invalid");
    }
    MarkRequires64Bit(meta.valueSize, requires64Bit);
    MarkRequires64Bit(meta.elementCount, requires64Bit);
    const auto componentCount = static_cast<ParamSize>(std::max<std::int32_t>(meta.dimension, 0));
    ParamSize valueCount = 0u;
    ParamSize byteCount = 0u;
    if (!CheckedParamSizeMultiply(
            meta.elementCount,
            componentCount,
            valueCount,
            error,
            std::string(fieldName) + ".valueCount") ||
        !CheckedParamSizeMultiply(
            valueCount,
            meta.valueSize,
            byteCount,
            error,
            std::string(fieldName) + ".byteCount")) {
        return false;
    }
    MarkRequires64Bit(valueCount, requires64Bit);
    MarkRequires64Bit(byteCount, requires64Bit);
    return ValidateNumericArrayBlockLayoutsForParams(meta, fieldName, requires64Bit, error);
}

inline bool ValidateAttrStorageParamsForParams(
    const AttrStorageParams& meta,
    const std::string_view fieldName,
    bool& requires64Bit,
    std::string* error) {
    if (!ValidateNumericArrayStorageParamsForParams(meta, fieldName, requires64Bit, error)) {
        return false;
    }
    if (!ValidateAttrRoleForParams(meta.type)) {
        return SetParamsFieldError(error, fieldName, "attribute role is invalid");
    }
    if (!ValidateAttrAttachmentForParams(meta.attachmentType)) {
        return SetParamsFieldError(error, fieldName, "attribute attachment is invalid");
    }
    if (!ValidateAttributeDecodeScheduleClassForParams(meta.decodeScheduleHint.scheduleClass)) {
        return SetParamsFieldError(error, fieldName, "decode schedule class is invalid");
    }
    MarkRequires64Bit(static_cast<ParamSize>(meta.name.size()), requires64Bit);
    MarkRequires64Bit(meta.binaryCount, requires64Bit);
    MarkRequires64Bit(meta.decodeScheduleHint.estimatedDecodeCost, requires64Bit);
    MarkRequires64Bit(meta.decodeScheduleHint.criticalPathCost, requires64Bit);
    return true;
}

inline bool CalculateAttributePayloadBytes(
    const CodecStorageParams& params,
    ParamSize& payloadBytes,
    std::string* error = nullptr) {
    payloadBytes = 0u;
    for (const auto& meta : params.attrParams) {
        if (!CheckedParamSizeAdd(
                payloadBytes,
                meta.binaryCount,
                payloadBytes,
                error,
                "attribute payload bytes")) {
            return false;
        }
    }
    return true;
}

inline bool ValidateTopologyConnectivityLayoutForParams(
    const TopoStorageParams& topo,
    bool& requires64Bit,
    std::string* error) {
    const auto& layout = topo.connectivityLayout;
    const ParamSize values[] = {
        layout.connectivityByteCount,
        layout.cellSizeByteCount,
        layout.cellPolynomialOrderByteCount,
        layout.cellTypeByteCount,
    };
    for (const auto value : values) {
        MarkRequires64Bit(value, requires64Bit);
    }
    MarkRequires64Bit(static_cast<ParamSize>(layout.blockLayouts.size()), requires64Bit);

    ParamSize payloadBytes = 0u;
    if (!CheckedParamSizeAdd(
            payloadBytes,
            layout.connectivityByteCount,
            payloadBytes,
            error,
            "topo.connectivityLayout.payloadBytes") ||
        !CheckedParamSizeAdd(
            payloadBytes,
            layout.cellSizeByteCount,
            payloadBytes,
            error,
            "topo.connectivityLayout.payloadBytes") ||
        !CheckedParamSizeAdd(
            payloadBytes,
            layout.cellPolynomialOrderByteCount,
            payloadBytes,
            error,
            "topo.connectivityLayout.payloadBytes") ||
        !CheckedParamSizeAdd(
            payloadBytes,
            layout.cellTypeByteCount,
            payloadBytes,
            error,
            "topo.connectivityLayout.payloadBytes")) {
        return false;
    }

    if (topo.isStructured != 0u || topo.isPolyhedron != 0u || topo.cellCount == 0u) {
        if (payloadBytes != 0u || !layout.blockLayouts.empty()) {
            return SetParamsError(error, "non-connectivity topology params cannot carry connectivity stream layout");
        }
        return true;
    }

    // 时间序列复用拓扑只保留规模信息，真实拓扑由 owner frame 提供
    if (topo.binaryCount == 0u && payloadBytes == 0u && layout.blockLayouts.empty()) {
        return true;
    }

    if (layout.blockLayouts.empty()) {
        return SetParamsError(error, "connectivity topology block layout is empty");
    }

    ParamSize blockPayloadBytes = 0u;
    ParamSize blockCellCount = 0u;
    ParamSize blockConnectivityCount = 0u;
    ParamSize blockConnectivityBytes = 0u;
    ParamSize blockCellSizeBytes = 0u;
    ParamSize blockOrderBytes = 0u;
    ParamSize blockTypeBytes = 0u;
    for (std::size_t blockIndex = 0u; blockIndex < layout.blockLayouts.size(); ++blockIndex) {
        const auto& block = layout.blockLayouts[blockIndex];
        const auto fieldName = "topo.connectivityLayout.blockLayouts[" + std::to_string(blockIndex) + "]";
        const ParamSize blockValues[] = {
            block.cellOffset,
            block.cellCount,
            block.connectivityOffset,
            block.connectivityCount,
            block.connectivityByteCount,
            block.cellSizeByteCount,
            block.cellPolynomialOrderByteCount,
            block.cellTypeByteCount,
        };
        for (const auto value : blockValues) {
            MarkRequires64Bit(value, requires64Bit);
        }
        if (block.cellOffset != blockCellCount ||
            block.connectivityOffset != blockConnectivityCount ||
            block.cellCount == 0u) {
            return SetParamsFieldError(error, fieldName, "block ranges are not contiguous");
        }
        ParamSize currentPayloadBytes = 0u;
        if (!CheckedParamSizeAdd(currentPayloadBytes, block.connectivityByteCount, currentPayloadBytes, error, fieldName) ||
            !CheckedParamSizeAdd(currentPayloadBytes, block.cellSizeByteCount, currentPayloadBytes, error, fieldName) ||
            !CheckedParamSizeAdd(currentPayloadBytes, block.cellPolynomialOrderByteCount, currentPayloadBytes, error, fieldName) ||
            !CheckedParamSizeAdd(currentPayloadBytes, block.cellTypeByteCount, currentPayloadBytes, error, fieldName) ||
            !CheckedParamSizeAdd(blockPayloadBytes, currentPayloadBytes, blockPayloadBytes, error, fieldName) ||
            !CheckedParamSizeAdd(blockCellCount, block.cellCount, blockCellCount, error, fieldName) ||
            !CheckedParamSizeAdd(blockConnectivityCount, block.connectivityCount, blockConnectivityCount, error, fieldName) ||
            !CheckedParamSizeAdd(blockConnectivityBytes, block.connectivityByteCount, blockConnectivityBytes, error, fieldName) ||
            !CheckedParamSizeAdd(blockCellSizeBytes, block.cellSizeByteCount, blockCellSizeBytes, error, fieldName) ||
            !CheckedParamSizeAdd(blockOrderBytes, block.cellPolynomialOrderByteCount, blockOrderBytes, error, fieldName) ||
            !CheckedParamSizeAdd(blockTypeBytes, block.cellTypeByteCount, blockTypeBytes, error, fieldName)) {
            return false;
        }
    }
    if (blockCellCount != topo.cellCount ||
        blockConnectivityCount != topo.cellBufferSize ||
        blockPayloadBytes != topo.binaryCount ||
        blockConnectivityBytes != layout.connectivityByteCount ||
        blockCellSizeBytes != layout.cellSizeByteCount ||
        blockOrderBytes != layout.cellPolynomialOrderByteCount ||
        blockTypeBytes != layout.cellTypeByteCount) {
        return SetParamsError(error, "connectivity topology block totals do not match aggregate layout");
    }

    if (payloadBytes != topo.binaryCount) {
        return SetParamsError(error, "connectivity topology stream layout bytes do not match topology payload bytes");
    }
    if (topo.hasCellTypes == 0u && layout.cellTypeByteCount != 0u) {
        return SetParamsError(error, "connectivity topology layout has cell type bytes without cell types");
    }
    if (topo.hasCellTypes != 0u && layout.cellTypeByteCount == 0u) {
        return SetParamsError(error, "connectivity topology cell types are missing their stream");
    }
    if (topo.fixedCellSize > 0 && layout.cellSizeByteCount != 0u) {
        return SetParamsError(error, "fixed-size connectivity topology carries a cell-size stream");
    }
    if (topo.fixedCellSize <= 0 && layout.cellSizeByteCount == 0u) {
        return SetParamsError(error, "variable-size connectivity topology is missing its cell-size stream");
    }
    return true;
}

inline bool ValidateTopologyPolyhedronLayoutsForParams(
    const TopoStorageParams& topo,
    bool& requires64Bit,
    std::string* error) {
    MarkRequires64Bit(static_cast<ParamSize>(topo.polyhedronStreamLayouts.size()), requires64Bit);
    if (topo.isStructured == 0u && topo.isPolyhedron == 0u) {
        if (!topo.polyhedronStreamLayouts.empty()) {
            return SetParamsError(error, "connectivity topology params cannot carry polyhedron stream layouts");
        }
        return true;
    }
    if (topo.isPolyhedron == 0u || topo.cellCount == 0u) {
        if (!topo.polyhedronStreamLayouts.empty()) {
            return SetParamsError(error, "empty topology params cannot carry polyhedron stream layouts");
        }
        return true;
    }

    constexpr std::uint32_t kPolyhedronStreamKindUniqueVertexCounts = 0u;
    constexpr std::uint32_t kPolyhedronStreamKindCellUniqueVertexIds = 1u;
    constexpr std::uint32_t kPolyhedronStreamKindCellFaceCounts = 2u;
    constexpr std::uint32_t kPolyhedronStreamKindFaceVertexCounts = 3u;
    constexpr std::uint32_t kPolyhedronStreamKindLocalFaceVertexIds = 4u;
    constexpr std::uint32_t kPolyhedronStreamCodecVarint = 1u;
    constexpr std::uint32_t kPolyhedronStreamCodecSegmentedBitpack = 2u;
    constexpr std::uint32_t kPolyhedronStreamFlagHasAuxBytes = 1u;

    struct ExpectedStreamLayout {
        std::uint32_t kind{0};
        std::uint32_t codec{0};
        ParamSize elementCount{0};
    };
    const std::array<ExpectedStreamLayout, 5u> expected{
        ExpectedStreamLayout{kPolyhedronStreamKindUniqueVertexCounts, kPolyhedronStreamCodecVarint, topo.cellCount},
        ExpectedStreamLayout{kPolyhedronStreamKindCellFaceCounts, kPolyhedronStreamCodecVarint, topo.cellCount},
        ExpectedStreamLayout{kPolyhedronStreamKindFaceVertexCounts, kPolyhedronStreamCodecVarint, topo.polyhedronFaceVertexCount},
        ExpectedStreamLayout{kPolyhedronStreamKindCellUniqueVertexIds, kPolyhedronStreamCodecVarint, topo.polyhedronVertexCount},
        ExpectedStreamLayout{kPolyhedronStreamKindLocalFaceVertexIds, kPolyhedronStreamCodecSegmentedBitpack, topo.cellBufferSize},
    };

    if (topo.polyhedronStreamCount != expected.size() ||
        topo.polyhedronStreamLayouts.size() != expected.size()) {
        return SetParamsError(error, "polyhedron topology stream layout count is invalid");
    }

    ParamSize payloadBytes = 0u;
    for (std::size_t index = 0u; index < expected.size(); ++index) {
        const auto& layout = topo.polyhedronStreamLayouts[index];
        const auto& expectedLayout = expected[index];
        MarkRequires64Bit(layout.elementCount, requires64Bit);
        MarkRequires64Bit(layout.auxiliaryByteSize, requires64Bit);
        MarkRequires64Bit(layout.encodedByteLength, requires64Bit);
        if (layout.kind != expectedLayout.kind ||
            layout.codec != expectedLayout.codec ||
            layout.elementCount != expectedLayout.elementCount) {
            return SetParamsError(error, "polyhedron topology stream layout does not match params totals");
        }
        if ((layout.flags & ~kPolyhedronStreamFlagHasAuxBytes) != 0u ||
            layout.flags != 0u ||
            layout.auxiliaryByteSize != 0u) {
            return SetParamsError(error, "polyhedron topology stream layout has unsupported auxiliary bytes");
        }
        if (!CheckedParamSizeAdd(
                payloadBytes,
                layout.encodedByteLength,
                payloadBytes,
                error,
                "topo.polyhedronStreamLayouts.payloadBytes")) {
            return false;
        }
    }
    if (payloadBytes != topo.binaryCount) {
        return SetParamsError(error, "polyhedron topology stream layout bytes do not match topology payload bytes");
    }
    return true;
}

inline bool ValidateTopoStorageParamsForParams(
    const TopoStorageParams& topo,
    bool& requires64Bit,
    std::string* error) {
    if (!IsBooleanParamFlag(topo.isStructured) ||
        !IsBooleanParamFlag(topo.isPolyhedron) ||
        !IsBooleanParamFlag(topo.hasCellTypes)) {
        return SetParamsError(error, "topology flags are invalid");
    }
    if (topo.isStructured != 0u && topo.isPolyhedron != 0u) {
        return SetParamsError(error, "topology params cannot be both structured and polyhedron");
    }
    if (topo.fixedCellSize < 0) {
        return SetParamsError(error, "topology fixed cell size is invalid");
    }
    if (topo.isStructured != 0u && topo.binaryCount != 0u) {
        return SetParamsError(error, "structured topology params cannot carry topology payload bytes");
    }
    const ParamSize values[] = {
        topo.cellCount,
        topo.cellBufferSize,
        topo.cellTypeCount,
        topo.polyhedronVertexCount,
        topo.polyhedronFaceVertexCount,
        topo.polyhedronStreamCount,
        topo.binaryCount,
    };
    for (const auto value : values) {
        MarkRequires64Bit(value, requires64Bit);
    }
    return ValidateTopologyConnectivityLayoutForParams(topo, requires64Bit, error) &&
        ValidateTopologyPolyhedronLayoutsForParams(topo, requires64Bit, error);
}

inline bool ResolveSpatialBlockCountForParams(
    const ParamSize elementTotal,
    const std::uint32_t blockElementCount,
    std::uint32_t& blockCount,
    std::string* error) {
    blockCount = 0u;
    if (blockElementCount == 0u) {
        return SetParamsError(error, "spatial block element count is zero");
    }
    if (elementTotal == 0u) {
        return true;
    }
    const auto count = 1u + (elementTotal - 1u) / blockElementCount;
    if (count > std::numeric_limits<std::uint32_t>::max()) {
        return SetParamsError(error, "spatial block count exceeds uint32 range");
    }
    blockCount = static_cast<std::uint32_t>(count);
    return true;
}

inline bool ValidateNumericArraySpatialBlockAlignmentForParams(
    const NumericArrayStorageParams& meta,
    const std::uint32_t blockElementCount,
    const ParamSize elementTotal,
    const std::string_view fieldName,
    std::string* error) {
    if (meta.elementCount != elementTotal) {
        return SetParamsFieldError(error, fieldName, "element count does not match the shared spatial layout");
    }
    std::uint32_t expectedBlockCount = 0u;
    if (!ResolveSpatialBlockCountForParams(
            elementTotal,
            blockElementCount,
            expectedBlockCount,
            error)) {
        return false;
    }
    if (meta.blockLayouts.size() != expectedBlockCount) {
        return SetParamsFieldError(error, fieldName, "block count does not match the shared spatial layout");
    }
    for (std::size_t blockIndex = 0u; blockIndex < meta.blockLayouts.size(); ++blockIndex) {
        const auto expectedOffset = static_cast<ParamSize>(blockIndex) * blockElementCount;
        const auto expectedCount = std::min<ParamSize>(
            blockElementCount,
            elementTotal - expectedOffset);
        const auto& block = meta.blockLayouts[blockIndex];
        if (block.elementOffset != expectedOffset || block.elementCount != expectedCount) {
            return SetParamsFieldError(
                error,
                fieldName,
                "block range does not match the shared spatial layout");
        }
    }
    return true;
}

inline bool ValidateSpatialBlockStorageParamsForParams(
    const CodecStorageParams& params,
    bool& requires64Bit,
    std::string* error) {
    const auto& spatial = params.spatialBlockParams;
    MarkRequires64Bit(spatial.pointElementTotal, requires64Bit);
    MarkRequires64Bit(spatial.cellElementTotal, requires64Bit);
    std::uint32_t expectedPointBlockCount = 0u;
    std::uint32_t expectedCellBlockCount = 0u;
    if (!ResolveSpatialBlockCountForParams(
            spatial.pointElementTotal,
            spatial.pointElementCount,
            expectedPointBlockCount,
            error) ||
        !ResolveSpatialBlockCountForParams(
            spatial.cellElementTotal,
            spatial.cellElementCount,
            expectedCellBlockCount,
            error)) {
        return false;
    }
    if (spatial.pointBlockCount != expectedPointBlockCount ||
        spatial.cellBlockCount != expectedCellBlockCount) {
        return SetParamsError(error, "spatial block counts do not match their element ranges");
    }
    if (!ValidateNumericArraySpatialBlockAlignmentForParams(
            params.geomParams,
            spatial.pointElementCount,
            spatial.pointElementTotal,
            "geom",
            error)) {
        return false;
    }
    if (params.topoParams.cellCount != spatial.cellElementTotal) {
        return SetParamsError(error, "topology cell count does not match the shared cell spatial layout");
    }
    if (params.topoParams.binaryCount != 0u &&
        params.topoParams.isStructured == 0u &&
        params.topoParams.isPolyhedron == 0u &&
        params.topoParams.cellCount != 0u) {
        const auto& topologyBlocks = params.topoParams.connectivityLayout.blockLayouts;
        if (topologyBlocks.size() != expectedCellBlockCount) {
            return SetParamsError(error, "topology block count does not match the shared cell spatial layout");
        }
        for (std::size_t blockIndex = 0u; blockIndex < topologyBlocks.size(); ++blockIndex) {
            const auto expectedOffset = static_cast<ParamSize>(blockIndex) * spatial.cellElementCount;
            const auto expectedCount = std::min<ParamSize>(
                spatial.cellElementCount,
                spatial.cellElementTotal - expectedOffset);
            if (topologyBlocks[blockIndex].cellOffset != expectedOffset ||
                topologyBlocks[blockIndex].cellCount != expectedCount) {
                return SetParamsError(error, "topology block range does not match the shared cell spatial layout");
            }
        }
    }
    for (std::size_t index = 0u; index < params.attrParams.size(); ++index) {
        const auto& attr = params.attrParams[index];
        const auto isPoint = attr.attachmentType == AttrAttachment::Point;
        if (!ValidateNumericArraySpatialBlockAlignmentForParams(
                attr,
                isPoint ? spatial.pointElementCount : spatial.cellElementCount,
                isPoint ? spatial.pointElementTotal : spatial.cellElementTotal,
                "attr[" + std::to_string(index) + "]",
                error)) {
            return false;
        }
    }
    return true;
}

// [DC防护:参数] storage params 在写入或采用前先校验结构自洽和平台限制
inline bool ValidateCodecStorageParamsContent(
    const CodecStorageParams& params,
    bool& requires64Bit,
    std::string* error) {
    requires64Bit = false;
    if (!ValidateMeshTypeForParams(params.meshType)) {
        return SetParamsError(error, "mesh type is invalid");
    }
    if (!ValidateNumericArrayStorageParamsForParams(params.geomParams, "geom", requires64Bit, error) ||
        !ValidateTopoStorageParamsForParams(params.topoParams, requires64Bit, error)) {
        return false;
    }
    MarkRequires64Bit(static_cast<ParamSize>(params.attrParams.size()), requires64Bit);
    MarkRequires64Bit(static_cast<ParamSize>(params.attrPayloadOrder.size()), requires64Bit);

    for (std::size_t index = 0u; index < params.attrParams.size(); ++index) {
        if (!ValidateAttrStorageParamsForParams(
                params.attrParams[index],
                "attr[" + std::to_string(index) + "]",
                requires64Bit,
                error)) {
            return false;
        }
    }
    ParamSize attrPayloadBytes = 0u;
    if (!CalculateAttributePayloadBytes(params, attrPayloadBytes, error)) {
        return false;
    }
    MarkRequires64Bit(attrPayloadBytes, requires64Bit);

    std::vector<std::size_t> payloadOrder;
    if (!ResolveAttributePayloadOrder(params, payloadOrder, error)) {
        return false;
    }
    for (const auto attrIndex : params.attrPayloadOrder) {
        MarkRequires64Bit(attrIndex, requires64Bit);
    }
    return ValidateSpatialBlockStorageParamsForParams(params, requires64Bit, error);
}

inline bool CheckNumericArrayStorageParamsPlatformLimits(
    const NumericArrayStorageParams& meta,
    const std::string_view fieldName,
    std::string* error) {
    std::size_t ignored = 0u;
    if (!CheckedSizeT(meta.valueSize, std::string(fieldName) + ".valueSize", ignored, error) ||
        !CheckedSizeT(meta.elementCount, std::string(fieldName) + ".elementCount", ignored, error)) {
        return false;
    }
    const auto componentCount = static_cast<ParamSize>(std::max<std::int32_t>(meta.dimension, 0));
    ParamSize valueCount = 0u;
    ParamSize byteCount = 0u;
    if (!CheckedParamSizeMultiply(
            meta.elementCount,
            componentCount,
            valueCount,
            error,
            std::string(fieldName) + ".valueCount") ||
        !CheckedParamSizeMultiply(
            valueCount,
            meta.valueSize,
            byteCount,
            error,
            std::string(fieldName) + ".byteCount")) {
        return false;
    }
    return CheckedSizeT(valueCount, std::string(fieldName) + ".valueCount", ignored, error) &&
        CheckedSizeT(byteCount, std::string(fieldName) + ".byteCount", ignored, error);
}

inline bool CheckCodecStorageParamsPlatformLimits(
    const CodecStorageParams& params,
    std::string* error) {
    if (!CheckNumericArrayStorageParamsPlatformLimits(params.geomParams, "geom", error)) {
        return false;
    }
    std::size_t ignored = 0u;
    const auto& topo = params.topoParams;
    const std::pair<ParamSize, const char*> topoFields[] = {
        {topo.cellCount, "topo.cellCount"},
        {topo.cellBufferSize, "topo.cellBufferSize"},
        {topo.cellTypeCount, "topo.cellTypeCount"},
        {topo.polyhedronVertexCount, "topo.polyhedronVertexCount"},
        {topo.polyhedronFaceVertexCount, "topo.polyhedronFaceVertexCount"},
        {topo.polyhedronStreamCount, "topo.polyhedronStreamCount"},
        {topo.binaryCount, "topo.binaryCount"},
    };
    for (const auto& field : topoFields) {
        if (!CheckedSizeT(field.first, field.second, ignored, error)) {
            return false;
        }
    }
    const auto& connectivityLayout = topo.connectivityLayout;
    const std::pair<ParamSize, const char*> connectivityFields[] = {
        {connectivityLayout.connectivityByteCount, "topo.connectivityLayout.connectivityByteCount"},
        {connectivityLayout.cellSizeByteCount, "topo.connectivityLayout.cellSizeByteCount"},
        {connectivityLayout.cellPolynomialOrderByteCount, "topo.connectivityLayout.cellPolynomialOrderByteCount"},
        {connectivityLayout.cellTypeByteCount, "topo.connectivityLayout.cellTypeByteCount"},
    };
    for (const auto& field : connectivityFields) {
        if (!CheckedSizeT(field.first, field.second, ignored, error)) {
            return false;
        }
    }
    if (!CheckedSizeT(
            static_cast<ParamSize>(connectivityLayout.blockLayouts.size()),
            "topo.connectivityLayout.blockLayouts.size",
            ignored,
            error)) {
        return false;
    }
    for (std::size_t index = 0u; index < connectivityLayout.blockLayouts.size(); ++index) {
        const auto& block = connectivityLayout.blockLayouts[index];
        const auto prefix = "topo.connectivityLayout.blockLayouts[" + std::to_string(index) + "]";
        const std::pair<ParamSize, const char*> fields[] = {
            {block.cellOffset, "cellOffset"},
            {block.cellCount, "cellCount"},
            {block.connectivityOffset, "connectivityOffset"},
            {block.connectivityCount, "connectivityCount"},
            {block.connectivityByteCount, "connectivityByteCount"},
            {block.cellSizeByteCount, "cellSizeByteCount"},
            {block.cellPolynomialOrderByteCount, "cellPolynomialOrderByteCount"},
            {block.cellTypeByteCount, "cellTypeByteCount"},
        };
        for (const auto& field : fields) {
            if (!CheckedSizeT(field.first, prefix + "." + field.second, ignored, error)) {
                return false;
            }
        }
    }
    if (!CheckedSizeT(
            static_cast<ParamSize>(topo.polyhedronStreamLayouts.size()),
            "topo.polyhedronStreamLayouts.size",
            ignored,
            error)) {
        return false;
    }
    for (std::size_t index = 0u; index < topo.polyhedronStreamLayouts.size(); ++index) {
        const auto& layout = topo.polyhedronStreamLayouts[index];
        const auto fieldName = "topo.polyhedronStreamLayouts[" + std::to_string(index) + "]";
        if (!CheckedSizeT(layout.elementCount, fieldName + ".elementCount", ignored, error) ||
            !CheckedSizeT(layout.auxiliaryByteSize, fieldName + ".auxiliaryByteSize", ignored, error) ||
            !CheckedSizeT(layout.encodedByteLength, fieldName + ".encodedByteLength", ignored, error)) {
            return false;
        }
    }
    for (std::size_t index = 0u; index < params.attrParams.size(); ++index) {
        const auto fieldName = "attr[" + std::to_string(index) + "]";
        if (!CheckNumericArrayStorageParamsPlatformLimits(params.attrParams[index], fieldName, error) ||
            !CheckedSizeT(params.attrParams[index].binaryCount, fieldName + ".binaryCount", ignored, error)) {
            return false;
        }
    }
    std::vector<std::size_t> payloadOrder;
    return ResolveAttributePayloadOrder(params, payloadOrder, error);
}

inline bool BuildCodecStorageParamsHeader(
    const CodecStorageParams& params,
    CodecStorageParamsHeader& header,
    std::string* error) {
    bool requires64Bit = false;
    if (!ValidateCodecStorageParamsContent(params, requires64Bit, error)) {
        return false;
    }
    header = {};
    header.SetRequires64BitSize(requires64Bit);
    return true;
}

// 把整块数据的存储描述序列化成 params section
inline bool SerializeCodecStorageParams(
    const CodecStorageParams& params,
    std::vector<std::uint8_t>& bytes,
    std::string* error = nullptr) {
    bytes.clear();
    CodecStorageParamsHeader header;
    if (!BuildCodecStorageParamsHeader(params, header, error)) {
        return false;
    }
    try {
        std::ostringstream stream(std::ios::binary | std::ios::out);
        cereal::PortableBinaryOutputArchive archive(
            stream,
            cereal::PortableBinaryOutputArchive::Options::LittleEndian());
        archive(header, params);

        const auto serialized = stream.str();
        bytes.assign(serialized.begin(), serialized.end());
        return true;
    } catch (const std::exception& exception) {
        bytes.clear();
        return validation::AssignError(error, exception.what());
    }
}

// 把 params section 反序列化回强类型存储元数据
inline bool DeserializeCodecStorageParams(
    const std::span<const std::uint8_t> bytes,
    CodecStorageParams& params,
    std::string* error = nullptr,
    CodecErrorCode* errorCode = nullptr) {
    params = {};
    if (errorCode != nullptr) {
        *errorCode = CodecErrorCode::PipelineFailure;
    }
    if (bytes.empty()) {
        return validation::AssignError(error, "empty params buffer");
    }
    try {
        const std::string serialized(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        std::istringstream stream(serialized, std::ios::binary | std::ios::in);
        cereal::PortableBinaryInputArchive archive(stream);
        CodecStorageParamsHeader header;
        archive(header);
        if (header.magic != kCodecStorageParamsMagic) {
            if (errorCode != nullptr) {
                *errorCode = CodecErrorCode::UnsupportedFormat;
            }
            params = {};
            return SetParamsError(error, "DataCodec params format is invalid");
        }
        if (header.version != kCodecStorageParamsVersion) {
            if (errorCode != nullptr) {
                *errorCode = CodecErrorCode::UnsupportedFormat;
            }
            params = {};
            return SetParamsError(error, "版本不符合");
        }
        if ((header.flags & ~CodecStorageParamsHeader::kKnownFlags) != 0u) {
            if (errorCode != nullptr) {
                *errorCode = CodecErrorCode::UnsupportedFormat;
            }
            params = {};
            return SetParamsError(error, "unsupported DataCodec params flags");
        }
        // 32 位进程在反序列化 payload 前按 header 快速拒绝 64 位payload 尺寸
        if constexpr (sizeof(std::size_t) < sizeof(ParamSize)) {
            if (header.Requires64BitSize()) {
                if (errorCode != nullptr) {
                    *errorCode = CodecErrorCode::UnsupportedPlatform;
                }
                params = {};
                return SetParamsError(error, "encoded DataCodec params require a 64-bit decoder");
            }
        }
        archive(params);
        bool requires64Bit = false;
        if (!ValidateCodecStorageParamsContent(params, requires64Bit, error)) {
            if (errorCode != nullptr) {
                *errorCode = CodecErrorCode::InvalidInput;
            }
            params = {};
            return false;
        }
        if (!CheckCodecStorageParamsPlatformLimits(params, error)) {
            if (errorCode != nullptr) {
                *errorCode = CodecErrorCode::UnsupportedPlatform;
            }
            params = {};
            return false;
        }
        return true;
    } catch (const std::exception& exception) {
        params = {};
        return validation::AssignError(error, exception.what());
    }
}

} // namespace datacodec

#endif
