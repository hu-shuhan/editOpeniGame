#ifndef DATACODEC_RUNTIME_WORKSPACE_ENCODETRANSFERCACHESET_H
#define DATACODEC_RUNTIME_WORKSPACE_ENCODETRANSFERCACHESET_H

#include "DataCodec/Storage/Common/BinaryValueIO.h"
#include "DataCodec/Storage/ByteIO/ByteSource.h"
#include "DataCodec/Runtime/Cache/TransferCache/Common/EncodeTransferCache.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <utility>
#include <vector>
namespace datacodec {

enum class EncodeTransferCacheState : std::uint8_t {
    Pending = 0u,
    Ready = 1u,
    Aborted = 2u,
};

class EncodeTransferCacheSet {
public:
    void Clear() {
        for (auto& record : m_transferCaches) {
            if (record.transferCache != nullptr) {
                record.transferCache->Release();
            }
        }
        m_transferCaches.clear();
        m_readyFlags.clear();
    }

    void AbortAll() {
        for (auto& record : m_transferCaches) {
            if (record.transferCache != nullptr) {
                record.transferCache->Release();
                record.transferCache.reset();
            }
        }
        for (std::size_t index = 0; index < m_readyFlags.size(); ++index) {
            StoreStateFlag(index, EncodeTransferCacheState::Aborted);
        }
    }

    [[nodiscard]] std::size_t AddTransferCache(
        const FieldType fieldType,
        std::string label,
        const std::size_t ordinal = 0,
        const AttrAttachment attachment = AttrAttachment::Point,
        const EncodedFieldCodecType codecType = EncodedFieldCodecType::Unknown) {
        EncodeTransferCacheSchedule schedule;
        schedule.fieldType = fieldType;
        schedule.ordinal = ordinal;
        schedule.attachment = attachment;
        schedule.codecType = codecType;
        schedule.label = std::move(label);
        m_transferCaches.push_back(EncodeTransferUnit{std::move(schedule), nullptr});
        m_readyFlags.push_back(std::make_shared<std::atomic<std::uint8_t>>(
            static_cast<std::uint8_t>(EncodeTransferCacheState::Pending)));
        return m_transferCaches.size() - 1u;
    }

    [[nodiscard]] std::size_t Count() const noexcept { return m_transferCaches.size(); }
    [[nodiscard]] EncodeTransferUnit& TransferCache(const std::size_t index) { return m_transferCaches.at(index); }
    [[nodiscard]] const EncodeTransferUnit& TransferCache(const std::size_t index) const { return m_transferCaches.at(index); }

    void PublishTransferCacheBytes(
        const std::size_t index,
        std::vector<std::uint8_t> bytes,
        const EncodedFieldCodecType codecType) {
        PublishTransferCache(
            index,
            std::make_shared<bytestore::VectorByteSource>(std::move(bytes)),
            codecType);
    }

    void PublishTransferCache(
        const std::size_t index,
        std::shared_ptr<bytestore::IByteSource> transferCache,
        const EncodedFieldCodecType codecType) {
        MarkPending(index);
        auto& record = TransferCache(index);
        if (record.transferCache != nullptr) {
            record.transferCache->Release();
        }
        record.schedule.codecType = codecType;
        record.schedule.rawSize = transferCache != nullptr ? transferCache->ByteSizeHint() : 0u;
        record.schedule.compressionType = EncodedFieldCompressionType::None;
        record.transferCache = std::move(transferCache);
        MarkReady(index);
    }

    void MarkReady(const std::size_t index) {
        auto& record = TransferCache(index);
        record.schedule.rawSize = record.transferCache != nullptr ? record.transferCache->ByteSizeHint() : 0u;
        StoreStateFlag(index, EncodeTransferCacheState::Ready);
    }

    void MarkPending(const std::size_t index) {
        StoreStateFlag(index, EncodeTransferCacheState::Pending);
    }

    [[nodiscard]] bool IsReady(const std::size_t index) const {
        return LoadState(index) == EncodeTransferCacheState::Ready;
    }

    bool WaitUntilReady(
        const std::size_t index,
        const std::stop_token& stopToken = {}) const {
        auto& state = *m_readyFlags.at(index);
        std::stop_callback stopCallback(stopToken, [&state]() {
            state.notify_all();
        });
        for (;;) {
            const auto value = state.load(std::memory_order_acquire);
            if (value == static_cast<std::uint8_t>(EncodeTransferCacheState::Ready)) {
                return true;
            }
            if (value == static_cast<std::uint8_t>(EncodeTransferCacheState::Aborted) ||
                stopToken.stop_requested()) {
                return false;
            }
            state.wait(value, std::memory_order_acquire);
        }
    }

    bool WaitUntilComplete(const std::stop_token& stopToken = {}) const {
        for (std::size_t index = 0; index < m_readyFlags.size(); ++index) {
            if (!WaitUntilReady(index, stopToken)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] std::uint64_t ResidentSizeHint() const noexcept {
        std::uint64_t totalBytes = VectorCapacityBytes(m_transferCaches) + VectorCapacityBytes(m_readyFlags);
        for (const auto& record : m_transferCaches) {
            if (record.transferCache != nullptr) {
                totalBytes += record.transferCache->ResidentSizeHint();
            }
        }
        return totalBytes;
    }

private:
    [[nodiscard]] EncodeTransferCacheState LoadState(const std::size_t index) const {
        const auto value = m_readyFlags.at(index)->load(std::memory_order_acquire);
        if (value == static_cast<std::uint8_t>(EncodeTransferCacheState::Ready)) {
            return EncodeTransferCacheState::Ready;
        }
        if (value == static_cast<std::uint8_t>(EncodeTransferCacheState::Aborted)) {
            return EncodeTransferCacheState::Aborted;
        }
        return EncodeTransferCacheState::Pending;
    }

    void StoreStateFlag(const std::size_t index, const EncodeTransferCacheState value) const {
        auto& ready = *m_readyFlags.at(index);
        {
            std::lock_guard<std::mutex> lock(m_readyMutex);
            ready.store(static_cast<std::uint8_t>(value), std::memory_order_release);
        }
        ready.notify_all();
    }

    std::vector<EncodeTransferUnit> m_transferCaches;
    std::vector<std::shared_ptr<std::atomic<std::uint8_t>>> m_readyFlags;
    mutable std::mutex m_readyMutex;
};

} // namespace datacodec

#endif
