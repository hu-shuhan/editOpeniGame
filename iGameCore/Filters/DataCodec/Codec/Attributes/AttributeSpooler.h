#ifndef DATACODEC_CODEC_ATTRIBUTES_ATTRIBUTESPOOLER_H
#define DATACODEC_CODEC_ATTRIBUTES_ATTRIBUTESPOOLER_H

#include "DataCodec/Storage/ByteIO/ByteSource.h"
#include "DataCodec/Common/DataCodecTypes.h"
#include "DataCodec/Validation/Common/DataCodecValidation.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <numeric>
#include <span>
#include <stop_token>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

class AttributeSpooler : public bytestore::IByteSource {
private:
    static constexpr std::uint8_t kRecordPending = 0u;
    static constexpr std::uint8_t kRecordReady = 1u;
    static constexpr std::uint8_t kRecordAborted = 2u;

    struct RecordSnapshot {
        std::shared_ptr<bytestore::IByteSource> source;
        std::uint64_t payloadBytes{0u};
    };

public:
    AttributeSpooler(
        const std::size_t recordCount,
        std::stop_token stopToken = {},
        const bytestore::ByteSourceConsumptionMode replayMode = bytestore::ByteSourceConsumptionMode::OneShot)
        : m_sources(recordCount),
          m_ready(recordCount),
          m_recordOrder(recordCount),
          m_stopToken(std::move(stopToken)),
          m_replayMode(replayMode) {
        for (auto& ready : m_ready) {
            ready.store(kRecordPending, std::memory_order_relaxed);
        }
        std::iota(m_recordOrder.begin(), m_recordOrder.end(), 0u);
    }

    ~AttributeSpooler() override { Release(); }

    bool SetRecordOrder(std::vector<std::size_t> recordOrder, std::string* error = nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (recordOrder.size() != m_sources.size()) {
            return validation::AssignError(error, "attribute record order size does not match record count");
        }
        std::vector<std::uint8_t> seen(recordOrder.size(), 0u);
        for (const auto index : recordOrder) {
            if (index >= recordOrder.size() || seen[index] != 0u) {
                return validation::AssignError(error, "attribute record order is not a valid permutation");
            }
            seen[index] = 1u;
        }
        m_recordOrder = std::move(recordOrder);
        return true;
    }

