#ifndef DATACODEC_CODEC_ATTRIBUTES_ATTRIBUTEENCODE_H
#define DATACODEC_CODEC_ATTRIBUTES_ATTRIBUTEENCODE_H

#include "DataCodec/API/Adapter/IEncodeAdapter.h"
#include "DataCodec/API/Params/CodecParamDefaults.h"
#include "DataCodec/API/Params/ReferenceControlParams.h"
#include "DataCodec/API/Params/CodecStorageParams.h"
#include "DataCodec/API/Params/CodecControlParams.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Codec/NumericArray/NumericArraySource.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Codec/Attributes/AttributeEncodeScheduler.h"
#include "DataCodec/Codec/Attributes/AttributeSpooler.h"
#include "DataCodec/Runtime/Cache/TransferCache/ReferenceTransferCacheBuilder.h"
#include "DataCodec/Codec/Reference/AttributeReferenceSchedule.h"
#include "DataCodec/Codec/Reference/AttributeReferenceScheduleBuilder.h"
#include "DataCodec/Codec/Reference/EncodeReferenceFrame.h"
#include "DataCodec/Codec/Reference/IntraFieldReference.h"
#include "DataCodec/Codec/Reference/TemporalReference.h"
#include "DataCodec/Codec/Remap/RemapOrderSource.h"
#include "DataCodec/Common/DataCodecError.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace datacodec {
namespace encodeimpl {

inline bool BuildAttributeNumericArraySource(
    const EncodeAttributeView& attr,
    const IRemapProvider* orderProvider,
    AttrStorageParams& meta,
    numericarray::NumericArraySource& source,
    std::string* error) {
    meta.dataType = ToDataType(attr.values.scalarType);
    const auto valueSize = attr.values.ComponentSize();
    if (valueSize == 0u || valueSize != DataTypeSize(meta.dataType)) {
        return validation::AssignError(error, "attribute numeric array scalar type is unsupported");
    }
    if (!attr.values.IsValid()) {
        return validation::AssignError(error, "attribute numeric array view is invalid");
    }
    if (orderProvider != nullptr && orderProvider->Size() != attr.values.tupleCount) {
        return validation::AssignError(
            error,
            "attribute remap provider size does not match tuple count");
    }
    meta.elementCount = attr.values.tupleCount;
    source = numericarray::NumericArraySource{
        .values = attr.values,
        .order = nullptr,
        .orderProvider = orderProvider,
        .layout = numericarray::MakeNumericArrayLayout(
            meta.dataType,
            valueSize,
            static_cast<std::size_t>(meta.elementCount),
            static_cast<std::size_t>(std::max(0, meta.dimension))),
    };
    return true;
}

inline bool BuildAttributeNumericArraySource(
    const IEncodeAttrView& attr,
    const IRemapProvider* orderProvider,
    AttrStorageParams& meta,
    numericarray::NumericArraySource& source,
    std::string* error) {
    EncodeAttributeView attrView;
    if (!attr.BuildAttributeView(attrView)) {
        return validation::AssignError(error, "attribute numeric array view is invalid");
    }
    return BuildAttributeNumericArraySource(attrView, orderProvider, meta, source, error);
}

} // namespace encodeimpl
} // namespace datacodec


#include "DataCodec/Codec/NumericArray/NumericArrayBlockEncode.h"
#include "DataCodec/Codec/NumericArray/NumericArrayReader.h"
#include "DataCodec/Runtime/Cache/TransferCache/Common/NumericArrayTransferCacheBuilder.h"
#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/API/Params/ReferenceControlParams.h"
#include "DataCodec/Codec/NumericArray/NumericArrayBlockWireFormat.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {
namespace encodeimpl {

inline bool IsSupportedAttributeDataType(const DataType type) {
    return numericarray::IsSupportedNumericArrayDataType(type);
}

struct AttributeFieldRequest {
    AttrAttachment attachment{AttrAttachment::Point};
    std::size_t attrIndex{0u};
    std::size_t metaIndex{0u};
    std::string_view stageName;
};

struct ResolvedAttributeField {
    AttributeFieldRequest request;
    AttrStorageParams meta;
    numericarray::NumericArraySource source;
    const NumericArrayControlParams* controlParams{nullptr};
};

struct AttributeFieldPayload {
    AttrStorageParams meta;
    std::shared_ptr<bytestore::IByteSource> transferCache;
};

struct AttributeEncodeData {
    IEncodeAdapter& adapter;
    CodecStorageParams& storageParams;
    const CodecControlParams* controlParams{nullptr};
    const RemapOrderSource& pointOrderSource;
    const RemapOrderSource& cellOrderSource;
    EncodeAttributeReferenceFrame& keyFrameReference;
    TemporalFieldRole temporalRole{TemporalFieldRole::SingleFrame};
    std::uint32_t keyFrameIndex{0u};
};

struct AttributeEncodeSchedule {
    AttributeEncodeScheduler& attributeScheduler;
    const EncodeResourceBudgetControlParams& resourceBudget;
    EncodeAttributeReferenceSchedule& pointReferenceSchedule;
    EncodeAttributeReferenceSchedule& cellReferenceSchedule;
};

struct AttributeEncodeCache {
    CacheResources& cacheResources;
    bytestore::ByteStoreSession& byteStoreSession;
    DecodedAttributeCacheSet* currentReferenceCache{nullptr};
    std::shared_ptr<AttributeSpooler> attributeOutput;
};

struct AttributeEncodeContext {
    std::function<void(std::string_view, std::uint64_t)> resourceCallback;
    std::mutex* currentReferenceCacheMutex{nullptr};
    std::mutex& referenceScheduleMutex;
    std::mutex& referenceScheduleBuildMutex;
};

struct AttributeEncodeRuntime {
    AttributeEncodeData data;
    AttributeEncodeSchedule schedule;
    AttributeEncodeCache cache;
    AttributeEncodeContext context;

    [[nodiscard]] std::size_t AttributeCount(const AttrAttachment attachment) const {
        return CountAttrStorageParams(data.storageParams, attachment);
    }

    [[nodiscard]] std::size_t MetaIndex(
        const AttrAttachment attachment,
        const std::size_t attrIndex) const {
        return ResolveAttrMetaIndex(data.storageParams, attachment, attrIndex);
    }

    [[nodiscard]] std::size_t SourceIndex(const std::size_t metaIndex) const {
        return ResolveAttrSourceIndex(data.storageParams, metaIndex);
    }

    [[nodiscard]] std::size_t LocalIndex(const std::size_t metaIndex) const {
        return ResolveAttrLocalIndex(data.storageParams, metaIndex);
    }

    [[nodiscard]] const IRemapProvider* OrderProvider(const AttrAttachment attachment) const noexcept {
        return attachment == AttrAttachment::Point
            ? data.pointOrderSource.Provider()
            : data.cellOrderSource.Provider();
    }

    [[nodiscard]] EncodeAttributeReferenceSchedule& ReferenceSchedule(
        const AttrAttachment attachment) noexcept {
        return attachment == AttrAttachment::Point
            ? schedule.pointReferenceSchedule
            : schedule.cellReferenceSchedule;
    }
};

using AttributeFieldEncodeResult = CodecStatus;

} // namespace encodeimpl
} // namespace datacodec


