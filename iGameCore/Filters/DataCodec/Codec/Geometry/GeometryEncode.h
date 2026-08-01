#ifndef DATACODEC_CODEC_GEOMETRY_GEOMETRYENCODE_H
#define DATACODEC_CODEC_GEOMETRY_GEOMETRYENCODE_H

#include "DataCodec/Runtime/Cache/DecodeCache/ReferenceCacheNumericArraySource.h"
#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Runtime/Cache/TransferCache/Common/NumericArrayTransferCacheBuilder.h"
#include "DataCodec/Runtime/Cache/TransferCache/GeometryTransferCache.h"
#include "DataCodec/Runtime/Cache/TransferCache/ReferenceTransferCacheBuilder.h"
#include "DataCodec/Runtime/Cache/CacheResources.h"
#include "DataCodec/Codec/NumericArray/NumericArrayReader.h"
#include "DataCodec/Codec/NumericArray/NumericArraySource.h"
#include "DataCodec/Codec/Reference/EncodeReferenceFrame.h"
#include "DataCodec/Codec/Reference/TemporalReference.h"
#include "DataCodec/Common/Views/ArrayViews.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"
#include "DataCodec/API/Params/CodecParamDefaults.h"
#include "DataCodec/API/Params/ReferenceControlParams.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

struct GeometryNumericArrayInput {
    GeometryStorageParams meta;
    numericarray::NumericArraySource source;
};

struct GeometryTransferCacheResult {
    std::shared_ptr<GeometryTransferCache> transferCache;
};

struct GeometryEncodeData {
    EncodeGeometryReferenceFrame& keyFrameReference;
    TemporalFieldRole temporalRole{TemporalFieldRole::SingleFrame};
    std::uint32_t keyFrameIndex{0u};
    std::uint32_t spatialBlockElementCount{262144u};
};

struct GeometryEncodeCache {
    CacheResources& cacheResources;
    bytestore::ByteStoreSession& byteStoreSession;
    DecodedGeometryReferenceCache* currentReferenceCache{nullptr};
    bool useMemoryTransferCache{false};
    bool useMemoryStaging{false};
};

struct GeometryEncodeRuntime {
    GeometryEncodeData data;
    GeometryEncodeCache cache;
};

struct GeometryReferenceDecision {
    bool hasReference{false};
    numericarrayreference::NumericArrayReferenceSourceData referenceSource;
    NumericArrayReferenceCodecId codecId{NumericArrayReferenceCodecId::NonReference};
};

inline bool ResolveGeometryNumericArrayLayout(
    const NumericArrayView& view,
    DataType& dataType,
    ParamSize& valueSize,
    std::string* error) {
    valueSize = view.ComponentSize();
    if (valueSize == 0u) {
        return validation::AssignError(error, "geometry numeric array scalar type is unsupported");
    }
    dataType = ToDataType(view.scalarType);
    return true;
}

inline bool BuildGeometryNumericArrayInput(
    numericarray::NumericArraySource source,
    GeometryStorageParams meta,
    GeometryNumericArrayInput& result,
    std::string* error = nullptr) {
    result = {};
    result.meta = std::move(meta);
    result.meta.dimension = 3;
    const auto& geometry = source.values;
    if (!geometry.IsValid()) {
        return validation::AssignError(error, "geometry view is invalid");
    }
    if (geometry.componentCount < 3) {
        return validation::AssignError(error, "geometry view requires at least 3 components");
    }
    DataType geometryDataType = DataType::Float32;
    ParamSize geometryValueSize = 0u;
    if (!ResolveGeometryNumericArrayLayout(geometry, geometryDataType, geometryValueSize, error)) {
        return false;
    }
    if (source.orderProvider != nullptr &&
        source.orderProvider->Size() != geometry.tupleCount) {
        return validation::AssignError(
            error,
            "geometry remap provider size does not match tuple count");
    }
    result.meta.dataType = geometryDataType;
    result.meta.elementCount = geometry.tupleCount;
    source.layout = numericarray::MakeNumericArrayLayout(
        geometryDataType,
        static_cast<std::size_t>(geometryValueSize),
        static_cast<std::size_t>(result.meta.elementCount),
        3u);
    result.source = std::move(source);
    return true;
}

inline NumericArrayReferenceCodecId ResolveGeometryReferenceCodecId(
    const GeometryReferenceControlParams& dependency) noexcept {
    if (!dependency.enabled) {
        return NumericArrayReferenceCodecId::NonReference;
    }
    return ResolveTemporalCodecId(dependency.temporalField.codec);
}