    bool SetRecordSource(
        const std::size_t index,
        std::shared_ptr<bytestore::IByteSource> source,
        std::string* error = nullptr) {
        if (source == nullptr) {
            return validation::AssignError(error, "attribute record source is null");
        }
        const auto payloadBytes = source->ByteSizeHint();
        if (bytestore::IsUnknownByteSize(payloadBytes)) {
            return validation::AssignError(error, "attribute record source payload size is unknown");
        }
        std::atomic<std::uint8_t>* readyFlag = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!ValidateWritableRecordLocked(index, error)) {
                return false;
            }
            m_sources[index] = std::move(source);
            readyFlag = &m_ready[index];
            m_knownByteSize.fetch_add(payloadBytes, std::memory_order_acq_rel);
            readyFlag->store(kRecordReady, std::memory_order_release);
        }
        readyFlag->notify_all();
        return true;
    }

    [[nodiscard]] std::size_t MissingRecordCount() const noexcept {
        std::size_t missing = 0u;
        for (const auto& ready : m_ready) {
            if (ready.load(std::memory_order_acquire) != kRecordReady) {
                ++missing;
            }
        }
        return missing;
    }

    bool WaitUntilRecordReady(const std::size_t index, std::string* error = nullptr) const {
        const std::atomic<std::uint8_t>* readyFlag = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (index >= m_ready.size()) {
                return validation::AssignError(error, "attribute record index is out of range");
            }
            readyFlag = &m_ready[index];
        }

        std::stop_callback stopCallback(m_stopToken, [readyFlag]() {
            const_cast<std::atomic<std::uint8_t>*>(readyFlag)->notify_all();
        });
        for (;;) {
            const auto value = readyFlag->load(std::memory_order_acquire);
            if (value == kRecordReady) {
                return true;
            }
            if (value == kRecordAborted || m_stopToken.stop_requested()) {
                return validation::AssignError(error, "attribute record stream stopped while waiting for a field");
            }
            readyFlag->wait(value, std::memory_order_acquire);
        }
    }

    [[nodiscard]] std::size_t RecordCount() const noexcept {
        return m_sources.size();
    }

    bool OrderedRecordSources(
        std::vector<std::shared_ptr<bytestore::IByteSource>>& sources,
        std::string* error = nullptr) const {
        sources.clear();
        std::vector<RecordSnapshot> records;
        if (!BuildRecordSnapshot(records, error)) {
            return false;
        }
        sources.reserve(records.size());
        for (auto& record : records) {
            sources.push_back(std::move(record.source));
        }
        return true;
    }

    [[nodiscard]] std::uint64_t ByteSizeHint() const noexcept override {
        return MissingRecordCount() == 0u
            ? m_knownByteSize.load(std::memory_order_acquire)
            : bytestore::kUnknownByteSize;
    }

    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uint64_t residentBytes =
            VectorCapacityBytes(m_sources) +
            VectorCapacityBytes(m_ready) +
            VectorCapacityBytes(m_recordOrder);
        for (const auto& source : m_sources) {
            if (source != nullptr) {
                residentBytes = validation::SaturatingAddU64(residentBytes, source->ResidentSizeHint());
            }
        }
        return residentBytes;
    }

    [[nodiscard]] std::uint64_t MappedSizeHint() const noexcept override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::uint64_t mappedBytes = 0u;
        for (const auto& source : m_sources) {
            if (source != nullptr) {
                mappedBytes = validation::SaturatingAddU64(mappedBytes, source->MappedSizeHint());
            }
        }
        return mappedBytes;
    }

    [[nodiscard]] bool PreferDirectCopy() const noexcept override { return true; }

    [[nodiscard]] bool CanRead() const noexcept override {
        if (MissingRecordCount() != 0u) {
            return false;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_replayMode == bytestore::ByteSourceConsumptionMode::OneShot && m_consumed) {
            return false;
        }
        if (m_recordOrder.size() != m_sources.size()) {
            return false;
        }
        for (const auto index : m_recordOrder) {
            if (index >= m_sources.size() || m_sources[index] == nullptr || !m_sources[index]->CanRead()) {
                return false;
            }
        }
        return true;
    }

    bool Read(
        const std::uint64_t offset,
        const std::span<std::uint8_t> output,
        std::string* error = nullptr) const override {
        std::vector<RecordSnapshot> records;
        if (!BuildRecordSnapshot(records, error)) {
            return false;
        }
        const auto byteSize = ByteSizeHint();
        if (bytestore::IsUnknownByteSize(byteSize) ||
            offset > byteSize ||
            output.size() > byteSize - offset) {
            return validation::AssignError(error, "attribute spooler read is outside the cache range");
        }

        std::uint64_t requestEnd = 0u;
        if (!validation::CheckedAddU64(
                offset,
                static_cast<std::uint64_t>(output.size()),
                requestEnd,
                "attribute spooler request end",
                error)) {
            return false;
        }
        std::uint64_t cacheOffset = 0u;
        std::size_t outputOffset = 0u;
        const auto copySegment = [&](
            const std::uint64_t segmentBytes,
            const auto& readSegment) -> bool {
            const auto segmentBegin = cacheOffset;
            std::uint64_t segmentEnd = 0u;
            if (!validation::CheckedAddU64(
                    segmentBegin,
                    segmentBytes,
                    segmentEnd,
                    "attribute spooler segment end",
                    error)) {
                return false;
            }
            cacheOffset = segmentEnd;
            if (segmentBytes == 0u || requestEnd <= segmentBegin || offset >= segmentEnd) {
                return true;
            }
            const auto readBegin = std::max(offset, segmentBegin);
            const auto readEnd = std::min(requestEnd, segmentEnd);
            const auto readBytes = static_cast<std::size_t>(readEnd - readBegin);
            const auto segmentReadOffset = readBegin - segmentBegin;
            auto target = output.subspan(outputOffset, readBytes);
            if (!readSegment(segmentReadOffset, target)) {
                return false;
            }
            return validation::CheckedAddSizeT(
                outputOffset,
                readBytes,
                outputOffset,
                "attribute spooler output offset",
                error);
        };

        for (const auto& record : records) {
            if (outputOffset >= output.size()) {
                break;
            }
            if (!copySegment(
                    record.payloadBytes,
                    [&](const std::uint64_t segmentOffset, const std::span<std::uint8_t> target) {
                        return record.source->Read(segmentOffset, target, error);
                    })) {
                return false;
            }
        }
        if (outputOffset != output.size()) {
            return validation::AssignError(error, "attribute spooler read did not fill the requested range");
        }
        return true;
    }

    bool CopyTo(bytestore::IByteWriter& writer, std::string* error = nullptr) override {
        std::vector<RecordSnapshot> records;
        if (!BuildRecordSnapshot(records, error)) {
            return false;
        }

        std::uint64_t copiedBytes = 0u;
        for (const auto& record : records) {
            if (record.source == nullptr) {
                return validation::AssignError(error, "attribute record source is missing");
            }
            if (!record.source->CopyTo(writer, error)) {
                return false;
            }
            if (!validation::CheckedAddU64(
                    copiedBytes,
                    record.payloadBytes,
                    copiedBytes,
                    "attribute spooler copied bytes",
                    error)) {
                return false;
            }
        }
        m_finalByteSize.store(copiedBytes, std::memory_order_release);
        if (m_replayMode == bytestore::ByteSourceConsumptionMode::OneShot) {
            ReleaseSourcesAfterCopy();
        }
        return true;
    }

    void Release() noexcept override {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& source : m_sources) {
            if (source != nullptr) {
                source->Release();
                source.reset();
            }
        }
        for (auto& ready : m_ready) {
            ready.store(kRecordAborted, std::memory_order_release);
            ready.notify_all();
        }
        m_knownByteSize.store(0u, std::memory_order_release);
        m_finalByteSize.store(0u, std::memory_order_release);
        m_consumed = true;
    }