#include "DataCodec/Codec/Remap/CellRemapBuilder.h"
#include "DataCodec/Codec/Remap/PointRemapBuilder.h"
#include "DataCodec/Codec/Topology/Polyhedron/PolyhedronTopologyRemap.h"
#include "DataCodec/Codec/SubCodec/ZstdCodec.h"
#include "DataCodec/Codec/Reference/ReferenceCodec.h"
#include "DataCodec/Runtime/Cache/DecodeCache/ReferenceCacheNumericArraySource.h"
#include "DataCodec/Storage/Common/BinaryValueIO.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <string_view>
#include <vector>
namespace datacodec {
namespace encodeimpl {

class AttributeReferencePolicy {
public:
    [[nodiscard]] static bool EnableIntraFieldReference(const AttrReferenceControlParams& dependency) noexcept {
        return dependency.enabled &&
            dependency.intraField.codec != IntraFieldReferenceCodec::Disabled;
    }

    [[nodiscard]] static bool EnableTemporalFieldReference(const AttrReferenceControlParams& dependency) noexcept {
        return dependency.enabled &&
            dependency.temporalField.codec != TemporalFieldReferenceCodec::Disabled;
    }

    [[nodiscard]] static NumericArrayReferenceCodecId ResolveCodecId(
        const NumericArrayReferenceCandidate& candidate,
        const AttrReferenceControlParams& dependency) noexcept {
        if (!candidate.HasReference()) {
            return NumericArrayReferenceCodecId::NonReference;
        }
        if (!dependency.enabled) {
            return NumericArrayReferenceCodecId::NonReference;
        }

        if (candidate.scope == NumericArrayReferenceScope::TemporalKeyFrame) {
            switch (dependency.temporalField.codec) {
                case TemporalFieldReferenceCodec::Wavelet:
                    return NumericArrayReferenceCodecId::Wavelet;
                case TemporalFieldReferenceCodec::Predictor:
                    return NumericArrayReferenceCodecId::Predictor;
                case TemporalFieldReferenceCodec::Disabled:
                default:
                    return NumericArrayReferenceCodecId::NonReference;
            }
        }

        if (candidate.scope == NumericArrayReferenceScope::IntraArray) {
            switch (dependency.intraField.codec) {
                case IntraFieldReferenceCodec::Wavelet:
                    return NumericArrayReferenceCodecId::Wavelet;
                case IntraFieldReferenceCodec::Affine:
                    return NumericArrayReferenceCodecId::Affine;
                case IntraFieldReferenceCodec::Predictor:
                    return NumericArrayReferenceCodecId::Predictor;
                case IntraFieldReferenceCodec::Disabled:
                default:
                    return NumericArrayReferenceCodecId::NonReference;
            }
        }

        return NumericArrayReferenceCodecId::NonReference;
    }
};

namespace detail {

inline bool EnsureReferencePointOrderProvider(
    AttributeEncodeRuntime& runtime,
    const IEncodeAdapter& referenceAdapter,
    std::string* error = nullptr) {
    auto& reference = runtime.data.keyFrameReference;
    if (reference.pointOrderProvider != nullptr ||
        referenceAdapter.GetNumberOfPoints() <= 1u ||
        referenceAdapter.IsStructuredMesh()) {
        return true;
    }
    pointremap::RemapProviders result;
    NumericArrayView geometry;
    if (!referenceAdapter.BuildGeometryView(geometry)) {
        return validation::AssignError(error, "reference adapter did not provide a geometry view for point remap");
    }
    if (geometry.tupleCount != referenceAdapter.GetNumberOfPoints()) {
        return validation::AssignError(error, "reference geometry view tuple count does not match adapter point count");
    }
    const bool useMemoryRemap =
        runtime.schedule.resourceBudget.RemapEncodeStorageMode() == EncodeStorageMode::Memory;
    const auto providerFactory = MakeStoreBackedWritableRemapProviderFactory(
        runtime.cache.byteStoreSession,
        "attribute_reference_point_remap",
        useMemoryRemap);
    if (!pointremap::BuildPointMortonRemapProviders(
            geometry,
            result,
            error,
            pointremap::BuildOptions{
                .providerFactory = providerFactory,
                .byteStoreSession = &runtime.cache.byteStoreSession,
                .scratchBudget = &runtime.cache.cacheResources.remapScratchBudget,
                .mortonLeafBudgetBytes = runtime.schedule.resourceBudget.RemapMortonLeafBytes(),
                .mortonRunBufferBytes = runtime.schedule.resourceBudget.RemapMortonRunBufferBytes(),
                .useMemoryScratchStore = useMemoryRemap,
            })) {
        return false;
    }
    reference.pointOrderProvider = std::move(result.orderProvider);
    reference.pointInverseOrderProvider = std::move(result.inverseProvider);
    return true;
}

inline const IRemapProvider* ReferencePointOrderProvider(
    AttributeEncodeRuntime& runtime,
    const IEncodeAdapter& referenceAdapter,
    std::string* error = nullptr) {
    if (!EnsureReferencePointOrderProvider(runtime, referenceAdapter, error)) {
        return nullptr;
    }
    return runtime.data.keyFrameReference.pointOrderProvider.get();
}

inline bool EnsureReferenceCellOrderProvider(
    AttributeEncodeRuntime& runtime,
    const IEncodeAdapter& referenceAdapter,
    std::string* error = nullptr) {
    auto& reference = runtime.data.keyFrameReference;
    if (reference.cellOrderProvider != nullptr ||
        referenceAdapter.GetNumberOfCells() <= 1u ||
        referenceAdapter.IsStructuredMesh()) {
        return true;
    }
    if (!EnsureReferencePointOrderProvider(runtime, referenceAdapter, error)) {
        return false;
    }
    const auto* pointInverse = reference.pointInverseOrderProvider.get();

    const bool useMemoryRemap =
        runtime.schedule.resourceBudget.RemapEncodeStorageMode() == EncodeStorageMode::Memory;
    const auto providerFactory = MakeStoreBackedWritableRemapProviderFactory(
        runtime.cache.byteStoreSession,
        "attribute_reference_cell_remap",
        useMemoryRemap);
    std::shared_ptr<IRemapProvider> provider;
    if (referenceAdapter.IsPolyhedronMesh()) {
        if (!polyhedron::BuildPolyhedronCellRangeMortonRemapProvider(
                referenceAdapter,
                cellremap::BuildOptions{
                    .pointInverse = pointInverse,
                    .providerFactory = providerFactory,
                    .byteStoreSession = &runtime.cache.byteStoreSession,
                    .scratchBudget = &runtime.cache.cacheResources.remapScratchBudget,
                    .mortonLeafBudgetBytes = runtime.schedule.resourceBudget.RemapMortonLeafBytes(),
                    .mortonRunBufferBytes = runtime.schedule.resourceBudget.RemapMortonRunBufferBytes(),
                    .useMemoryScratchStore = useMemoryRemap,
                    .progressCallback = {},
                    .resourceCallback = runtime.context.resourceCallback,
                },
                provider,
                error)) {
            return false;
        }
    } else {
        TopologyView topology;
        if (!cellremap::BuildCellTopologyView(referenceAdapter, topology, error)) {
            return false;
        }
        if (!cellremap::BuildMortonRemapProvider(
                topology,
                cellremap::BuildOptions{
                    .pointInverse = pointInverse,
                    .providerFactory = providerFactory,
                    .byteStoreSession = &runtime.cache.byteStoreSession,
                    .scratchBudget = &runtime.cache.cacheResources.remapScratchBudget,
                    .mortonLeafBudgetBytes = runtime.schedule.resourceBudget.RemapMortonLeafBytes(),
                    .mortonRunBufferBytes = runtime.schedule.resourceBudget.RemapMortonRunBufferBytes(),
                    .useMemoryScratchStore = useMemoryRemap,
                    .resourceCallback = runtime.context.resourceCallback,
                },
                provider,
                error)) {
            return false;
        }
    }
    reference.cellOrderProvider = std::move(provider);
    return true;
}

inline const IRemapProvider* ResolveReferenceOrderProvider(
    AttributeEncodeRuntime& runtime,
    const IEncodeAdapter& referenceAdapter,
    const AttrAttachment attachment,
    std::string* error = nullptr) {
    if (attachment == AttrAttachment::Point) {
        return ReferencePointOrderProvider(runtime, referenceAdapter, error);
    }
    if (!EnsureReferenceCellOrderProvider(runtime, referenceAdapter, error)) {
        return nullptr;
    }
    return runtime.data.keyFrameReference.cellOrderProvider.get();
}

inline bool ValidateSupportedAttributeField(const AttrStorageParams& meta, std::string* error = nullptr) {
    if (!IsSupportedAttributeDataType(meta.dataType)) {
        return validation::AssignError(error, "attribute data type is unsupported");
    }
    if (meta.elementCount > 0u && meta.dimension <= 0) {
        return validation::AssignError(error, "attribute dimension is invalid");
    }
    return true;
}

inline bool ResolveExpectedAttributeRawBytes(
    const AttrStorageParams& meta,
    std::size_t& bytes,
    std::string* error = nullptr) {
    bytes = 0u;
    if (meta.dimension < 0) {
        return validation::AssignError(error, "attribute dimension is invalid");
    }
    std::size_t localValueSize = 0u;
    std::size_t localElementCount = 0u;
    if (!TryParamSizeToSizeT(NumericArrayValueSize(meta), localValueSize) ||
        !TryParamSizeToSizeT(meta.elementCount, localElementCount)) {
        return validation::AssignError(error, "attribute metadata exceeds this platform size limit");
    }
    const auto componentCount = static_cast<std::size_t>(meta.dimension);
    std::size_t tupleBytes = 0u;
    if (!validation::CheckedMulSizeT(
            componentCount,
            localValueSize,
            tupleBytes,
            "attribute tuple byte count",
            error)) {
        return false;
    }
    if (!validation::CheckedMulSizeT(
            localElementCount,
            tupleBytes,
            bytes,
            "attribute raw byte count",
            error)) {
        return false;
    }
    return true;
}

inline bool ValidateAttributeRawByteSpan(
    const AttrStorageParams& meta,
    const std::span<const std::uint8_t> bytes,
    const char* label,
    std::string* error = nullptr) {
    std::size_t expectedBytes = 0u;
    if (!ResolveExpectedAttributeRawBytes(meta, expectedBytes, error)) {
        return false;
    }
    if (bytes.size() != expectedBytes) {
        return validation::AssignError(error, std::string(label) + " byte count does not match attribute metadata");
    }
    return true;
}


template<typename TValue>
inline bool SelectTemporalPredictorOffsetOnlyForBlockTyped(
    const NumericArrayStorageParams& meta,
    const CompressorConfig& defaultCompressor,
    const std::span<const std::uint8_t> currentBytes,
    const numericarray::NumericArrayReader& referenceReader,
    const NumericArrayStorageParams& referenceMeta,
    ScratchByteBufferPool& scratchBytePool,
    const std::uint32_t elementOffset,
    const std::uint32_t elementCount,
    const std::size_t componentCount,
    const AttrReferenceControlParams& dependency,
    const NumericArrayReferenceKind referenceKind,
    const std::uint16_t localParentFieldIndex,
    const ScratchByteQuotaAcquire& acquireScratchQuota,
    std::int32_t& predictorOffset,
    std::string* error = nullptr) {
    predictorOffset = 0;
    if (referenceKind != NumericArrayReferenceKind::TemporalKeyFrame ||
        !dependency.temporalField.predictor.enableLocalWindowSearch ||
        dependency.temporalField.predictor.windowRadius <= 0) {
        return true;
    }

    const auto strategy = dependency.temporalField.predictor.searchStrategy;
    TemporalPredictorOffsetSearchResult bestResult;
    bool hasBestResult = false;
    const auto evaluateOffsets = [&](const std::vector<std::int32_t>& offsets) -> bool {
        for (const auto candidateOffset : offsets) {
            ScratchByteBuffer candidatePredictorBytes;
            if (!numericarrayreference::BuildNumericArrayPredictorReferenceBlockBytes(
                    referenceReader,
                    referenceMeta,
                    meta,
                    scratchBytePool,
                    acquireScratchQuota,
                    elementOffset,
                    elementCount,
                    candidateOffset,
                    candidatePredictorBytes,
                    error)) {
                return false;
            }
            TemporalPredictorOffsetSearchResult candidateResult;
            if (!EvaluateTemporalPredictorOffsetForBlock<TValue>(
                meta,
                defaultCompressor,
                currentBytes,
                candidatePredictorBytes.Span(),
                scratchBytePool,
                elementOffset,
                elementCount,
                componentCount,
                referenceKind,
                localParentFieldIndex,
                strategy,
                candidateOffset,
                candidateResult,
                error)) {
                return false;
            }
            candidatePredictorBytes.Release();
            if (!hasBestResult || IsBetterTemporalPredictorOffset(strategy, candidateResult, bestResult)) {
                bestResult = candidateResult;
                hasBestResult = true;
            }
        }
        return true;
    };

    if (UsesCoarseToFineTemporalPredictorSearch(strategy)) {
        if (!evaluateOffsets(BuildCoarseOffsets(dependency.temporalField.predictor.windowRadius))) {
            return false;
        }
        if (hasBestResult &&
            !evaluateOffsets(BuildRefinedOffsets(
                dependency.temporalField.predictor.windowRadius,
                bestResult.offset))) {
            return false;
        }
    } else if (!evaluateOffsets(BuildExhaustiveOffsets(dependency.temporalField.predictor.windowRadius))) {
        return false;
    }
    if (!hasBestResult) {
        return validation::AssignError(error, "failed to select temporal predictor offset");
    }
    predictorOffset = bestResult.offset;
    return true;
}

inline bool SelectTemporalPredictorOffsetOnlyForBlock(
    const NumericArrayStorageParams& meta,
    const CompressorConfig& defaultCompressor,
    const std::span<const std::uint8_t> currentBytes,
    const numericarray::NumericArrayReader& referenceReader,
    const NumericArrayStorageParams& referenceMeta,
    ScratchByteBufferPool& scratchBytePool,
    const std::uint32_t elementOffset,
    const std::uint32_t elementCount,
    const AttrReferenceControlParams& dependency,
    const NumericArrayReferenceKind referenceKind,
    const std::uint16_t localParentFieldIndex,
    const ScratchByteQuotaAcquire& acquireScratchQuota,
    std::int32_t& predictorOffset,
    std::string* error = nullptr) {
    const auto componentCount = static_cast<std::size_t>(std::max(meta.dimension, 0));
    if (NumericArrayValueSize(meta) == sizeof(float)) {
        return SelectTemporalPredictorOffsetOnlyForBlockTyped<float>(
            meta,
            defaultCompressor,
            currentBytes,
            referenceReader,
            referenceMeta,
            scratchBytePool,
            elementOffset,
            elementCount,
            componentCount,
            dependency,
            referenceKind,
            localParentFieldIndex,
            acquireScratchQuota,
            predictorOffset,
            error);
    }
    if (NumericArrayValueSize(meta) == sizeof(double)) {
        return SelectTemporalPredictorOffsetOnlyForBlockTyped<double>(
            meta,
            defaultCompressor,
            currentBytes,
            referenceReader,
            referenceMeta,
            scratchBytePool,
            elementOffset,
            elementCount,
            componentCount,
            dependency,
            referenceKind,
            localParentFieldIndex,
            acquireScratchQuota,
            predictorOffset,
            error);
    }
    return validation::AssignError(error, "temporal predictor requires float32 or float64 data");
}

struct ReferenceSourceData {
    NumericArrayReferenceCandidate candidate;
    AttrStorageParams meta;
    numericarray::NumericArraySource source;