inline bool ResolveGeometryReferenceDecision(
    const GeometryEncodeRuntime& runtime,
    const GeometryStorageParams& currentMeta,
    const GeometryReferenceControlParams& dependency,
    GeometryReferenceDecision& decision,
    std::string* error = nullptr) {
    decision = {};
    if (runtime.data.temporalRole != TemporalFieldRole::PredFrame) {
        return true;
    }
    if (!dependency.enabled || dependency.temporalField.codec == TemporalFieldReferenceCodec::Disabled) {
        return validation::AssignError(error, "geometry prediction frame requires an enabled temporal reference codec");
    }
    if (runtime.data.keyFrameReference.geometryReferenceCache == nullptr ||
        !runtime.data.keyFrameReference.geometryReferenceCache->IsComplete()) {
        return validation::AssignError(error, "geometry key frame reference cache is not complete");
    }
    GeometryStorageParams referenceMeta;
    numericarray::NumericArraySource referenceSource;
    if (!BuildGeometryReferenceCacheNumericArraySource(
            runtime.data.keyFrameReference.geometryReferenceCache,
            currentMeta,
            referenceMeta,
            referenceSource,
            error)) {
        return false;
    }
    decision.hasReference = true;
    decision.codecId = ResolveGeometryReferenceCodecId(dependency);
    if (decision.codecId == NumericArrayReferenceCodecId::NonReference) {
        return validation::AssignError(error, "geometry reference candidate resolved to non-reference codec");
    }
    decision.referenceSource = numericarrayreference::NumericArrayReferenceSourceData{
        .candidate = NumericArrayReferenceCandidate{
            .scope = NumericArrayReferenceScope::TemporalKeyFrame,
            .referenceFrameIndex = runtime.data.keyFrameIndex,
            .localParentFieldIndex = 0xFFFFu,
        },
        .meta = referenceMeta,
        .source = std::move(referenceSource),
    };
    return true;
}

inline bool WriteCurrentGeometryReferenceCache(
    const GeometryEncodeRuntime& runtime,
    const GeometryStorageParams& meta,
    const numericarray::NumericArraySource& source,
    std::string* error = nullptr) {
    auto* store = runtime.cache.currentReferenceCache;
    if (store == nullptr) {
        return true;
    }
    if (!store->BeginGeometry(meta, error)) {
        return false;
    }
    numericarray::NumericArrayReader reader;
    if (!numericarray::BuildNumericArrayReader(source, reader, error)) {
        return false;
    }
    std::size_t localValueSize = 0u;
    std::size_t localElementCount = 0u;
    if (!TryParamSizeToSizeT(NumericArrayValueSize(meta), localValueSize) ||
        !TryParamSizeToSizeT(meta.elementCount, localElementCount)) {
        return validation::AssignError(error, "geometry metadata exceeds this platform size limit");
    }
    std::size_t tupleBytes = 0u;
    if (!validation::CheckedMulSizeT(
            static_cast<std::size_t>(std::max(meta.dimension, 0)),
            localValueSize,
            tupleBytes,
            "geometry tuple bytes",
            error)) {
        return false;
    }
    if (tupleBytes == 0u) {
        return store->EndGeometry(error);
    }
    const auto resolvedWindowBytes = std::max<std::size_t>(runtime.cache.cacheResources.accessWindowBytes, tupleBytes);
    const auto elementsPerWindow = std::max<std::size_t>(1u, resolvedWindowBytes / tupleBytes);
    std::size_t elementOffset = 0u;
    while (elementOffset < localElementCount) {
        const auto elementCount = std::min<std::size_t>(elementsPerWindow, localElementCount - elementOffset);
        std::size_t currentWindowBytes = 0u;
        if (!validation::CheckedMulSizeT(
                elementCount,
                tupleBytes,
                currentWindowBytes,
                "geometry encode window bytes",
                error)) {
            return false;
        }
        auto windowLease = runtime.cache.cacheResources.windowBudget.Acquire(currentWindowBytes);
        (void)windowLease;
        ScratchByteBuffer rangeBytes;
        if (!reader.ReadElements(elementOffset, elementCount, runtime.cache.cacheResources.scratchBytePool, rangeBytes, error)) {
            return false;
        }
        if (!store->WriteRange(elementOffset, elementCount, rangeBytes.Bytes().data(), rangeBytes.Bytes().size(), error)) {
            return false;
        }
        elementOffset += elementCount;
    }
    return store->EndGeometry(error);
}

