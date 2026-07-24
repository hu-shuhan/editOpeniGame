#ifndef DATACODEC_CODEC_REMAP_COMMON_MORTONREMAPBUILDER_H
#define DATACODEC_CODEC_REMAP_COMMON_MORTONREMAPBUILDER_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Codec/Remap/RemapProvider.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace datacodec {
namespace mortonremap {

inline constexpr std::uint32_t kMortonBucketMask16 = 0xffffu;
inline constexpr std::size_t kMortonBucketCount16 = 1u << 16u;
inline constexpr std::size_t kMortonDefaultLeafBudgetBytes = 256u * 1024u * 1024u;
inline constexpr std::size_t kMortonRunRecordBytes = sizeof(std::uint16_t) + sizeof(IndexType);
inline constexpr std::size_t kMortonRunBufferBytes = 8u * 1024u * 1024u;
inline constexpr std::size_t kMortonHighBucketBufferBytes = 1024u;

struct MortonRemapOptions {
    std::string resourcePrefix{"remap.morton"};
    std::function<void(double)> progressCallback{};
    std::function<void(std::string_view, std::uint64_t)> resourceCallback{};
    WritableRemapProviderFactory providerFactory{};
    bytestore::ByteStoreSession* byteStoreSession{nullptr};
    resource::ActiveByteBudget* scratchBudget{nullptr};
    std::size_t leafBudgetBytes{kMortonDefaultLeafBudgetBytes};
    std::size_t runBufferBytes{kMortonRunBufferBytes};
    bool buildInverse{false};
    bool useMemoryScratchStore{false};
};

struct MortonRemapResult {
    std::shared_ptr<IRemapProvider> orderProvider;
    std::shared_ptr<IRemapProvider> inverseProvider;
};

struct MortonKeyedIndex {
    std::uint32_t key{0u};
    IndexType index{0u};
};

struct MortonLowKeyLeaf {
    std::uint32_t begin{0};
    std::uint32_t end{0};
    std::uint64_t count{0};
};

inline void InvokeProgress(
    const MortonRemapOptions& options,
    const double normalized) {
    callback::InvokeProgress(options.progressCallback, normalized);
}

inline void InvokeResource(
    const MortonRemapOptions& options,
    const std::string_view suffix,
    const std::uint64_t logicalBytes) {
    const auto name = callback::MakeResourceName(options.resourcePrefix, suffix);
    callback::InvokeResource(options.resourceCallback, name, logicalBytes);
}

inline std::size_t ResolveLeafBudgetElements(const std::size_t budgetBytes) noexcept {
    const auto safeBudget = budgetBytes == 0u ? kMortonDefaultLeafBudgetBytes : budgetBytes;
    return std::max<std::size_t>(1u, safeBudget / (sizeof(IndexType) * 2u + sizeof(std::uint16_t)));
}

inline std::size_t ResolveRemapLeafBudgetBytes(const std::size_t requestedLeafBudgetBytes) noexcept {
    if (requestedLeafBudgetBytes != 0u) {
        return requestedLeafBudgetBytes;
    }
    return kMortonDefaultLeafBudgetBytes;
}

inline std::size_t ResolveRunBufferBytes(const std::size_t requestedRunBufferBytes) noexcept {
    if (requestedRunBufferBytes != 0u) {
        return requestedRunBufferBytes;
    }
    return kMortonRunBufferBytes;
}

inline std::uint64_t EstimateMortonWorkspaceBytes(
    const std::size_t elementCount,
    const std::size_t leafBudgetBytes,
    const std::size_t runBufferBytes = kMortonRunBufferBytes) noexcept {
    const auto leafBudgetElements = ResolveLeafBudgetElements(leafBudgetBytes);
    const auto bucketTables =
        static_cast<std::uint64_t>(kMortonBucketCount16) *
        static_cast<std::uint64_t>(sizeof(std::size_t)) * 4u;
    const auto touchedBuckets =
        static_cast<std::uint64_t>(kMortonBucketCount16) * sizeof(std::uint32_t);
    const auto leafScratch =
        static_cast<std::uint64_t>(leafBudgetElements) *
        static_cast<std::uint64_t>(sizeof(IndexType) * 2u + sizeof(std::uint16_t));
    const auto highBucketBuffers =
        static_cast<std::uint64_t>(std::min<std::size_t>(elementCount, kMortonBucketCount16)) *
        static_cast<std::uint64_t>(kMortonHighBucketBufferBytes);
    const auto runBuffer =
        static_cast<std::uint64_t>(ResolveRunBufferBytes(runBufferBytes));
    return std::max<std::uint64_t>(
        1u,
        validation::SaturatingAddU64(
            validation::SaturatingAddU64(bucketTables, touchedBuckets),
            validation::SaturatingAddU64(
                validation::SaturatingAddU64(leafScratch, highBucketBuffers),
                runBuffer)));
}

class RemapScratchRun final {
public:
    RemapScratchRun() = default;
    explicit RemapScratchRun(std::shared_ptr<bytestore::IByteStore> store)
        : m_store(std::move(store)) {}

    RemapScratchRun(const RemapScratchRun&) = delete;
    RemapScratchRun& operator=(const RemapScratchRun&) = delete;