    [[nodiscard]] bool HasReference() const noexcept {
        return candidate.HasReference();
    }
};

struct AttributeReferenceDecision {
    ReferenceSourceData referenceSource;

    [[nodiscard]] bool HasReference() const noexcept {
        return referenceSource.HasReference();
    }
};

inline bool BuildAttributeNumericArraySourceForIndex(
    const IEncodeAdapter& adapter,
    const AttrAttachment attachment,
    const std::size_t attrIndex,
    const IRemapProvider* orderProvider,
    AttrStorageParams& meta,
    numericarray::NumericArraySource& source,
    std::string* error = nullptr) {
    EncodeAttributeView attrView;
    if (!(attachment == AttrAttachment::Point
        ? adapter.BuildPointAttributeView(attrIndex, attrView)
        : adapter.BuildCellAttributeView(attrIndex, attrView))) {
        return validation::AssignError(error, "failed to get attribute view");
    }
    if (!encodeimpl::BuildAttributeNumericArraySource(
        attrView,
        orderProvider,
        meta,
        source,
        error)) {
        return false;
    }
    return true;
}

inline bool ResolveAttributeField(
    AttributeEncodeRuntime& runtime,
    const AttributeFieldRequest& request,
    ResolvedAttributeField& field,
    std::string* error = nullptr) {
    field = {};
    field.request = request;

    const auto attrCount = request.attachment == AttrAttachment::Point
        ? runtime.data.adapter.GetNumberOfPointAttrs()
        : runtime.data.adapter.GetNumberOfCellAttrs();
    if (request.attrIndex >= attrCount) {
        return validation::AssignError(error, "attribute index is outside the current attachment domain");
    }
    if (request.metaIndex >= runtime.data.storageParams.attrParams.size()) {
        return validation::AssignError(error, "attribute meta index is outside the storage params");
    }
    if (runtime.data.storageParams.attrParams.at(request.metaIndex).attachmentType != request.attachment) {
        return validation::AssignError(error, "attribute attachment does not match the storage params");
    }

    const auto* orderProvider = runtime.OrderProvider(request.attachment);
    field.meta = runtime.data.storageParams.attrParams.at(request.metaIndex);
    if (runtime.data.controlParams != nullptr) {
        field.controlParams = &runtime.data.controlParams->GetAttrControl(field.meta.name);
    }
    numericarray::NumericArraySource source;
    if (!BuildAttributeNumericArraySourceForIndex(
            runtime.data.adapter,
            request.attachment,
            request.attrIndex,
            orderProvider,
            field.meta,
            source,
            error)) {
        return false;
    }

    if (!ValidateSupportedAttributeField(field.meta, error)) {
        return false;
    }

    field.source = std::move(source);
    return true;
}

inline bool FindReferenceAttributeSource(
    AttributeEncodeRuntime& runtime,
    const IEncodeAdapter& referenceAdapter,
    const AttrAttachment attachment,
    const AttrStorageParams& currentMeta,
    AttrStorageParams& referenceMeta,
    numericarray::NumericArraySource& referenceSource,
    std::string* error = nullptr) {
    const auto attrCount =
        attachment == AttrAttachment::Point ? referenceAdapter.GetNumberOfPointAttrs() : referenceAdapter.GetNumberOfCellAttrs();
    std::string orderError;
    const auto* referenceOrderProvider =
        ResolveReferenceOrderProvider(runtime, referenceAdapter, attachment, &orderError);
    if (!orderError.empty()) {
        return validation::AssignError(error, orderError);
    }
    for (std::size_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
        EncodeAttributeView attrView;
        if (!(attachment == AttrAttachment::Point
                ? referenceAdapter.BuildPointAttributeView(attrIndex, attrView)
                : referenceAdapter.BuildCellAttributeView(attrIndex, attrView))) {
            return validation::AssignError(error, "failed to get reference attribute view");
        }
        if (attrView.name != currentMeta.name ||
            ToDataType(attrView.values.scalarType) != currentMeta.dataType ||
            attrView.values.componentCount != currentMeta.dimension) {
            continue;
        }
        referenceMeta = currentMeta;
        referenceMeta.codecType = EncodedFieldCodecType::Raw;
        return encodeimpl::BuildAttributeNumericArraySource(
                attrView,
                referenceOrderProvider,
                referenceMeta,
                referenceSource,
                error);
    }

    return validation::AssignError(error, "matching reference attribute source was not found");
}

inline bool FindReferenceAttributeSource(
    const std::shared_ptr<DecodedAttributeCacheSet>& referenceStore,
    const AttrAttachment attachment,
    const AttrStorageParams& currentMeta,
    AttrStorageParams& referenceMeta,
    numericarray::NumericArraySource& referenceSource,
    std::string* error = nullptr) {
    if (referenceStore == nullptr || !referenceStore->IsComplete()) {
        return validation::AssignError(error, "attribute reference cache is not complete");
    }
    return BuildAttributeReferenceCacheNumericArraySource(
        referenceStore,
        attachment,
        currentMeta,
        referenceMeta,
        referenceSource,
        error);
}

inline bool WriteAttributeSourceToReferenceCache(
    DecodedAttributeCacheSet& store,
    const std::size_t attrMetaIndex,
    const AttrStorageParams& meta,
    const numericarray::NumericArraySource& source,
    ScratchByteBufferPool& scratchBytePool,
    const std::size_t windowBytes,
    std::string* error = nullptr) {
    numericarray::NumericArrayReader reader;
    if (!numericarray::BuildNumericArrayReader(source, reader, error)) {
        return false;
    }
    std::size_t localValueSize = 0u;
    std::size_t localElementCount = 0u;
    if (!TryParamSizeToSizeT(NumericArrayValueSize(meta), localValueSize) ||
        !TryParamSizeToSizeT(meta.elementCount, localElementCount)) {
        return validation::AssignError(error, "attribute reference metadata exceeds this platform size limit");
    }
    std::size_t tupleBytes = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(std::max(meta.dimension, 0)),
            localValueSize,
            tupleBytes,
            "attribute reference cache tuple bytes",
            error)) {
        return false;
    }
    if (tupleBytes == 0u && localElementCount != 0u) {
        return validation::AssignError(error, "attribute reference cache tuple size is invalid");
    }
    if (!store.BeginAttribute(attrMetaIndex, meta, error)) {
        return false;
    }
    if (tupleBytes == 0u) {
        return store.EndAttribute(attrMetaIndex, error);
    }