inline bool BuildGeometryTransferCache(
    numericarray::NumericArraySource geometry,
    GeometryStorageParams meta,
    GeometryEncodeRuntime& runtime,
    const NumericArrayControlParams* controlParams,
    const GeometryReferenceControlParams& dependency,
    GeometryTransferCacheResult& result,
    std::string* error = nullptr) {
    result = {};
    GeometryNumericArrayInput numericArrayInput;
    if (!BuildGeometryNumericArrayInput(std::move(geometry), std::move(meta), numericArrayInput, error)) {
        return false;
    }
    auto geometryMeta = std::move(numericArrayInput.meta);
    NumericArrayControlParams defaultControl;
    if (controlParams == nullptr) {
        defaultControl.regionControl = MakeSingleRegionPrecisionControl(MakeDefaultGeometryValueCompressor());
        controlParams = &defaultControl;
    }
    const auto& regionControl = controlParams->regionControl;
    GeometryReferenceDecision referenceDecision;
    if (regionControl.regions.empty()) {
        if (!ResolveGeometryReferenceDecision(runtime, geometryMeta, dependency, referenceDecision, error)) {
            return false;
        }
    }
    geometryMeta.codecType = referenceDecision.hasReference
        ? EncodedFieldCodecType::Delta
        : EncodedFieldCodecType::NumericArrayBlocks;

    std::shared_ptr<bytestore::IByteSource> payload;
    if (geometryMeta.elementCount == 0u && !referenceDecision.hasReference) {
        payload = std::make_shared<bytestore::VectorByteSource>(std::vector<std::uint8_t>{});
        if (!WriteCurrentGeometryReferenceCache(runtime, geometryMeta, numericArrayInput.source, error)) {
            return false;
        }
        result.transferCache = std::make_shared<GeometryTransferCache>(std::move(geometryMeta), std::move(payload));
        return true;
    }

    numericarray::NumericArrayReader reader;
    if (!numericarray::BuildNumericArrayReader(numericArrayInput.source, reader, error)) {
        return false;
    }
    bool encodeOrdinary = !referenceDecision.hasReference;
    if (referenceDecision.hasReference) {
        const auto& defaultCompressor = regionControl.defaultPrecision.compressor;
        const auto createStagingStore =
            numericarrayreference::NumericArrayReferenceStagingStoreFactory{
                [&runtime](
                    const std::string& label,
                    std::uint64_t,
                    std::string* storeError) {
                    return bytestore::CreateByteStore(
                        runtime.cache.byteStoreSession,
                        label,
                        runtime.cache.useMemoryStaging,
                        storeError);
                }};
        std::string referenceError;
        if (!numericarrayreference::BuildNumericArrayReferenceTransferCache(
                geometryMeta,
                defaultCompressor,
                numericArrayInput.source,
                referenceDecision.referenceSource,
                referenceDecision.codecId,
                numericarrayreference::NumericArrayReferenceTransferControl{
                    .predictor = dependency.temporalField.predictor,
                    .selectionMode = dependency.temporalField.selectionMode,
                    .spatialBlockElementCount = runtime.data.spatialBlockElementCount,
                    .createStagingStore = createStagingStore,
                    .useMemoryStaging = runtime.cache.useMemoryStaging,
                    .useMemoryTransferCache = runtime.cache.useMemoryTransferCache},
                runtime.cache.cacheResources.scratchBytePool,
                runtime.cache.cacheResources.windowBudget,
                runtime.cache.cacheResources.accessWindowBytes,
                payload,
                runtime.cache.byteStoreSession,
                &geometryMeta.blockLayouts,
                &referenceError,
                "geometry_reference")) {
            if (dependency.temporalField.selectionMode == ReferenceSelectionMode::Forced) {
                return validation::AssignError(
                    error,
                    "forced geometry reference encode failed: " + referenceError);
            }
            return validation::AssignError(
                error,
                "geometry reference encode failed: " + referenceError);
        } else {
            const auto hasReferenceBlock = std::any_of(
                geometryMeta.blockLayouts.begin(),
                geometryMeta.blockLayouts.end(),
                [](const NumericArrayBlockLayoutParams& layout) {
                    return NumericArrayBlockModeCodecId(layout.mode) !=
                        NumericArrayReferenceCodecId::NonReference;
                });
            geometryMeta.codecType = hasReferenceBlock
                ? EncodedFieldCodecType::Delta
                : EncodedFieldCodecType::NumericArrayBlocks;
        }
    }
    if (encodeOrdinary) {
        encodeimpl::NumericArrayTransferCacheResult numericArrayResult;
        encodeimpl::NumericArrayTransferCacheRuntime transferRuntime;
        transferRuntime.useMemoryTransferCache = runtime.cache.useMemoryTransferCache;
        numericarray::NumericArrayBlockParams blockParams;
        if (!numericarray::MakeNumericArrayBlockParamsFromMeta(geometryMeta, blockParams, error)) {
            return false;
        }
        if (controlParams != nullptr) {
            numericarray::ApplyNumericArrayControlParams(blockParams, *controlParams);
        }
        if (!encodeimpl::BuildNumericArrayTransferCache(
                blockParams,
                reader,
                runtime.cache.cacheResources.scratchBytePool,
                numericArrayResult,
                runtime.cache.byteStoreSession,
                error,
                "geometry",
                runtime.cache.cacheResources.accessWindowBytes,
                runtime.data.spatialBlockElementCount,
                &transferRuntime)) {
            return false;
        }
        payload = std::move(numericArrayResult.transferCache);
        geometryMeta.blockLayouts = std::move(numericArrayResult.blockLayouts);
    }
    if (payload == nullptr) {
        return validation::AssignError(error, "geometry transfer cache payload is missing");
    }
    if (!WriteCurrentGeometryReferenceCache(runtime, geometryMeta, numericArrayInput.source, error)) {
        return false;
    }
    result.transferCache = std::make_shared<GeometryTransferCache>(std::move(geometryMeta), std::move(payload));
    return true;
}

} // namespace datacodec

#endif