    RemapScratchRun(RemapScratchRun&&) noexcept = default;
    RemapScratchRun& operator=(RemapScratchRun&&) noexcept = default;

    ~RemapScratchRun() { Release(); }

    [[nodiscard]] bool IsValid() const noexcept { return m_store != nullptr; }

    bool ResizeRecords(const std::size_t recordCount, std::string* error = nullptr) {
        std::uint64_t byteCount = 0u;
        if (!ResolveByteRange(recordCount, 0u, byteCount, error)) {
            return false;
        }
        if (m_store == nullptr) {
            return validation::AssignError(error, "Morton scratch run store is missing");
        }
        return m_store->Resize(byteCount, error);
    }

    bool WriteRecordBytes(
        const std::size_t recordOffset,
        const std::span<const std::uint8_t> bytes,
        std::string* error = nullptr) {
        if (m_store == nullptr) {
            return validation::AssignError(error, "Morton scratch run store is missing");
        }
        std::uint64_t byteOffset = 0u;
        if (!ResolveByteRange(recordOffset, bytes.size(), byteOffset, error)) {
            return false;
        }
        return m_store->WriteAt(byteOffset, bytes, error);
    }

    bool ReadRecordBytes(
        const std::size_t recordOffset,
        const std::span<std::uint8_t> bytes,
        std::string* error = nullptr) const {
        if (m_store == nullptr) {
            return validation::AssignError(error, "Morton scratch run store is missing");
        }
        std::uint64_t byteOffset = 0u;
        if (!ResolveByteRange(recordOffset, bytes.size(), byteOffset, error)) {
            return false;
        }
        return m_store->Read(byteOffset, bytes, error);
    }

    void Release() noexcept {
        if (m_store != nullptr) {
            m_store->Release();
            m_store.reset();
        }
    }

private:
    static bool ResolveByteRange(
        const std::size_t recordOffset,
        const std::size_t byteCount,
        std::uint64_t& byteOffset,
        std::string* error) {
        if (!validation::CanMulU64(static_cast<std::uint64_t>(recordOffset), kMortonRunRecordBytes)) {
            validation::AssignError(error, "Morton scratch run byte offset exceeds addressable size");
            return false;
        }
        byteOffset = static_cast<std::uint64_t>(recordOffset) * kMortonRunRecordBytes;
        if (!validation::CanAddU64(byteOffset, static_cast<std::uint64_t>(byteCount))) {
            validation::AssignError(error, "Morton scratch run byte range exceeds addressable size");
            return false;
        }
        return true;
    }

    std::shared_ptr<bytestore::IByteStore> m_store;
};

class RemapScratchSpooler final {
public:
    explicit RemapScratchSpooler(
        bytestore::ByteStoreSession& session,
        const bool useMemoryStore = false)
        : m_session(session),
          m_useMemoryStore(useMemoryStore) {}