    const auto resolvedWindowBytes = std::max<std::size_t>(windowBytes, tupleBytes);
    const auto elementsPerWindow = std::max<std::size_t>(1u, resolvedWindowBytes / tupleBytes);
    std::size_t elementOffset = 0u;
    while (elementOffset < localElementCount) {
        const auto elementCount = std::min<std::size_t>(
            elementsPerWindow,
            localElementCount - elementOffset);
        ScratchByteBuffer bytes;
        if (!reader.ReadElements(elementOffset, elementCount, scratchBytePool, bytes, error)) {
            return false;
        }
        std::size_t expectedWindowBytes = 0u;
        if (!validation::CheckedMulSizeT(
                elementCount,
                tupleBytes,
                expectedWindowBytes,
                "attribute reference cache window bytes",
                error)) {
            return false;
        }
        if (bytes.Bytes().size() != expectedWindowBytes) {
            return validation::AssignError(error, "attribute reference cache window bytes do not match tuple size");
        }
        if (!store.WriteAttributeRange(
                attrMetaIndex,
                elementOffset,
                elementCount,
                bytes.Bytes().data(),
                bytes.Bytes().size(),
                error)) {
            return false;
        }
        elementOffset += elementCount;
    }
    return store.EndAttribute(attrMetaIndex, error);
}

inline bool IsMissingAttributeReferenceSourceError(const std::string_view message) noexcept {
    return message == "matching reference attribute source was not found" ||
        message == "matching attribute reference cache field was not found";
}

inline bool ResolveTemporalReferenceSourceCandidate(
    AttributeEncodeRuntime& runtime,
    const AttrAttachment attachment,
    const AttrStorageParams& meta,
    const AttrReferenceControlParams& dependency,
    ReferenceSourceData& resolved,
    std::string* error = nullptr) {
    resolved = {};
    const auto& reference = runtime.data.keyFrameReference;
    if (runtime.data.temporalRole != TemporalFieldRole::PredFrame ||
        !AttributeReferencePolicy::EnableTemporalFieldReference(dependency)) {
        return true;
    }
    if (numericarray::IsIntegerNumericArrayDataType(meta.dataType) &&
        dependency.temporalField.codec != TemporalFieldReferenceCodec::Wavelet) {
        return validation::AssignError(
            error,
            "integer temporal attribute reference requires the wavelet codec");
    }
    AttrStorageParams referenceMeta = meta;
    numericarray::NumericArraySource referenceSource;
    std::string localError;
    if (reference.attrReferenceCache != nullptr) {
        if (!FindReferenceAttributeSource(
                reference.attrReferenceCache,
                attachment,
                meta,
                referenceMeta,
                referenceSource,
                &localError)) {
            if (numericarray::IsIntegerNumericArrayDataType(meta.dataType) &&
                IsMissingAttributeReferenceSourceError(localError)) {
                return true;
            }
            return validation::AssignError(error, localError);
        }
    } else if (reference.adapter != nullptr) {
        if (!FindReferenceAttributeSource(
                runtime,
                *reference.adapter,
                attachment,
                meta,
                referenceMeta,
                referenceSource,
                &localError)) {
            if (numericarray::IsIntegerNumericArrayDataType(meta.dataType) &&
                IsMissingAttributeReferenceSourceError(localError)) {
                return true;
            }
            return validation::AssignError(error, localError);
        }
    } else {
        return validation::AssignError(error, "attribute key-frame reference cache is missing");
    }

    resolved = ReferenceSourceData{
        .candidate = {},
        .meta = referenceMeta,
        .source = std::move(referenceSource),
    };
    resolved.candidate.scope = NumericArrayReferenceScope::TemporalKeyFrame;
    resolved.candidate.referenceFrameIndex = runtime.data.keyFrameIndex;
    return true;
}





inline bool BuildAttributeReferenceScheduleForAttachment(
    AttributeEncodeRuntime& runtime,
    const AttrAttachment attachment,
    const AttrReferenceControlParams& dependency,
    EncodeAttributeReferenceSchedule& schedule,
    std::string* error = nullptr) {
    schedule = {};
    const auto attrCount = runtime.AttributeCount(attachment);
    schedule.topologyOrder.resize(attrCount);
    schedule.entries.resize(attrCount);
    for (std::size_t index = 0; index < attrCount; ++index) {
        schedule.topologyOrder[index] = index;
    }
    if (attrCount == 0u) {
        schedule.initialized = true;
        return true;
    }
    if (runtime.data.temporalRole != TemporalFieldRole::SingleFrame ||
        !AttributeReferencePolicy::EnableIntraFieldReference(dependency)) {
        schedule.initialized = true;
        return true;
    }

    const auto* orderProvider = runtime.OrderProvider(attachment);
    std::vector<AttrStorageParams> metas(attrCount);
    std::vector<numericarray::NumericArraySource> sources(attrCount);
    std::vector<std::size_t> metaIndices(attrCount);
    std::vector<std::uint8_t> referenceAllowed(attrCount, 1u);
    for (std::size_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
        metaIndices[attrIndex] = runtime.MetaIndex(attachment, attrIndex);
        if (metaIndices[attrIndex] == kInvalidTransferCacheIndex) {
            return validation::AssignError(error, "attribute meta index is outside the storage params");
        }
        metas[attrIndex] = runtime.data.storageParams.attrParams.at(metaIndices[attrIndex]);
        const auto sourceIndex = runtime.SourceIndex(metaIndices[attrIndex]);
        if (sourceIndex == kInvalidTransferCacheIndex) {
            return validation::AssignError(error, "attribute source index is outside the adapter domain");
        }
        if (!BuildAttributeNumericArraySourceForIndex(
                runtime.data.adapter,
                attachment,
                sourceIndex,
                orderProvider,
                metas[attrIndex],
                sources[attrIndex],
                error) ||
            !ValidateSupportedAttributeField(metas[attrIndex], error)) {
            return false;
        }
        NumericArrayControlParams fallbackControl;
        fallbackControl.regionControl = MakeSingleRegionPrecisionControl(
            MakeDefaultAttributeValueCompressor());
        const auto& fieldControl = runtime.data.controlParams != nullptr
            ? runtime.data.controlParams->GetAttrControl(metas[attrIndex].name)
            : fallbackControl;
        if (!fieldControl.regionControl.regions.empty()) {
            referenceAllowed[attrIndex] = 0u;
        }
    }

    return BuildAttributeIntraFieldReferenceSchedule(
        metas,
        sources,
        metaIndices,
        referenceAllowed,
        dependency,
        runtime.cache.cacheResources.scratchBytePool,
        schedule,
        error);
}

inline bool ApplyAttributeRecordOrder(
    AttributeEncodeRuntime& runtime,
    std::string* error = nullptr) {
    if (runtime.cache.attributeOutput == nullptr) {
        return validation::AssignError(error, "attribute output was not initialized");
    }

    EncodeAttributeReferenceSchedule pointSchedule;
    EncodeAttributeReferenceSchedule cellSchedule;
    {
        std::lock_guard<std::mutex> lock(runtime.context.referenceScheduleMutex);
        pointSchedule = runtime.schedule.pointReferenceSchedule;
        cellSchedule = runtime.schedule.cellReferenceSchedule;
    }

    const auto pointAttrCount = runtime.AttributeCount(AttrAttachment::Point);
    const auto cellAttrCount = runtime.AttributeCount(AttrAttachment::Cell);
    std::vector<std::size_t> recordOrder;
    recordOrder.reserve(pointAttrCount + cellAttrCount);
    if (pointSchedule.topologyOrder.size() == pointAttrCount) {
        for (const auto localIndex : pointSchedule.topologyOrder) {
            recordOrder.push_back(runtime.MetaIndex(AttrAttachment::Point, localIndex));
        }
    } else {
        for (std::size_t localIndex = 0; localIndex < pointAttrCount; ++localIndex) {
            recordOrder.push_back(runtime.MetaIndex(AttrAttachment::Point, localIndex));
        }
    }
    if (cellSchedule.topologyOrder.size() == cellAttrCount) {
        for (const auto localIndex : cellSchedule.topologyOrder) {
            recordOrder.push_back(runtime.MetaIndex(AttrAttachment::Cell, localIndex));
        }
    } else {
        for (std::size_t localIndex = 0; localIndex < cellAttrCount; ++localIndex) {
            recordOrder.push_back(runtime.MetaIndex(AttrAttachment::Cell, localIndex));
        }
    }
    if (std::any_of(recordOrder.begin(), recordOrder.end(), [](const auto metaIndex) {
            return metaIndex == kInvalidTransferCacheIndex;
        })) {
        return validation::AssignError(error, "attribute record order contains an invalid meta index");
    }
    std::vector<ParamSize> storageOrder;
    storageOrder.reserve(recordOrder.size());
    for (const auto metaIndex : recordOrder) {
        storageOrder.push_back(static_cast<ParamSize>(metaIndex));
    }
    if (!runtime.cache.attributeOutput->SetRecordOrder(std::move(recordOrder), error)) {
        return false;
    }
    runtime.data.storageParams.attrPayloadOrder = std::move(storageOrder);
    return true;
}

inline bool PublishAttributeReferenceSchedule(
    AttributeEncodeRuntime& runtime,
    const AttrAttachment attachment,
    EncodeAttributeReferenceSchedule schedule,
    std::string* error = nullptr) {
    {
        std::lock_guard<std::mutex> lock(runtime.context.referenceScheduleMutex);
        auto& target = runtime.ReferenceSchedule(attachment);
        target = std::move(schedule);
        target.initialized = true;
    }
    return ApplyAttributeRecordOrder(runtime, error);
}

inline bool EnsureAttributeReferenceScheduleForAttachment(
    AttributeEncodeRuntime& runtime,
    const AttrAttachment attachment,
    const AttrReferenceControlParams& dependency,
    EncodeAttributeReferenceSchedule& schedule,
    std::string* error = nullptr) {
    std::lock_guard<std::mutex> buildLock(runtime.context.referenceScheduleBuildMutex);
    {
        std::lock_guard<std::mutex> scheduleLock(runtime.context.referenceScheduleMutex);
        schedule = runtime.ReferenceSchedule(attachment);
    }
    if (schedule.initialized) {
        return true;
    }
    if (!BuildAttributeReferenceScheduleForAttachment(runtime, attachment, dependency, schedule, error)) {
        return false;
    }
    return PublishAttributeReferenceSchedule(runtime, attachment, schedule, error);
}

inline bool ResolveScheduledFieldReferenceSourceCandidate(
    AttributeEncodeRuntime& runtime,
    const AttrAttachment attachment,
    const AttrStorageParams& meta,
    const AttrReferenceControlParams& dependency,
    const EncodeAttributeReferenceSchedule& schedule,
    const std::size_t attrIndex,
    ReferenceSourceData& resolved,
    std::string* error = nullptr) {
    resolved = {};
    ReferenceSourceData temporal;
    if (!ResolveTemporalReferenceSourceCandidate(
            runtime,
            attachment,
            meta,
            dependency,
            temporal,
            error)) {
        return false;
    }
    if (temporal.HasReference()) {
        resolved = std::move(temporal);
        return true;
    }
    if (runtime.data.temporalRole == TemporalFieldRole::PredFrame ||
        runtime.data.temporalRole != TemporalFieldRole::SingleFrame ||
        !AttributeReferencePolicy::EnableIntraFieldReference(dependency) ||
        attrIndex >= schedule.entries.size()) {
        return true;
    }

    const auto& entry = schedule.entries[attrIndex];
    if (!entry.hasIntraParent) {
        return true;
    }
    resolved = ReferenceSourceData{
        .candidate = {},
        .meta = entry.parentMeta,
        .source = entry.parentSource,
    };
    resolved.candidate.scope = NumericArrayReferenceScope::IntraArray;
    resolved.candidate.localParentFieldIndex = entry.parentMetaIndex;
    return true;
}

inline bool BuildAttributeReferenceDecision(
    AttributeEncodeRuntime& runtime,
    const ResolvedAttributeField& field,
    const AttrReferenceControlParams& dependency,
    AttributeReferenceDecision& decision,
    std::string* error = nullptr) {
    decision = {};
    EncodeAttributeReferenceSchedule referenceSchedule;
    if (!EnsureAttributeReferenceScheduleForAttachment(
            runtime,
            field.request.attachment,
            dependency,
            referenceSchedule,
            error)) {
        return false;
    }
    const auto localAttrIndex = runtime.LocalIndex(field.request.metaIndex);
    if (localAttrIndex == kInvalidTransferCacheIndex) {
        return validation::AssignError(error, "attribute local index is outside the storage params");
    }
    const auto decisionOk = ResolveScheduledFieldReferenceSourceCandidate(
        runtime,
        field.request.attachment,
        field.meta,
        dependency,
        referenceSchedule,
        localAttrIndex,
        decision.referenceSource,
        error);
    if (!decisionOk) {
        return false;
    }
    if (numericarray::IsIntegerNumericArrayDataType(field.meta.dataType) &&
        decision.referenceSource.HasReference()) {
        const auto codecId = AttributeReferencePolicy::ResolveCodecId(
            decision.referenceSource.candidate,
            dependency);
        if (codecId != NumericArrayReferenceCodecId::Wavelet ||
            decision.referenceSource.meta.dataType != field.meta.dataType ||
            decision.referenceSource.meta.dimension != field.meta.dimension ||
            decision.referenceSource.meta.elementCount != field.meta.elementCount) {
            return validation::AssignError(
                error,
                "integer attribute reference metadata or codec is incompatible");
        }
    }
    return true;
}

struct AttributeReferenceTransferData {
    const AttrStorageParams& meta;
    const NumericArrayControlParams* controlParams{nullptr};
    const numericarray::NumericArraySource& currentSource;
    const ReferenceSourceData& referenceData;
    NumericArrayReferenceCodecId codecId{NumericArrayReferenceCodecId::NonReference};
    const AttrReferenceControlParams& dependency;
    std::uint32_t spatialBlockElementCount{262144u};
};

inline ReferenceSelectionMode ResolveAttributeReferenceSelectionMode(
    const NumericArrayReferenceCandidate& candidate,
    const AttrReferenceControlParams& dependency) noexcept {
    return candidate.scope == NumericArrayReferenceScope::IntraArray
        ? dependency.intraField.selectionMode
        : dependency.temporalField.selectionMode;
}

inline const NumericArrayControlParams& ResolveAttributeControlParams(
    const NumericArrayControlParams* controlParams,
    NumericArrayControlParams& fallbackControl) {
    if (controlParams != nullptr) {
        return *controlParams;
    }
    fallbackControl.regionControl = MakeSingleRegionPrecisionControl(MakeDefaultAttributeValueCompressor());
    return fallbackControl;
}

struct AttributeReferenceTransferSchedule {
    AttributeEncodeScheduler& attributeScheduler;
};

struct AttributeReferenceTransferCache {
    ScratchByteBufferPool& scratchBytePool;
    window::WindowBudget& windowBudget;
    std::size_t windowBytes{0u};
    bytestore::ByteStoreSession& byteStoreSession;
};

inline bool BuildReferenceTransferCache(
    const AttributeReferenceTransferData& data,
    const AttributeReferenceTransferSchedule& schedule,
    const AttributeReferenceTransferCache& cache,
    std::shared_ptr<bytestore::IByteSource>& transferCache,
    std::vector<NumericArrayBlockLayoutParams>* blockLayouts = nullptr,
    std::string* error = nullptr,
    const std::string& storeLabel = "attribute_reference") {
    numericarrayreference::NumericArrayReferenceSourceData genericReferenceData{
        .candidate = data.referenceData.candidate,
        .meta = data.referenceData.meta,
        .source = data.referenceData.source,
    };
    NumericArrayControlParams fallbackControl;
    const auto& controlParams = ResolveAttributeControlParams(data.controlParams, fallbackControl);
    const auto& defaultCompressor = controlParams.regionControl.defaultPrecision.compressor;
    ScratchByteQuotaAcquire acquireScratchQuota = [&schedule](const std::uint64_t bytes) {
        return schedule.attributeScheduler.AcquireScratch(bytes);
    };
    auto selectPredictorOffset = [&data, &defaultCompressor, acquireScratchQuota](
        const NumericArrayStorageParams& currentMeta,
        const std::span<const std::uint8_t> currentBytes,
        const numericarray::NumericArrayReader& referenceReader,
        const NumericArrayStorageParams& referenceMeta,
        ScratchByteBufferPool& localScratchBytePool,
        const std::uint32_t elementOffset,
        const std::uint32_t elementCount,
        const NumericArrayReferenceKind referenceKind,
        const std::uint16_t localParentFieldIndex,
        std::int32_t& predictorOffset,
        std::string* localError) {
        if (referenceKind == NumericArrayReferenceKind::IntraArray) {
            predictorOffset = 0;
            return true;
        }
        return SelectTemporalPredictorOffsetOnlyForBlock(
            currentMeta,
            defaultCompressor,
            currentBytes,
            referenceReader,
            referenceMeta,
            localScratchBytePool,
            elementOffset,
            elementCount,
            data.dependency,
            referenceKind,
            localParentFieldIndex,
            acquireScratchQuota,
            predictorOffset,
            localError);
    };
    numericarrayreference::NumericArrayReferenceStagingStoreFactory createStagingStore =
        [&cache, &schedule](
            const std::string& label,
            const std::uint64_t logicalBytes,
            std::string* localError) -> std::shared_ptr<bytestore::IByteStore> {
            auto stagingLease = std::make_shared<AttributeByteQuota::Lease>(
                schedule.attributeScheduler.AcquireStaging(logicalBytes));
            auto store = bytestore::CreateByteStore(
                cache.byteStoreSession,
                label,
                schedule.attributeScheduler.StagingStorageMode() == EncodeStorageMode::Memory,
                localError);
            if (store == nullptr) {
                stagingLease->Release();
                return nullptr;
            }
            return std::shared_ptr<bytestore::IByteStore>(
                store.get(),
                [store = std::move(store), stagingLease](bytestore::IByteStore* rawStore) mutable {
                    if (rawStore != nullptr) {
                        rawStore->Release();
                    }
                    stagingLease->Release();
                    store.reset();
                });
        };
    return numericarrayreference::BuildNumericArrayReferenceTransferCache(
        data.meta,
        defaultCompressor,
        data.currentSource,
        genericReferenceData,
        data.codecId,
        numericarrayreference::NumericArrayReferenceTransferControl{
            .affineBlockRSquared = data.dependency.intraField.affine.blockRSquared,
            .predictor = data.dependency.temporalField.predictor,
            .selectionMode = ResolveAttributeReferenceSelectionMode(
                data.referenceData.candidate,
                data.dependency),
            .autoSelectionStrategy =
                data.referenceData.candidate.scope == NumericArrayReferenceScope::IntraArray
                ? data.dependency.intraField.autoSelectionStrategy
                : ReferenceAutoSelectionStrategy::Exact,
            .spatialBlockElementCount = data.spatialBlockElementCount,
            .acquireScratchQuota = acquireScratchQuota,
            .selectPredictorOffset = std::move(selectPredictorOffset),
            .createStagingStore = std::move(createStagingStore),
            .useMemoryStaging =
                schedule.attributeScheduler.StagingStorageMode() == EncodeStorageMode::Memory,
            .useMemoryTransferCache =
                schedule.attributeScheduler.TransferCacheStorageMode() == EncodeStorageMode::Memory,
        },
        cache.scratchBytePool,
        cache.windowBudget,
        cache.windowBytes,
        transferCache,
        cache.byteStoreSession,
        blockLayouts,
        error,
        storeLabel);
}

inline bool EncodeNonReferenceField(
    const ResolvedAttributeField& field,
    AttributeEncodeRuntime& runtime,
    AttributeFieldPayload& payload,
    std::string* error = nullptr) {
    payload = {};
    payload.meta = field.meta;
    numericarray::NumericArrayReader reader;
    if (!numericarray::BuildNumericArrayReader(field.source, reader, error)) {
        return false;
    }

    NumericArrayTransferCacheResult numericArrayResult;
    numericarray::NumericArrayBlockParams blockParams;
    if (!numericarray::MakeNumericArrayBlockParamsFromMeta(payload.meta, blockParams, error)) {
        return false;
    }
    NumericArrayControlParams fallbackControl;
    const auto& controlParams = ResolveAttributeControlParams(field.controlParams, fallbackControl);
    numericarray::ApplyNumericArrayControlParams(blockParams, controlParams);
    NumericArrayTransferCacheRuntime transferRuntime;
    transferRuntime.useMemoryTransferCache =
        runtime.schedule.attributeScheduler.TransferCacheStorageMode() == EncodeStorageMode::Memory;
    transferRuntime.acquireScratchQuota = [&runtime](const std::uint64_t bytes) {
        return runtime.schedule.attributeScheduler.AcquireScratch(bytes);
    };
    transferRuntime.acquireFloatingPointEncodeLane = [&runtime]() {
        return std::static_pointer_cast<void>(
            std::make_shared<AttributeLaneGate::Lease>(
                runtime.schedule.attributeScheduler.AcquirePressioLane()));
    };
    if (runtime.schedule.attributeScheduler.CollectTiming()) {
        transferRuntime.recordFloatingPointEncodeDuration =
            [&runtime](const std::chrono::nanoseconds duration) {
                runtime.schedule.attributeScheduler.RecordPressioDuration(duration);
            };
    }
    if (!BuildNumericArrayTransferCache(
            blockParams,
            reader,
            runtime.cache.cacheResources.scratchBytePool,
            numericArrayResult,
            runtime.cache.byteStoreSession,
            error,
            "attribute_" + std::to_string(field.request.metaIndex),
            runtime.cache.cacheResources.accessWindowBytes,
            runtime.data.storageParams.spatialBlockParams.ElementCount(field.request.attachment),
            &transferRuntime)) {
        return false;
    }
    payload.transferCache = std::move(numericArrayResult.transferCache);
    payload.meta.blockLayouts = std::move(numericArrayResult.blockLayouts);
    if (payload.transferCache == nullptr) {
        return validation::AssignError(error, "attribute transfer cache is null");
    }
    payload.meta.binaryCount = static_cast<ParamSize>(payload.transferCache->ByteSizeHint());
    payload.meta.codecType = EncodedFieldCodecType::Raw;
    return true;
}

inline bool EncodeReferenceField(
    const ResolvedAttributeField& field,
    const AttributeReferenceDecision& decision,
    const AttrReferenceControlParams& dependency,
    AttributeEncodeRuntime& runtime,
    AttributeFieldPayload& payload,
    std::string* error = nullptr) {
    payload = {};
    payload.meta = field.meta;
    const auto codecId = AttributeReferencePolicy::ResolveCodecId(decision.referenceSource.candidate, dependency);
    if (codecId == NumericArrayReferenceCodecId::NonReference) {
        return validation::AssignError(error, "attribute reference candidate resolved to non-reference codec");
    }
    if (numericarray::IsIntegerNumericArrayDataType(field.meta.dataType) &&
        codecId != NumericArrayReferenceCodecId::Wavelet) {
        return validation::AssignError(error, "integer attribute reference requires wavelet codec");
    }

    std::optional<AttributeLaneGate::Lease> referenceLane;
    referenceLane.emplace(runtime.schedule.attributeScheduler.AcquireReferenceLane());
    if (!BuildReferenceTransferCache(
            AttributeReferenceTransferData{
                .meta = payload.meta,
                .controlParams = field.controlParams,
                .currentSource = field.source,
                .referenceData = decision.referenceSource,
                .codecId = codecId,
                .dependency = dependency,
                .spatialBlockElementCount =
                    runtime.data.storageParams.spatialBlockParams.ElementCount(field.request.attachment),
            },
            AttributeReferenceTransferSchedule{
                .attributeScheduler = runtime.schedule.attributeScheduler,
            },
            AttributeReferenceTransferCache{
                .scratchBytePool = runtime.cache.cacheResources.scratchBytePool,
                .windowBudget = runtime.cache.cacheResources.windowBudget,
                .windowBytes = runtime.cache.cacheResources.accessWindowBytes,
                .byteStoreSession = runtime.cache.byteStoreSession,
            },
            payload.transferCache,
            &payload.meta.blockLayouts,
            error,
            "attribute_reference_" + std::to_string(field.request.metaIndex))) {
        return false;
    }
    payload.meta.binaryCount = static_cast<ParamSize>(
        payload.transferCache != nullptr ? payload.transferCache->ByteSizeHint() : 0u);
    const auto hasReferenceBlock = std::any_of(
        payload.meta.blockLayouts.begin(),
        payload.meta.blockLayouts.end(),
        [](const NumericArrayBlockLayoutParams& layout) {
            return NumericArrayBlockModeCodecId(layout.mode) !=
                NumericArrayReferenceCodecId::NonReference;
        });
    payload.meta.codecType = hasReferenceBlock
        ? EncodedFieldCodecType::Delta
        : EncodedFieldCodecType::Raw;
    return true;
}

inline bool EncodeFieldPayload(
    const ResolvedAttributeField& field,
    const AttributeReferenceDecision& decision,
    const AttrReferenceControlParams& dependency,
    AttributeEncodeRuntime& runtime,
    AttributeFieldPayload& payload,
    std::string* error = nullptr) {
    NumericArrayControlParams fallbackControl;
    const auto& controlParams = ResolveAttributeControlParams(field.controlParams, fallbackControl);
    const auto& regionControl = controlParams.regionControl;
    const auto forcedReferenceRequired =
        runtime.data.temporalRole == TemporalFieldRole::PredFrame
        ? dependency.temporalField.selectionMode == ReferenceSelectionMode::Forced
        : dependency.intraField.selectionMode == ReferenceSelectionMode::Forced;
    if (!regionControl.regions.empty()) {
        if (forcedReferenceRequired) {
            return validation::AssignError(
                error,
                "forced attribute reference does not support layered region encoding");
        }
        return EncodeNonReferenceField(field, runtime, payload, error);
    }
    if (!decision.HasReference()) {
        if (forcedReferenceRequired) {
            return validation::AssignError(
                error,
                "forced attribute reference has no compatible reference source");
        }
        return EncodeNonReferenceField(field, runtime, payload, error);
    }

    AttributeFieldPayload referencePayload;
    std::string referenceError;
    if (!EncodeReferenceField(
            field,
            decision,
            dependency,
            runtime,
            referencePayload,
            &referenceError)) {
        if (ResolveAttributeReferenceSelectionMode(
                decision.referenceSource.candidate,
                dependency) == ReferenceSelectionMode::Forced) {
            return validation::AssignError(
                error,
                "forced attribute reference encode failed: " + referenceError);
        }
        return validation::AssignError(
            error,
            "attribute reference encode failed: " + referenceError);
    }
    payload = std::move(referencePayload);
    return true;
}

inline bool PublishDomainRecord(
    AttributeEncodeRuntime& runtime,
    const ResolvedAttributeField& field,
    AttributeFieldPayload payload,
    std::string* error = nullptr) {
    auto transferCache = std::move(payload.transferCache);
    if (runtime.cache.attributeOutput == nullptr) {
        return validation::AssignError(error, "attribute output was not initialized");
    }
    if (!runtime.cache.attributeOutput->SetRecordSource(
            field.request.metaIndex,
            transferCache,
            error)) {
        if (transferCache != nullptr) {
            transferCache->Release();
        }
        return false;
    }

    runtime.data.storageParams.attrParams.at(field.request.metaIndex) = payload.meta;
    if (runtime.cache.currentReferenceCache == nullptr) {
        return true;
    }
    if (!runtime.cache.currentReferenceCache->IsInitialized()) {
        return validation::AssignError(error, "current attribute reference cache is not initialized");
    }
    if (runtime.context.currentReferenceCacheMutex == nullptr) {
        return validation::AssignError(error, "current attribute reference cache mutex is missing");
    }

    bool storeWriteOk = false;
    {
        std::lock_guard<std::mutex> lock(*runtime.context.currentReferenceCacheMutex);
        storeWriteOk = WriteAttributeSourceToReferenceCache(
            *runtime.cache.currentReferenceCache,
            field.request.metaIndex,
            field.meta,
            field.source,
            runtime.cache.cacheResources.scratchBytePool,
            runtime.cache.cacheResources.accessWindowBytes,
            error);
    }
    return storeWriteOk;
}

} // namespace detail