private:
    bool BuildRecordSnapshot(
        std::vector<RecordSnapshot>& records,
        std::string* error) const {
        records.clear();
        std::vector<std::size_t> recordOrder;
        std::size_t sourceCount = 0u;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_replayMode == bytestore::ByteSourceConsumptionMode::OneShot && m_consumed) {
                return validation::AssignError(error, "one-shot attribute spooler was already consumed");
            }
            recordOrder = m_recordOrder;
            sourceCount = m_sources.size();
        }
        if (recordOrder.size() != sourceCount) {
            return validation::AssignError(error, "attribute record order is incomplete");
        }
        for (const auto index : recordOrder) {
            if (!WaitUntilRecordReady(index, error)) {
                return false;
            }
            std::shared_ptr<bytestore::IByteSource> source;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (index >= m_sources.size()) {
                    return validation::AssignError(error, "attribute record index is out of range");
                }
                source = m_sources[index];
            }
            if (source == nullptr) {
                return validation::AssignError(error, "attribute record source is missing");
            }
            if (!source->CanRead()) {
                return validation::AssignError(error, "attribute record source does not support ranged reads");
            }
            const auto payloadBytes = source->ByteSizeHint();
            if (bytestore::IsUnknownByteSize(payloadBytes)) {
                return validation::AssignError(error, "attribute record source payload size is unknown");
            }
            records.push_back(RecordSnapshot{
                .source = std::move(source),
                .payloadBytes = payloadBytes,
            });
        }
        return true;
    }

    void ReleaseSourcesAfterCopy() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& source : m_sources) {
            if (source != nullptr) {
                source->Release();
                source.reset();
            }
        }
        m_consumed = true;
    }

    bool ValidateWritableRecordLocked(const std::size_t index, std::string* error) const {
        if (index >= m_sources.size()) {
            return validation::AssignError(error, "attribute record index is out of range");
        }
        if (m_consumed) {
            return validation::AssignError(error, "attribute spooler was already released");
        }
        if (m_ready[index].load(std::memory_order_acquire) != kRecordPending) {
            return validation::AssignError(error, "attribute record was already published");
        }
        return true;
    }

    std::vector<std::shared_ptr<bytestore::IByteSource>> m_sources;
    std::vector<std::atomic<std::uint8_t>> m_ready;
    std::vector<std::size_t> m_recordOrder;
    mutable std::mutex m_mutex;
    std::stop_token m_stopToken;
    std::atomic<std::uint64_t> m_knownByteSize{0u};
    std::atomic<std::uint64_t> m_finalByteSize{0u};
    bytestore::ByteSourceConsumptionMode m_replayMode{bytestore::ByteSourceConsumptionMode::OneShot};
    bool m_consumed{false};
};

} // namespace datacodec

#endif