    [[nodiscard]] RemapScratchRun CreateRun(
        const std::string_view label,
        const std::size_t recordCount,
        std::string* error = nullptr) {
        auto store = bytestore::CreateByteStore(
            m_session,
            std::string(label),
            m_useMemoryStore,
            error);
        if (store == nullptr) {
            return {};
        }
        RemapScratchRun run(std::move(store));
        if (!run.ResizeRecords(recordCount, error)) {
            return {};
        }
        return run;
    }

private:
    bytestore::ByteStoreSession& m_session;
    bool m_useMemoryStore{false};
};

inline void AppendRunRecord(
    std::vector<std::uint8_t>& buffer,
    const std::uint16_t lowKey,
    const IndexType elementId) {
    const auto offset = buffer.size();
    buffer.resize(offset + kMortonRunRecordBytes);
    buffer[offset] = static_cast<std::uint8_t>(lowKey & 0xffu);
    buffer[offset + 1u] = static_cast<std::uint8_t>((lowKey >> 8u) & 0xffu);
    const auto value = static_cast<std::uint32_t>(elementId);
    buffer[offset + 2u] = static_cast<std::uint8_t>(value & 0xffu);
    buffer[offset + 3u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    buffer[offset + 4u] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
    buffer[offset + 5u] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
}

inline std::uint16_t DecodeRunLowKey(const std::uint8_t* record) noexcept {
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(record[0]) |
        (static_cast<std::uint16_t>(record[1]) << 8u));
}

inline IndexType DecodeRunElementId(const std::uint8_t* record) noexcept {
    const auto value =
        static_cast<std::uint32_t>(record[2]) |
        (static_cast<std::uint32_t>(record[3]) << 8u) |
        (static_cast<std::uint32_t>(record[4]) << 16u) |
        (static_cast<std::uint32_t>(record[5]) << 24u);
    return static_cast<IndexType>(value);
}

inline bool FlushHighBucketBuffer(
    RemapScratchRun& output,
    std::vector<std::uint8_t>& buffer,
    std::vector<std::size_t>& highWriteOffsets,
    const std::size_t highBucket,
    std::string* error = nullptr) {
    if (buffer.empty()) {
        return true;
    }
    if (buffer.size() % kMortonRunRecordBytes != 0u) {
        return validation::AssignError(error, "Morton high bucket buffer is not record aligned");
    }
    if (!output.WriteRecordBytes(
            highWriteOffsets[highBucket],
            std::span<const std::uint8_t>(buffer.data(), buffer.size()),
            error)) {
        return false;
    }
    highWriteOffsets[highBucket] += buffer.size() / kMortonRunRecordBytes;
    buffer.clear();
    return true;
}

inline std::uint64_t HighBuffersCapacityBytes(
    const std::vector<std::vector<std::uint8_t>>& buffers) noexcept {
    std::uint64_t totalBytes = VectorCapacityBytes(buffers);
    for (const auto& buffer : buffers) {
        totalBytes = validation::SaturatingAddU64(totalBytes, VectorCapacityBytes(buffer));
    }
    return totalBytes;
}

template<typename TKeyGetter>
inline bool WriteHighBucketRun(
    const std::size_t elementCount,
    TKeyGetter&& keyGetter,
    const std::vector<std::size_t>& highCounts,
    const std::vector<std::size_t>& highOffsets,
    std::vector<std::size_t>& highWriteOffsets,
    RemapScratchSpooler& scratchSpooler,
    RemapScratchRun& run,
    const MortonRemapOptions& options,
    std::string* error = nullptr) {
    const auto label = options.resourcePrefix.empty()
        ? std::string("remap_morton_high_run")
        : options.resourcePrefix + ".high_run";
    run = scratchSpooler.CreateRun(label, elementCount, error);
    if (!run.IsValid()) {
        return validation::AssignError(error, "failed to open Morton high bucket run");
    }

    highWriteOffsets = highOffsets;
    std::vector<std::vector<std::uint8_t>> highBuffers(kMortonBucketCount16);
    const auto progressStep = std::max<std::size_t>(elementCount / 64u, 1u);
    for (std::size_t elementIndex = 0; elementIndex < elementCount; ++elementIndex) {
        const auto key = keyGetter(elementIndex);
        const auto highBucket = static_cast<std::uint16_t>((key >> 16u) & kMortonBucketMask16);
        const auto lowKey = static_cast<std::uint16_t>(key & kMortonBucketMask16);
        auto& buffer = highBuffers[highBucket];
        AppendRunRecord(buffer, lowKey, static_cast<IndexType>(elementIndex));
        if (buffer.size() >= kMortonHighBucketBufferBytes &&
            !FlushHighBucketBuffer(run, buffer, highWriteOffsets, highBucket, error)) {
            return false;
        }

        if ((elementIndex + 1u) % progressStep == 0u || elementIndex + 1u == elementCount) {
            InvokeProgress(
                options,
                0.35 + 0.15 * (static_cast<double>(elementIndex + 1u) / static_cast<double>(elementCount)));
        }
    }

    InvokeResource(options, "high_bucket_buffers", HighBuffersCapacityBytes(highBuffers));
    for (std::size_t highBucket = 0; highBucket < highBuffers.size(); ++highBucket) {
        if (!FlushHighBucketBuffer(run, highBuffers[highBucket], highWriteOffsets, highBucket, error)) {
            return false;
        }
    }

    for (std::size_t highBucket = 0; highBucket < kMortonBucketCount16; ++highBucket) {
        if (highWriteOffsets[highBucket] != highOffsets[highBucket] + highCounts[highBucket]) {
            return validation::AssignError(error, "Morton high bucket run write count mismatch");
        }
    }
    return true;
}

inline bool CountRunSegmentLowKeys(
    const RemapScratchRun& run,
    const std::size_t recordBegin,
    const std::size_t recordCount,
    const std::size_t runBufferBytes,
    std::vector<std::size_t>& lowCounts,
    std::vector<std::uint32_t>& touchedLowBuckets,
    std::string* error = nullptr) {
    touchedLowBuckets.clear();
    const auto recordsPerRead = std::max<std::size_t>(1u, runBufferBytes / kMortonRunRecordBytes);
    std::vector<std::uint8_t> buffer(recordsPerRead * kMortonRunRecordBytes);
    std::size_t remaining = recordCount;
    std::size_t currentRecordOffset = recordBegin;
    while (remaining > 0u) {
        const auto currentRecords = std::min<std::size_t>(remaining, recordsPerRead);
        const auto currentBytes = currentRecords * kMortonRunRecordBytes;
        if (!run.ReadRecordBytes(
                currentRecordOffset,
                std::span<std::uint8_t>(buffer.data(), currentBytes),
                error)) {
            return false;
        }
        for (std::size_t recordIndex = 0; recordIndex < currentRecords; ++recordIndex) {
            const auto* record = buffer.data() + recordIndex * kMortonRunRecordBytes;
            const auto lowKey = DecodeRunLowKey(record);
            if (lowCounts[lowKey] == 0u) {
                touchedLowBuckets.push_back(lowKey);
            }
            lowCounts[lowKey]++;
        }
        remaining -= currentRecords;
        currentRecordOffset += currentRecords;
    }
    return true;
}

inline void ResetLowWorkspace(
    const std::vector<std::uint32_t>& touchedLowBuckets,
    std::vector<std::size_t>& lowCounts,
    std::vector<std::size_t>& lowOffsets) {
    for (const auto lowBucket : touchedLowBuckets) {
        lowCounts[lowBucket] = 0u;
        lowOffsets[lowBucket] = 0u;
    }
}

inline std::vector<MortonLowKeyLeaf> BuildLowKeyLeaves(
    const std::vector<std::size_t>& lowCounts,
    const std::size_t leafBudgetElements) {
    std::vector<MortonLowKeyLeaf> leaves;
    std::uint32_t currentBegin = 0u;
    std::uint32_t currentEnd = 0u;
    std::uint64_t currentCount = 0u;
    bool hasCurrent = false;

    const auto pushCurrent = [&]() {
        if (!hasCurrent || currentCount == 0u) {
            return;
        }
        leaves.push_back(MortonLowKeyLeaf{
            .begin = currentBegin,
            .end = currentEnd,
            .count = currentCount,
        });
    };

    for (std::uint32_t lowKey = 0u; lowKey < kMortonBucketCount16; ++lowKey) {
        const auto count = static_cast<std::uint64_t>(lowCounts[lowKey]);
        if (count == 0u) {
            continue;
        }
        if (count > leafBudgetElements) {
            pushCurrent();
            hasCurrent = false;
            currentCount = 0u;
            leaves.push_back(MortonLowKeyLeaf{
                .begin = lowKey,
                .end = static_cast<std::uint32_t>(lowKey + 1u),
                .count = count,
            });
            continue;
        }
        if (!hasCurrent) {
            currentBegin = lowKey;
            currentEnd = static_cast<std::uint32_t>(lowKey + 1u);
            currentCount = count;
            hasCurrent = true;
            continue;
        }
        if (currentCount + count > leafBudgetElements) {
            leaves.push_back(MortonLowKeyLeaf{
                .begin = currentBegin,
                .end = currentEnd,
                .count = currentCount,
            });
            currentBegin = lowKey;
            currentEnd = static_cast<std::uint32_t>(lowKey + 1u);
            currentCount = count;
            continue;
        }
        currentCount += count;
        currentEnd = static_cast<std::uint32_t>(lowKey + 1u);
    }
    if (hasCurrent && currentCount > 0u) {
        leaves.push_back(MortonLowKeyLeaf{
            .begin = currentBegin,
            .end = currentEnd,
            .count = currentCount,
        });
    }
    return leaves;
}

inline bool ReadRunSegmentLeaf(
    const RemapScratchRun& run,
    const std::size_t recordBegin,
    const std::size_t recordCount,
    const MortonLowKeyLeaf& leaf,
    const std::size_t runBufferBytes,
    std::vector<IndexType>& scratch,
    std::vector<std::uint16_t>& lowBuckets,
    std::string* error = nullptr) {
    scratch.clear();
    lowBuckets.clear();
    scratch.reserve(static_cast<std::size_t>(leaf.count));
    lowBuckets.reserve(static_cast<std::size_t>(leaf.count));

    const auto recordsPerRead = std::max<std::size_t>(1u, runBufferBytes / kMortonRunRecordBytes);
    std::vector<std::uint8_t> buffer(recordsPerRead * kMortonRunRecordBytes);
    std::size_t remaining = recordCount;
    std::size_t currentRecordOffset = recordBegin;
    while (remaining > 0u) {
        const auto currentRecords = std::min<std::size_t>(remaining, recordsPerRead);
        const auto currentBytes = currentRecords * kMortonRunRecordBytes;
        if (!run.ReadRecordBytes(
                currentRecordOffset,
                std::span<std::uint8_t>(buffer.data(), currentBytes),
                error)) {
            return false;
        }
        for (std::size_t recordIndex = 0; recordIndex < currentRecords; ++recordIndex) {
            const auto* record = buffer.data() + recordIndex * kMortonRunRecordBytes;
            const auto lowKey = DecodeRunLowKey(record);
            if (lowKey < leaf.begin || lowKey >= leaf.end) {
                continue;
            }
            lowBuckets.push_back(lowKey);
            scratch.push_back(DecodeRunElementId(record));
        }
        remaining -= currentRecords;
        currentRecordOffset += currentRecords;
    }
    return scratch.size() == leaf.count && lowBuckets.size() == leaf.count;
}

inline bool ReadRunSegmentSingleLowKeyChunk(
    const RemapScratchRun& run,
    const std::size_t recordBegin,
    const std::size_t recordCount,
    const std::uint16_t lowKeyFilter,
    const std::uint64_t skipCount,
    const std::size_t maxCount,
    const std::size_t runBufferBytes,
    std::vector<IndexType>& scratch,
    std::string* error = nullptr) {
    scratch.clear();
    scratch.reserve(maxCount);
    std::uint64_t skipped = 0u;

    const auto recordsPerRead = std::max<std::size_t>(1u, runBufferBytes / kMortonRunRecordBytes);
    std::vector<std::uint8_t> buffer(recordsPerRead * kMortonRunRecordBytes);
    std::size_t remaining = recordCount;
    std::size_t currentRecordOffset = recordBegin;
    while (remaining > 0u && scratch.size() < maxCount) {
        const auto currentRecords = std::min<std::size_t>(remaining, recordsPerRead);
        const auto currentBytes = currentRecords * kMortonRunRecordBytes;
        if (!run.ReadRecordBytes(
                currentRecordOffset,
                std::span<std::uint8_t>(buffer.data(), currentBytes),
                error)) {
            return false;
        }
        for (std::size_t recordIndex = 0; recordIndex < currentRecords && scratch.size() < maxCount; ++recordIndex) {
            const auto* record = buffer.data() + recordIndex * kMortonRunRecordBytes;
            const auto lowKey = DecodeRunLowKey(record);
            if (lowKey != lowKeyFilter) {
                continue;
            }
            if (skipped < skipCount) {
                ++skipped;
                continue;
            }
            scratch.push_back(DecodeRunElementId(record));
        }
        remaining -= currentRecords;
        currentRecordOffset += currentRecords;
    }
    return scratch.size() == maxCount;
}

inline void BuildBufferedLeafOrder(
    const MortonLowKeyLeaf& leaf,
    const std::vector<IndexType>& scratch,
    const std::vector<std::uint16_t>& lowBuckets,
    const std::vector<std::size_t>& lowCounts,
    std::vector<std::size_t>& lowOffsets,
    std::vector<IndexType>& orderedElements) {
    orderedElements.resize(scratch.size());
    std::size_t runningOffset = 0u;
    for (std::size_t lowKey = leaf.begin; lowKey < leaf.end; ++lowKey) {
        lowOffsets[lowKey] = runningOffset;
        runningOffset += lowCounts[lowKey];
    }
    for (std::size_t index = 0; index < scratch.size(); ++index) {
        const auto lowKey = lowBuckets[index];
        orderedElements[lowOffsets[lowKey]++] = scratch[index];
    }
}

struct MortonRemapOutput {
    IWritableRemapProvider* orderProvider{nullptr};
    IWritableRemapProvider* inverseProvider{nullptr};
    std::size_t nextIndex{0};