inline AttributeFieldEncodeResult EncodeAttributeField(
    AttributeEncodeRuntime& runtime,
    const AttributeFieldRequest& request,
    const AttrReferenceControlParams& dependency) {
    ResolvedAttributeField field;
    std::string fieldError;
    if (!detail::ResolveAttributeField(runtime, request, field, &fieldError)) {
        return {
            .success = false,
            .code = CodecErrorCode::MissingInput,
            .message = "failed to resolve attribute field: " + fieldError,
        };
    }

    detail::AttributeReferenceDecision referenceDecision;
    std::string referenceError;
    if (!detail::BuildAttributeReferenceDecision(
            runtime,
            field,
            dependency,
            referenceDecision,
            &referenceError)) {
        return {
            .success = false,
            .code = CodecErrorCode::PipelineFailure,
            .message = "failed to build attribute reference schedule: " + referenceError,
        };
    }

    AttributeFieldPayload payload;
    std::string encodeError;
    if (!detail::EncodeFieldPayload(
            field,
            referenceDecision,
            dependency,
            runtime,
            payload,
            &encodeError)) {
        return {
            .success = false,
            .code = CodecErrorCode::EncodeFailure,
            .message = "failed to encode attribute field: " + encodeError,
        };
    }

    std::string publishError;
    if (!detail::PublishDomainRecord(
            runtime,
            field,
            std::move(payload),
            &publishError)) {
        return {
            .success = false,
            .code = CodecErrorCode::PipelineFailure,
            .message = "failed to publish attribute field: " + publishError,
        };
    }
    return {};
}

} // namespace encodeimpl
} // namespace datacodec

#endif
