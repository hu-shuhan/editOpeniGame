#ifndef DATACODEC_RUNTIME_CACHE_DECODECACHE_REFERENCECACHENUMERICARRAYSOURCE_H
#define DATACODEC_RUNTIME_CACHE_DECODECACHE_REFERENCECACHENUMERICARRAYSOURCE_H

#include "DataCodec/Runtime/Cache/DecodeCache/DecodedAttributeCacheSet.h"
#include "DataCodec/Runtime/Cache/DecodeCache/DecodedGeometryCache.h"
#include "DataCodec/Codec/NumericArray/NumericArraySource.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <utility>
namespace datacodec {

inline bool ResolveReferenceCacheContiguousBytes(
    const std::shared_ptr<bytestore::IByteSource>& bytes,
    const std::size_t expectedBytes,
    std::span<const std::uint8_t>& contiguousBytes,
    bool& contiguousReady,
    std::string* error = nullptr) {
    contiguousBytes = {};
    contiguousReady = false;
    if (bytes == nullptr) {
        return validation::AssignError(error, "reference cache byte source is missing");
    }
    const auto status = bytes->PrepareContiguousBytes(contiguousBytes, error);
    if (status == ContiguousViewStatus::Error) {
        return false;
    }
    if (bytes->ByteSizeHint() != expectedBytes || !bytes->CanRead()) {
        return validation::AssignError(error, "reference cache byte source size is invalid");
    }
    if (status == ContiguousViewStatus::Ready) {
        if (contiguousBytes.size() != expectedBytes) {
            return validation::AssignError(error, "reference cache contiguous byte size is invalid");
        }
        contiguousReady = true;
    }
    return true;
}

struct GeometryReferenceCacheTupleSource {
    std::shared_ptr<DecodedGeometryReferenceCache> store;
    std::size_t tupleBytes{0u};
};

inline bool ReadGeometryReferenceCacheTupleBytes(
    const void* userData,
    const std::size_t tupleIndex,
    void* output,
    std::string* error) {
    const auto* source = static_cast<const GeometryReferenceCacheTupleSource*>(userData);
    if (source == nullptr || source->store == nullptr) {
        return validation::AssignError(error, "geometry reference tuple source is missing");
    }
    return source->store->ReadRange(
        tupleIndex,
        1u,
        output,
        source->tupleBytes,
        error);
}

inline bool ReadGeometryReferenceCacheTupleRangeBytes(
    const void* userData,
    const std::size_t tupleIndex,
    const std::size_t tupleCount,
    void* output,
    const std::size_t outputByteSize,
    std::string* error) {
    const auto* source = static_cast<const GeometryReferenceCacheTupleSource*>(userData);
    if (source == nullptr || source->store == nullptr) {
        return validation::AssignError(error, "geometry reference tuple source is missing");
    }
    return source->store->ReadRange(
        tupleIndex,
        tupleCount,
        output,
        outputByteSize,
        error);
}

inline bool BuildGeometryReferenceCacheNumericArraySource(
    std::shared_ptr<DecodedGeometryReferenceCache> store,
    const GeometryStorageParams& targetMeta,
    GeometryStorageParams& referenceMeta,
    numericarray::NumericArraySource& source,
    std::string* error = nullptr) {
    if (store == nullptr || !store->IsComplete()) {
        return validation::AssignError(error, "geometry reference cache is not complete");
    }
    referenceMeta = store->StorageParams();
    referenceMeta.codecType = EncodedFieldCodecType::Raw;
    if (referenceMeta.dataType != targetMeta.dataType ||
        referenceMeta.valueSize != targetMeta.valueSize ||
        referenceMeta.dimension != targetMeta.dimension) {
        return validation::AssignError(error, "geometry reference metadata does not match current geometry");
    }
    source = {};
    if (!numericarray::MakeNumericArrayLayoutFromMeta(referenceMeta, source.layout, error)) {
        return false;
    }
    const auto elementBytes = source.ElementBytes();
    if (elementBytes == 0u) {
        if (source.layout.elementCount != 0u) {
            return validation::AssignError(error, "geometry reference element size is invalid");
        }
        source.values = MakeCompactNumericArrayView(
            nullptr,
            ToScalarType(referenceMeta.dataType),
            0u,
            std::max(referenceMeta.dimension, 0),
            ViewBufferOrigin::CodecCache);
        return true;
    }
    std::size_t totalBytes = 0u;
    if (!validation::CheckedMulSizeT(
            source.layout.elementCount,
            elementBytes,
            totalBytes,
            "geometry reference bytes",
            error)) {
        return false;
    }
    const auto bytes = store->Bytes();
    std::span<const std::uint8_t> contiguousBytes;
    bool contiguousReady = false;
    if (!ResolveReferenceCacheContiguousBytes(
            bytes,
            totalBytes,
            contiguousBytes,
            contiguousReady,
            error)) {
        return false;
    }
    if (contiguousReady) {
        source.owner = bytes;
        source.values = MakeCompactNumericArrayView(
            contiguousBytes.data(),
            ToScalarType(referenceMeta.dataType),
            source.layout.elementCount,
            std::max(referenceMeta.dimension, 0),
            ViewBufferOrigin::CodecCache);
        return true;
    }
    auto tupleSource = std::make_shared<GeometryReferenceCacheTupleSource>();
    tupleSource->store = std::move(store);
    tupleSource->tupleBytes = elementBytes;
    source.owner = tupleSource;
    source.values.scalarType = ToScalarType(referenceMeta.dataType);
    source.values.layout = ArrayLayout::GetterOnly;
    source.values.origin = ViewBufferOrigin::CodecCache;
    source.values.tupleCount = source.layout.elementCount;
    source.values.componentCount = std::max(referenceMeta.dimension, 0);
    source.values.userData = tupleSource.get();
    source.values.getTupleBytes = &ReadGeometryReferenceCacheTupleBytes;
    source.values.getTupleRangeBytes = &ReadGeometryReferenceCacheTupleRangeBytes;
    return true;
}

inline bool TryFindAttributeReferenceCacheIndex(
    const DecodedAttributeCacheSet& store,
    const AttrAttachment attachment,
    const AttrStorageParams& targetMeta,
    std::size_t& attrIndex) {
    const auto& storageParams = store.StorageParams();
    for (std::size_t index = 0; index < storageParams.attrParams.size(); ++index) {
        const auto& meta = storageParams.attrParams[index];
        if (meta.attachmentType == attachment &&
            meta.name == targetMeta.name &&
            meta.dataType == targetMeta.dataType &&
            meta.dimension == targetMeta.dimension) {
            attrIndex = index;
            return true;
        }
    }
    return false;
}

struct AttributeReferenceCacheTupleSource {
    std::shared_ptr<DecodedAttributeCacheSet> store;
    std::size_t attrIndex{0u};
    std::size_t tupleBytes{0u};
};

struct DecodedAttributeCacheTupleSource {
    const DecodedAttributeCacheSet* store{nullptr};
    std::size_t attrIndex{0u};
    std::size_t tupleBytes{0u};
};

inline bool ReadAttributeReferenceCacheTupleBytes(
    const void* userData,
    const std::size_t tupleIndex,
    void* output,
    std::string* error) {
    const auto* source = static_cast<const AttributeReferenceCacheTupleSource*>(userData);
    if (source == nullptr || source->store == nullptr) {
        return validation::AssignError(error, "attribute reference tuple source is missing");
    }
    return source->store->ReadRange(
        source->attrIndex,
        tupleIndex,
        1u,
        output,
        source->tupleBytes,
        error);
}

inline bool ReadAttributeReferenceCacheTupleRangeBytes(
    const void* userData,
    const std::size_t tupleIndex,
    const std::size_t tupleCount,
    void* output,
    const std::size_t outputByteSize,
    std::string* error) {
    const auto* source = static_cast<const AttributeReferenceCacheTupleSource*>(userData);
    if (source == nullptr || source->store == nullptr) {
        return validation::AssignError(error, "attribute reference tuple source is missing");
    }
    return source->store->ReadRange(
        source->attrIndex,
        tupleIndex,
        tupleCount,
        output,
        outputByteSize,
        error);
}

inline bool ReadDecodedAttributeCacheTupleBytes(
    const void* userData,
    const std::size_t tupleIndex,
    void* output,
    std::string* error) {
    const auto* source = static_cast<const DecodedAttributeCacheTupleSource*>(userData);
    if (source == nullptr || source->store == nullptr) {
        return validation::AssignError(error, "decoded attribute tuple source is missing");
    }
    return source->store->ReadRange(
        source->attrIndex,
        tupleIndex,
        1u,
        output,
        source->tupleBytes,
        error);
}

inline bool ReadDecodedAttributeCacheTupleRangeBytes(
    const void* userData,
    const std::size_t tupleIndex,
    const std::size_t tupleCount,
    void* output,
    const std::size_t outputByteSize,
    std::string* error) {
    const auto* source = static_cast<const DecodedAttributeCacheTupleSource*>(userData);
    if (source == nullptr || source->store == nullptr) {
        return validation::AssignError(error, "decoded attribute tuple source is missing");
    }
    return source->store->ReadRange(
        source->attrIndex,
        tupleIndex,
        tupleCount,
        output,
        outputByteSize,
        error);
}

inline bool BuildDecodedAttributeCacheNumericArraySource(
    const DecodedAttributeCacheSet& store,
    const std::size_t attrIndex,
    const AttrStorageParams& meta,
    numericarray::NumericArraySource& source,
    std::string* error = nullptr) {
    const auto& storageParams = store.StorageParams();
    if (attrIndex >= storageParams.attrParams.size()) {
        return validation::AssignError(error, "decoded attribute cache index is out of range");
    }
    source = {};
    if (!numericarray::MakeNumericArrayLayoutFromMeta(meta, source.layout, error)) {
        return false;
    }
    const auto scalarType = ToScalarType(meta.dataType);
    const auto componentCount = std::max(meta.dimension, 0);
    const auto elementBytes = source.ElementBytes();
    if (elementBytes == 0u) {
        if (source.layout.elementCount != 0u) {
            return validation::AssignError(error, "decoded attribute element size is invalid");
        }
        source.values = MakeCompactNumericArrayView(
            nullptr,
            scalarType,
            0u,
            componentCount,
            ViewBufferOrigin::CodecCache);
        return true;
    }
    std::size_t totalBytes = 0u;
    if (!validation::CheckedMulSizeT(
            source.layout.elementCount,
            elementBytes,
            totalBytes,
            "decoded attribute bytes",
            error)) {
        return false;
    }
    const auto bytes = store.Bytes(attrIndex);
    std::span<const std::uint8_t> contiguousBytes;
    bool contiguousReady = false;
    if (!ResolveReferenceCacheContiguousBytes(
            bytes,
            totalBytes,
            contiguousBytes,
            contiguousReady,
            error)) {
        return false;
    }
    if (contiguousReady) {
        source.owner = bytes;
        source.values = MakeCompactNumericArrayView(
            contiguousBytes.data(),
            scalarType,
            source.layout.elementCount,
            componentCount,
            ViewBufferOrigin::CodecCache);
        return true;
    }
    auto tupleSource = std::make_shared<DecodedAttributeCacheTupleSource>();
    tupleSource->store = &store;
    tupleSource->attrIndex = attrIndex;
    tupleSource->tupleBytes = elementBytes;
    source.owner = tupleSource;
    source.values.scalarType = scalarType;
    source.values.layout = ArrayLayout::GetterOnly;
    source.values.origin = ViewBufferOrigin::CodecCache;
    source.values.tupleCount = source.layout.elementCount;
    source.values.componentCount = componentCount;
    source.values.userData = tupleSource.get();
    source.values.getTupleBytes = &ReadDecodedAttributeCacheTupleBytes;
    source.values.getTupleRangeBytes = &ReadDecodedAttributeCacheTupleRangeBytes;
    return true;
}

inline bool BuildAttributeReferenceCacheNumericArraySource(
    std::shared_ptr<DecodedAttributeCacheSet> store,
    const AttrAttachment attachment,
    const AttrStorageParams& targetMeta,
    AttrStorageParams& referenceMeta,
    numericarray::NumericArraySource& source,
    std::string* error = nullptr) {
    if (store == nullptr || !store->IsComplete()) {
        return validation::AssignError(error, "attribute reference cache is not complete");
    }
    std::size_t attrIndex = 0u;
    if (!TryFindAttributeReferenceCacheIndex(*store, attachment, targetMeta, attrIndex)) {
        return validation::AssignError(error, "matching attribute reference cache field was not found");
    }
    referenceMeta = store->StorageParams().attrParams[attrIndex];
    referenceMeta.codecType = EncodedFieldCodecType::Raw;
    source = {};
    if (!numericarray::MakeNumericArrayLayoutFromMeta(referenceMeta, source.layout, error)) {
        return false;
    }
    const auto scalarType = ToScalarType(referenceMeta.dataType);
    const auto componentCount = std::max(referenceMeta.dimension, 0);
    const auto elementBytes = source.ElementBytes();
    if (elementBytes == 0u) {
        if (source.layout.elementCount != 0u) {
            return validation::AssignError(error, "attribute reference element size is invalid");
        }
        source.values = MakeCompactNumericArrayView(
            nullptr,
            scalarType,
            0u,
            componentCount,
            ViewBufferOrigin::CodecCache);
        return true;
    }
    std::size_t totalBytes = 0u;
    if (!validation::CheckedMulSizeT(
            source.layout.elementCount,
            elementBytes,
            totalBytes,
            "attribute reference bytes",
            error)) {
        return false;
    }
    const auto bytes = store->Bytes(attrIndex);
    std::span<const std::uint8_t> contiguousBytes;
    bool contiguousReady = false;
    if (!ResolveReferenceCacheContiguousBytes(
            bytes,
            totalBytes,
            contiguousBytes,
            contiguousReady,
            error)) {
        return false;
    }
    if (contiguousReady) {
        source.owner = bytes;
        source.values = MakeCompactNumericArrayView(
            contiguousBytes.data(),
            scalarType,
            source.layout.elementCount,
            componentCount,
            ViewBufferOrigin::CodecCache);
        return true;
    }
    auto tupleSource = std::make_shared<AttributeReferenceCacheTupleSource>();
    tupleSource->store = std::move(store);
    tupleSource->attrIndex = attrIndex;
    tupleSource->tupleBytes = elementBytes;
    source.owner = tupleSource;
    source.values.scalarType = scalarType;
    source.values.layout = ArrayLayout::GetterOnly;
    source.values.origin = ViewBufferOrigin::CodecCache;
    source.values.tupleCount = source.layout.elementCount;
    source.values.componentCount = componentCount;
    source.values.userData = tupleSource.get();
    source.values.getTupleBytes = &ReadAttributeReferenceCacheTupleBytes;
    source.values.getTupleRangeBytes = &ReadAttributeReferenceCacheTupleRangeBytes;
    return true;
}

} // namespace datacodec

#endif