    bool Append(std::span<const IndexType> order, std::string* error = nullptr) {
        if (orderProvider == nullptr) {
            return validation::AssignError(error, "Morton remap output provider is null");
        }
        if (!orderProvider->AppendRange(order, error)) {
            return false;
        }
        if (inverseProvider != nullptr) {
            for (std::size_t index = 0; index < order.size(); ++index) {
                if (!inverseProvider->WriteAt(
                        static_cast<std::size_t>(order[index]),
                        static_cast<IndexType>(nextIndex + index),
                        error)) {
                    return false;
                }
            }
        }
        nextIndex += order.size();
        return true;
    }
};

inline bool AppendRunSegmentByMortonKey(
    const RemapScratchRun& run,
    const std::size_t recordBegin,
    const std::size_t recordCount,
    const std::size_t leafBudgetElements,
    const std::size_t runBufferBytes,
    MortonRemapOutput& output,
    std::vector<IndexType>& scratch,
    std::vector<IndexType>& orderedElements,
    std::vector<std::uint16_t>& lowBuckets,
    std::vector<std::size_t>& lowCounts,
    std::vector<std::size_t>& lowOffsets,
    std::vector<std::uint32_t>& touchedLowBuckets,
    std::string* error = nullptr) {
    auto fail = [&]() {
        ResetLowWorkspace(touchedLowBuckets, lowCounts, lowOffsets);
        return false;
    };

    if (!CountRunSegmentLowKeys(
            run,
            recordBegin,
            recordCount,
            runBufferBytes,
            lowCounts,
            touchedLowBuckets,
            error)) {
        return fail();
    }

    const auto leaves = BuildLowKeyLeaves(lowCounts, leafBudgetElements);
    std::size_t outputOffset = 0u;
    for (const auto& leaf : leaves) {
        if (leaf.count == 0u) {
            continue;
        }
        const auto isSingleLowKeyLeaf =
            static_cast<std::size_t>(leaf.end) == static_cast<std::size_t>(leaf.begin) + 1u;
        if (leaf.count > leafBudgetElements && isSingleLowKeyLeaf) {
            std::uint64_t emitted = 0u;
            while (emitted < leaf.count) {
                const auto currentCount = static_cast<std::size_t>(
                    std::min<std::uint64_t>(leaf.count - emitted, leafBudgetElements));
                if (!ReadRunSegmentSingleLowKeyChunk(
                        run,
                        recordBegin,
                        recordCount,
                        static_cast<std::uint16_t>(leaf.begin),
                        emitted,
                        currentCount,
                        runBufferBytes,
                        scratch,
                        error)) {
                    return fail();
                }
                if (!output.Append(std::span<const IndexType>(scratch.data(), scratch.size()), error)) {
                    return fail();
                }
                outputOffset += scratch.size();
                emitted += scratch.size();
            }
            continue;
        }
        if (!ReadRunSegmentLeaf(
                run,
                recordBegin,
                recordCount,
                leaf,
                runBufferBytes,
                scratch,
                lowBuckets,
                error)) {
            return fail();
        }
        BuildBufferedLeafOrder(
            leaf,
            scratch,
            lowBuckets,
            lowCounts,
            lowOffsets,
            orderedElements);
        if (!output.Append(std::span<const IndexType>(orderedElements.data(), orderedElements.size()), error)) {
            return fail();
        }
        outputOffset += static_cast<std::size_t>(leaf.count);
    }

    ResetLowWorkspace(touchedLowBuckets, lowCounts, lowOffsets);
    return outputOffset == recordCount;
}

template<typename TKeyGetter>
inline bool BuildInMemoryMortonRemapProvider(
    const std::size_t elementCount,
    TKeyGetter&& keyGetter,
    MortonRemapResult& result,
    const MortonRemapOptions& options,
    std::string* error) {
    std::vector<MortonKeyedIndex> keyed;
    keyed.reserve(elementCount);
    for (std::size_t elementIndex = 0; elementIndex < elementCount; ++elementIndex) {
        keyed.push_back(MortonKeyedIndex{
            .key = keyGetter(elementIndex),
            .index = static_cast<IndexType>(elementIndex),
        });
    }
    std::stable_sort(
        keyed.begin(),
        keyed.end(),
        [](const MortonKeyedIndex& lhs, const MortonKeyedIndex& rhs) {
            if (lhs.key != rhs.key) {
                return lhs.key < rhs.key;
            }
            return lhs.index < rhs.index;
        });

    std::vector<IndexType> order;
    order.reserve(elementCount);
    for (const auto& entry : keyed) {
        order.push_back(entry.index);
    }
    InvokeResource(
        options,
        "in_memory_order",
        VectorCapacityBytes(keyed) + VectorCapacityBytes(order));

    const auto makeLabel = [&options](const std::string_view suffix) {
        if (options.resourcePrefix.empty()) {
            return std::string("remap_morton_") + std::string(suffix);
        }
        return options.resourcePrefix + "." + std::string(suffix);
    };
    const auto publishProvider = [&options, &makeLabel, error](
        std::vector<IndexType>& values,
        const std::string_view suffix,
        std::shared_ptr<IRemapProvider>& target) -> bool {
        if (!options.providerFactory) {
            target = MakeVectorRemapProvider(std::move(values));
            return true;
        }
        auto provider = options.providerFactory(
            values.size(),
            false,
            makeLabel(suffix),
            error);
        if (provider == nullptr) {
            if (error != nullptr && error->empty()) {
                validation::AssignError(error, "failed to create in-memory Morton remap provider");
            }
            return false;
        }
        if (!provider->AppendRange(std::span<const IndexType>(values.data(), values.size()), error) ||
            !provider->EndWrite(error)) {
            provider->Release();
            return false;
        }
        target = std::move(provider);
        std::vector<IndexType>().swap(values);
        return true;
    };

    if (options.buildInverse) {
        std::vector<IndexType> inverse(elementCount);
        for (std::size_t rank = 0; rank < order.size(); ++rank) {
            inverse[static_cast<std::size_t>(order[rank])] = static_cast<IndexType>(rank);
        }
        InvokeResource(
            options,
            "in_memory_inverse",
            VectorCapacityBytes(inverse));
        if (!publishProvider(inverse, "inverse", result.inverseProvider)) {
            return false;
        }
    }
    if (!publishProvider(order, "order", result.orderProvider)) {
        if (result.inverseProvider != nullptr) {
            result.inverseProvider->Release();
            result.inverseProvider.reset();
        }
        return false;
    }
    InvokeProgress(options, 1.0);
    return true;
}

template<typename TKeyGetter>
inline bool BuildMortonRemapProvider(
    const std::size_t elementCount,
    TKeyGetter&& keyGetter,
    MortonRemapResult& result,
    const MortonRemapOptions& options = {},
    std::string* error = nullptr) {
    result = {};
    if (elementCount > static_cast<std::size_t>(std::numeric_limits<IndexType>::max())) {
        return validation::AssignError(error, "Morton remap element count exceeds IndexType range");
    }
    if (elementCount <= 1u) {
        result.orderProvider = std::make_shared<IdentityRemapProvider>(elementCount);
        if (options.buildInverse) {
            result.inverseProvider = std::make_shared<IdentityRemapProvider>(elementCount);
        }
        InvokeProgress(options, 1.0);
        return true;
    }
    const auto resolvedLeafBudgetBytes = ResolveRemapLeafBudgetBytes(options.leafBudgetBytes);
    const auto resolvedRunBufferBytes = ResolveRunBufferBytes(options.runBufferBytes);
    const auto leafBudgetElements = ResolveLeafBudgetElements(resolvedLeafBudgetBytes);
    if (elementCount <= leafBudgetElements) {
        const auto keyedBytes = validation::SaturatingMulU64(
            static_cast<std::uint64_t>(elementCount),
            sizeof(MortonKeyedIndex));
        const auto orderBytes = validation::SaturatingMulU64(
            static_cast<std::uint64_t>(elementCount),
            sizeof(IndexType) * (options.buildInverse ? 2u : 1u));
        const auto inMemoryWorkBytes = validation::SaturatingAddU64(keyedBytes, orderBytes);
        auto scratchLease = options.scratchBudget != nullptr
            ? options.scratchBudget->Acquire(inMemoryWorkBytes)
            : resource::ActiveByteBudget::Lease{};
        return BuildInMemoryMortonRemapProvider(
            elementCount,
            std::forward<TKeyGetter>(keyGetter),
            result,
            options,
            error);
    }
    if (options.byteStoreSession == nullptr) {
        return validation::AssignError(error, "Morton remap requires a byte store session");
    }
    if (!options.providerFactory) {
        return validation::AssignError(error, "Morton remap requires a writable remap provider factory");
    }

    const auto remapWorkBytes = EstimateMortonWorkspaceBytes(
        elementCount,
        resolvedLeafBudgetBytes,
        resolvedRunBufferBytes);
    InvokeResource(options, "work_budget_requested", remapWorkBytes);
    auto scratchLease = options.scratchBudget != nullptr
        ? options.scratchBudget->Acquire(remapWorkBytes)
        : resource::ActiveByteBudget::Lease{};
    InvokeResource(options, "run_buffer_bytes", static_cast<std::uint64_t>(resolvedRunBufferBytes));

    const auto keyCacheBytes = validation::SaturatingMulU64(
        static_cast<std::uint64_t>(elementCount),
        sizeof(std::uint32_t));
    resource::ActiveByteBudget::Lease keyCacheLease;
    bool useKeyCache = options.scratchBudget == nullptr;
    if (options.scratchBudget != nullptr) {
        auto lease = options.scratchBudget->TryAcquire(keyCacheBytes);
        if (lease.has_value()) {
            keyCacheLease = std::move(*lease);
            useKeyCache = true;
        }
    }
    std::vector<std::uint32_t> keyCache;
    if (useKeyCache) {
        keyCache.resize(elementCount);
    }
    InvokeResource(options, "key_cache", useKeyCache ? keyCacheBytes : 0u);

    std::vector<std::size_t> highCounts(kMortonBucketCount16, 0u);
    std::vector<std::size_t> highOffsets(kMortonBucketCount16, 0u);
    std::vector<std::size_t> highWriteOffsets(kMortonBucketCount16, 0u);
    InvokeResource(
        options,
        "allocate_high_tables",
        VectorCapacityBytes(highCounts) + VectorCapacityBytes(highOffsets) + VectorCapacityBytes(highWriteOffsets));

    const auto progressStep = std::max<std::size_t>(elementCount / 64u, 1u);
    for (std::size_t elementIndex = 0; elementIndex < elementCount; ++elementIndex) {
        const auto key = keyGetter(elementIndex);
        if (useKeyCache) {
            keyCache[elementIndex] = key;
        }
        const auto highBucket = static_cast<std::uint16_t>((key >> 16u) & kMortonBucketMask16);
        highCounts[highBucket]++;
        if ((elementIndex + 1u) % progressStep == 0u || elementIndex + 1u == elementCount) {
            InvokeProgress(
                options,
                0.10 + 0.25 * (static_cast<double>(elementIndex + 1u) / static_cast<double>(elementCount)));
        }
    }
    InvokeResource(
        options,
        "key_build",
        VectorCapacityBytes(highCounts) + VectorCapacityBytes(highOffsets) + VectorCapacityBytes(highWriteOffsets));

    std::size_t maxHighBucketSize = 0u;
    std::size_t nonEmptyHighBuckets = 0u;
    std::size_t runningOffset = 0u;
    for (std::size_t bucket = 0; bucket < kMortonBucketCount16; ++bucket) {
        const auto bucketSize = highCounts[bucket];
        highOffsets[bucket] = runningOffset;
        highWriteOffsets[bucket] = runningOffset;
        runningOffset += bucketSize;
        if (bucketSize > 0u) {
            ++nonEmptyHighBuckets;
            maxHighBucketSize = std::max(maxHighBucketSize, bucketSize);
        }
    }
    InvokeResource(options, "non_empty_high_buckets", static_cast<std::uint64_t>(nonEmptyHighBuckets));
    InvokeResource(options, "max_high_bucket_size", static_cast<std::uint64_t>(maxHighBucketSize));

    auto& byteStoreSession = *options.byteStoreSession;
    RemapScratchSpooler scratchSpooler(byteStoreSession, options.useMemoryScratchStore);
    RemapScratchRun highRun;
    const auto highRunKeyGetter = [&](const std::size_t elementIndex) {
        return useKeyCache ? keyCache[elementIndex] : keyGetter(elementIndex);
    };

    if (!WriteHighBucketRun(
            elementCount,
            highRunKeyGetter,
            highCounts,
            highOffsets,
            highWriteOffsets,
            scratchSpooler,
            highRun,
            options,
            error)) {
        return false;
    }
    std::vector<std::uint32_t>().swap(keyCache);
    keyCacheLease.Release();
    InvokeProgress(options, 0.50);
    InvokeResource(
        options,
        "radix_high",
        VectorCapacityBytes(highCounts) + VectorCapacityBytes(highOffsets) + VectorCapacityBytes(highWriteOffsets));

    ReleaseVectorStorage(highWriteOffsets);
    InvokeResource(
        options,
        "release_high_write_offsets",
        VectorCapacityBytes(highCounts) + VectorCapacityBytes(highOffsets));

    const auto maxBufferedBucketSize = std::min<std::size_t>(maxHighBucketSize, leafBudgetElements);
    InvokeResource(
        options,
        "low_leaf_budget",
        static_cast<std::uint64_t>(leafBudgetElements * (sizeof(IndexType) * 2u + sizeof(std::uint16_t))));
    InvokeResource(options, "leaf_budget_elements", static_cast<std::uint64_t>(leafBudgetElements));
    InvokeResource(options, "max_buffered_bucket_size", static_cast<std::uint64_t>(maxBufferedBucketSize));

    std::vector<IndexType> scratch;
    scratch.reserve(maxBufferedBucketSize);
    std::vector<IndexType> orderedElements;
    orderedElements.reserve(maxBufferedBucketSize);
    std::vector<std::uint16_t> lowBuckets;
    lowBuckets.reserve(maxBufferedBucketSize);
    std::vector<std::size_t> lowCounts(kMortonBucketCount16, 0u);
    std::vector<std::size_t> lowOffsets(kMortonBucketCount16, 0u);
    std::vector<std::uint32_t> touchedLowBuckets;
    touchedLowBuckets.reserve(std::min<std::size_t>(maxBufferedBucketSize, kMortonBucketCount16));
    InvokeResource(
        options,
        "allocate_scratch",
        VectorCapacityBytes(scratch) + VectorCapacityBytes(orderedElements) + VectorCapacityBytes(lowBuckets) +
            VectorCapacityBytes(highCounts) + VectorCapacityBytes(highOffsets) + VectorCapacityBytes(lowCounts) +
            VectorCapacityBytes(lowOffsets) + VectorCapacityBytes(touchedLowBuckets));

    const auto makeWritableProvider = [&](const bool randomWrite, const char* label)
        -> std::shared_ptr<IWritableRemapProvider> {
        return options.providerFactory(elementCount, randomWrite, label, error);
    };

    auto provider = makeWritableProvider(false, "order");
    if (provider == nullptr) {
        return false;
    }
    std::shared_ptr<IWritableRemapProvider> inverseProvider;
    if (options.buildInverse) {
        inverseProvider = makeWritableProvider(true, "inverse");
        if (inverseProvider == nullptr) {
            provider->Release();
            return false;
        }
    }

    MortonRemapOutput output{
        .orderProvider = provider.get(),
        .inverseProvider = inverseProvider.get(),
        .nextIndex = 0u,
    };

    std::size_t processedHighBuckets = 0u;
    std::size_t spilledHighBuckets = 0u;
    std::size_t largestSpilledBucket = 0u;
    for (std::size_t highBucket = 0; highBucket < kMortonBucketCount16; ++highBucket) {
        const auto bucketSize = highCounts[highBucket];
        if (bucketSize == 0u) {
            continue;
        }
        const auto bucketBegin = highOffsets[highBucket];
        if (bucketSize > leafBudgetElements) {
            ++spilledHighBuckets;
            largestSpilledBucket = std::max(largestSpilledBucket, bucketSize);
        }
        if (!AppendRunSegmentByMortonKey(
                highRun,
                bucketBegin,
                bucketSize,
                leafBudgetElements,
                resolvedRunBufferBytes,
                output,
                scratch,
                orderedElements,
                lowBuckets,
                lowCounts,
                lowOffsets,
                touchedLowBuckets,
                error)) {
            provider->Release();
            if (inverseProvider != nullptr) {
                inverseProvider->Release();
            }
            return false;
        }

        ++processedHighBuckets;
        if (processedHighBuckets % 512u == 0u || processedHighBuckets == nonEmptyHighBuckets) {
            InvokeProgress(
                options,
                0.50 + 0.45 *
                    (static_cast<double>(processedHighBuckets) /
                     static_cast<double>(std::max<std::size_t>(nonEmptyHighBuckets, 1u))));
        }
    }
    InvokeResource(
        options,
        "radix_low",
        VectorCapacityBytes(scratch) + VectorCapacityBytes(orderedElements) + VectorCapacityBytes(lowBuckets) +
            VectorCapacityBytes(highCounts) + VectorCapacityBytes(highOffsets) + VectorCapacityBytes(lowCounts) +
            VectorCapacityBytes(lowOffsets) + VectorCapacityBytes(touchedLowBuckets));
    InvokeResource(options, "spilled_high_buckets", static_cast<std::uint64_t>(spilledHighBuckets));
    InvokeResource(options, "largest_spilled_bucket", static_cast<std::uint64_t>(largestSpilledBucket));

    if (!provider->EndWrite(error)) {
        provider->Release();
        if (inverseProvider != nullptr) {
            inverseProvider->Release();
        }
        return false;
    }
    if (inverseProvider != nullptr && !inverseProvider->EndRandomWrite(error)) {
        provider->Release();
        inverseProvider->Release();
        return false;
    }

    highRun.Release();
    ReleaseVectorStorage(scratch);
    ReleaseVectorStorage(orderedElements);
    ReleaseVectorStorage(lowBuckets);
    ReleaseVectorStorage(highCounts);
    ReleaseVectorStorage(highOffsets);
    ReleaseVectorStorage(lowCounts);
    ReleaseVectorStorage(lowOffsets);
    ReleaseVectorStorage(touchedLowBuckets);
    InvokeResource(options, "release_temp", 0u);
    InvokeProgress(options, 1.0);

    result.orderProvider = std::move(provider);
    result.inverseProvider = std::move(inverseProvider);
    return true;
}

} // namespace mortonremap
} // namespace datacodec

#endif
