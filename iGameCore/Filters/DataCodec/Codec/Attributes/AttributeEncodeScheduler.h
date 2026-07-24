#ifndef DATACODEC_CODEC_ATTRIBUTES_ATTRIBUTEENCODESCHEDULER_H
#define DATACODEC_CODEC_ATTRIBUTES_ATTRIBUTEENCODESCHEDULER_H

#include "DataCodec/Storage/ByteStore/ByteStore.h"
#include "DataCodec/Storage/ByteIO/ByteBudget.h"
#include "DataCodec/Common/DataCodecCallback.h"
#include "DataCodec/API/Params/CodecControlParams.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
namespace datacodec {

using AttributeByteQuotaStats = resource::ActiveByteBudgetStats;
using AttributeByteQuota = resource::ActiveByteBudget;

struct AttributeLaneGateStats {
    std::uint32_t laneCount{0u};
    std::uint32_t active{0u};
    std::uint32_t peakActive{0u};
    std::uint64_t acquireCount{0u};
    std::uint64_t waitCount{0u};
    std::uint64_t totalWaitNanoseconds{0u};
};

struct AttributeEncodeSchedulerStats {
    AttributeByteQuotaStats scratch;
    AttributeByteQuotaStats staging;
    AttributeLaneGateStats pressioLane;
    AttributeLaneGateStats referenceLane;
    std::uint64_t pressioCallCount{0u};
    std::uint64_t pressioTotalNanoseconds{0u};
    std::uint64_t pressioMaxNanoseconds{0u};
};

class AttributeLaneGate final {
public:
    class Lease final {
    public:
        Lease() = default;
        explicit Lease(AttributeLaneGate* gate) noexcept
            : m_gate(gate) {}

        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;

        Lease(Lease&& other) noexcept { MoveFrom(std::move(other)); }

        Lease& operator=(Lease&& other) noexcept {
            if (this != &other) {
                Release();
                MoveFrom(std::move(other));
            }
            return *this;
        }

        ~Lease() { Release(); }

        void Release() noexcept {
            if (m_gate != nullptr) {
                m_gate->Release();
                m_gate = nullptr;
            }
        }

    private:
        void MoveFrom(Lease&& other) noexcept {
            m_gate = other.m_gate;
            other.m_gate = nullptr;
        }

        AttributeLaneGate* m_gate{nullptr};
    };

    explicit AttributeLaneGate(const std::uint32_t lanes = 1u) {
        Reset(lanes);
    }

    void Reset(
        const std::uint32_t lanes,
        const bool collectTiming = false) noexcept {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lanes = std::max<std::uint32_t>(lanes, 1u);
            m_collectTiming = collectTiming;
            m_active = 0u;
            m_peakActive = 0u;
            m_acquireCount = 0u;
            m_waitCount = 0u;
            m_totalWaitNanoseconds = 0u;
        }
        m_condition.notify_all();
    }

    [[nodiscard]] Lease Acquire() {
        std::unique_lock<std::mutex> lock(m_mutex);
        const auto shouldWait = m_active >= m_lanes;
        const auto collectWaitTiming = shouldWait && m_collectTiming;
        const auto waitStart = callback::StartTiming(collectWaitTiming);
        m_condition.wait(lock, [this]() {
            return m_active < m_lanes;
        });
        if (shouldWait) {
            ++m_waitCount;
            if (collectWaitTiming) {
                m_totalWaitNanoseconds += static_cast<std::uint64_t>(
                    callback::ElapsedNanoseconds(waitStart).count());
            }
        }
        ++m_acquireCount;
        ++m_active;
        m_peakActive = std::max(m_peakActive, m_active);
        return Lease(this);
    }

    [[nodiscard]] std::uint32_t Active() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_active;
    }

    [[nodiscard]] std::uint32_t PeakActive() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_peakActive;
    }

    [[nodiscard]] std::uint32_t LaneCount() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_lanes;
    }

    [[nodiscard]] AttributeLaneGateStats SnapshotStats() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return AttributeLaneGateStats{
            .laneCount = m_lanes,
            .active = m_active,
            .peakActive = m_peakActive,
            .acquireCount = m_acquireCount,
            .waitCount = m_waitCount,
            .totalWaitNanoseconds = m_totalWaitNanoseconds,
        };
    }

private:
    void Release() noexcept {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_active != 0u) {
                --m_active;
            }
        }
        m_condition.notify_all();
    }

    std::uint32_t m_lanes{1u};
    bool m_collectTiming{false};
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::uint32_t m_active{0u};
    std::uint32_t m_peakActive{0u};
    std::uint64_t m_acquireCount{0u};
    std::uint64_t m_waitCount{0u};
    std::uint64_t m_totalWaitNanoseconds{0u};
};

class AttributeEncodeScheduler final {
public:
    explicit AttributeEncodeScheduler(
        const ResourceBudgetControlParams* params = nullptr,
        const bool collectTiming = false) {
        Configure(params, collectTiming);
    }

    void Configure(
        const ResourceBudgetControlParams* params,
        const bool collectTiming = false) {
        ResourceBudgetControlParams defaults;
        const auto& resolved = params != nullptr ? *params : defaults;
        m_scratchQuota.Reset(resolved.AttributeScratchQuotaBytes(), collectTiming);
        m_stagingStorageMode = resolved.AttributeEncodeStagingStorageMode();
        m_stagingQuota.Reset(
            resolved.AttributeStagingQuotaBytes(m_stagingStorageMode),
            collectTiming);
        m_pressioLane.Reset(resolved.AttributePressioLaneCount(), collectTiming);
        m_referenceLane.Reset(resolved.AttributeReferenceLaneCount(), collectTiming);
        m_transferCacheStorageMode = resolved.AttributeEncodeTransferCacheStorageMode();
        m_collectTiming = collectTiming;
        {
            std::lock_guard<std::mutex> lock(m_statsMutex);
            m_pressioCallCount = 0u;
            m_pressioTotalNanoseconds = 0u;
            m_pressioMaxNanoseconds = 0u;
        }
    }

    [[nodiscard]] AttributeByteQuota::Lease AcquireScratch(const std::uint64_t bytes) {
        return m_scratchQuota.Acquire(bytes);
    }

    [[nodiscard]] AttributeByteQuota::Lease AcquireStaging(const std::uint64_t bytes) {
        return m_stagingQuota.Acquire(bytes);
    }

    [[nodiscard]] AttributeLaneGate::Lease AcquirePressioLane() {
        return m_pressioLane.Acquire();
    }

    [[nodiscard]] AttributeLaneGate::Lease AcquireReferenceLane() {
        return m_referenceLane.Acquire();
    }

    void RecordPressioDuration(const std::chrono::nanoseconds duration) noexcept {
        if (!m_collectTiming) {
            return;
        }
        const auto nanoseconds = static_cast<std::uint64_t>(std::max<std::int64_t>(duration.count(), 0));
        std::lock_guard<std::mutex> lock(m_statsMutex);
        ++m_pressioCallCount;
        m_pressioTotalNanoseconds += nanoseconds;
        m_pressioMaxNanoseconds = std::max(m_pressioMaxNanoseconds, nanoseconds);
    }

    [[nodiscard]] EncodeStorageMode TransferCacheStorageMode() const noexcept {
        return m_transferCacheStorageMode;
    }
    [[nodiscard]] EncodeStorageMode StagingStorageMode() const noexcept {
        return m_stagingStorageMode;
    }
    [[nodiscard]] bool CollectTiming() const noexcept { return m_collectTiming; }
    [[nodiscard]] const AttributeByteQuota& ScratchQuota() const noexcept { return m_scratchQuota; }
    [[nodiscard]] const AttributeByteQuota& StagingQuota() const noexcept { return m_stagingQuota; }
    [[nodiscard]] const AttributeLaneGate& PressioLane() const noexcept { return m_pressioLane; }
    [[nodiscard]] const AttributeLaneGate& ReferenceLane() const noexcept { return m_referenceLane; }
    [[nodiscard]] AttributeEncodeSchedulerStats SnapshotStats() const noexcept {
        AttributeEncodeSchedulerStats stats{
            .scratch = m_scratchQuota.SnapshotStats(),
            .staging = m_stagingQuota.SnapshotStats(),
            .pressioLane = m_pressioLane.SnapshotStats(),
            .referenceLane = m_referenceLane.SnapshotStats(),
        };
        std::lock_guard<std::mutex> lock(m_statsMutex);
        stats.pressioCallCount = m_pressioCallCount;
        stats.pressioTotalNanoseconds = m_pressioTotalNanoseconds;
        stats.pressioMaxNanoseconds = m_pressioMaxNanoseconds;
        return stats;
    }

private:
    AttributeByteQuota m_scratchQuota;
    AttributeByteQuota m_stagingQuota;
    AttributeLaneGate m_pressioLane;
    AttributeLaneGate m_referenceLane;
    EncodeStorageMode m_transferCacheStorageMode{EncodeStorageMode::Managed};
    EncodeStorageMode m_stagingStorageMode{EncodeStorageMode::Managed};
    bool m_collectTiming{false};
    mutable std::mutex m_statsMutex;
    std::uint64_t m_pressioCallCount{0u};
    std::uint64_t m_pressioTotalNanoseconds{0u};
    std::uint64_t m_pressioMaxNanoseconds{0u};
};

class AttributeStagingHandle final {
public:
    AttributeStagingHandle() = default;
    AttributeStagingHandle(
        std::shared_ptr<bytestore::IByteStore> store,
        AttributeByteQuota::Lease stagingLease)
        : m_store(std::move(store)),
          m_stagingLease(std::move(stagingLease)) {}

    AttributeStagingHandle(const AttributeStagingHandle&) = delete;
    AttributeStagingHandle& operator=(const AttributeStagingHandle&) = delete;

    AttributeStagingHandle(AttributeStagingHandle&&) noexcept = default;
    AttributeStagingHandle& operator=(AttributeStagingHandle&&) noexcept = default;

    ~AttributeStagingHandle() { Release(); }

    [[nodiscard]] bytestore::IByteStore* Store() const noexcept { return m_store.get(); }
    [[nodiscard]] std::shared_ptr<bytestore::IByteStore> SharedStore() const noexcept { return m_store; }

    void Release() noexcept {
        if (m_store != nullptr) {
            m_store->Release();
            m_store.reset();
        }
        m_stagingLease.Release();
    }

private:
    std::shared_ptr<bytestore::IByteStore> m_store;
    AttributeByteQuota::Lease m_stagingLease;
};

class AttributeStagingManager final {
public:
    AttributeStagingManager(
        bytestore::ByteStoreSession& session,
        AttributeEncodeScheduler& scheduler)
        : m_session(session),
          m_scheduler(scheduler) {}

    [[nodiscard]] AttributeStagingHandle Create(
        const std::string& label,
        const std::uint64_t logicalBytes,
        std::string* error = nullptr) {
        auto lease = m_scheduler.AcquireStaging(logicalBytes);
        auto store = bytestore::CreateByteStore(
            m_session,
            label,
            m_scheduler.StagingStorageMode() == EncodeStorageMode::Memory,
            error);
        if (store == nullptr) {
            return {};
        }
        return AttributeStagingHandle(std::move(store), std::move(lease));
    }

private:
    bytestore::ByteStoreSession& m_session;
    AttributeEncodeScheduler& m_scheduler;
};

} // namespace datacodec

#endif
