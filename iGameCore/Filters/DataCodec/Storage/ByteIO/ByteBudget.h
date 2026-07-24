#ifndef DATACODEC_STORAGE_BYTEIO_BYTEBUDGET_H
#define DATACODEC_STORAGE_BYTEIO_BYTEBUDGET_H

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

namespace datacodec {
namespace resource {

struct ActiveByteBudgetStats {
    std::uint64_t maxActiveBytes{0u};
    std::uint64_t activeBytes{0u};
    std::uint64_t peakActiveBytes{0u};
    std::uint64_t peakRequestedBytes{0u};
    std::uint64_t acquireCount{0u};
    std::uint64_t waitCount{0u};
    std::uint64_t totalWaitNanoseconds{0u};
};

struct ResidentByteBudgetStats {
    std::uint64_t limitBytes{0u};
    std::uint64_t residentBytes{0u};
    std::uint64_t peakResidentBytes{0u};
};

class ResidentByteBudget final {
public:
    explicit ResidentByteBudget(
        const std::uint64_t limitBytes = std::numeric_limits<std::uint64_t>::max()) noexcept
        : m_limitBytes(std::max<std::uint64_t>(limitBytes, 1u)) {}

    void Reset(
        const std::uint64_t limitBytes = std::numeric_limits<std::uint64_t>::max()) noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_limitBytes = std::max<std::uint64_t>(limitBytes, 1u);
        m_residentBytes = 0u;
        m_peakResidentBytes = 0u;
    }

    [[nodiscard]] bool TryReserve(const std::uint64_t bytes) noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (bytes > m_limitBytes - std::min(m_residentBytes, m_limitBytes)) {
            return false;
        }
        m_residentBytes += bytes;
        m_peakResidentBytes = std::max(m_peakResidentBytes, m_residentBytes);
        return true;
    }

    void Release(const std::uint64_t bytes) noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_residentBytes = bytes >= m_residentBytes ? 0u : m_residentBytes - bytes;
    }

    [[nodiscard]] ResidentByteBudgetStats SnapshotStats() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return ResidentByteBudgetStats{
            .limitBytes = m_limitBytes,
            .residentBytes = m_residentBytes,
            .peakResidentBytes = m_peakResidentBytes,
        };
    }

private:
    mutable std::mutex m_mutex;
    std::uint64_t m_limitBytes{std::numeric_limits<std::uint64_t>::max()};
    std::uint64_t m_residentBytes{0u};
    std::uint64_t m_peakResidentBytes{0u};
};

class ActiveByteBudget final {
public:
    class Lease final {
    public:
        Lease() = default;
        Lease(ActiveByteBudget* budget, const std::uint64_t bytes) noexcept
            : m_budget(budget),
              m_bytes(bytes) {}

        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;

        Lease(Lease&& other) noexcept {
            MoveFrom(std::move(other));
        }

        Lease& operator=(Lease&& other) noexcept {
            if (this != &other) {
                Release();
                MoveFrom(std::move(other));
            }
            return *this;
        }

        ~Lease() { Release(); }

        void Release() noexcept {
            if (m_budget != nullptr) {
                m_budget->Release(m_bytes);
                m_budget = nullptr;
                m_bytes = 0u;
            }
        }

        [[nodiscard]] std::uint64_t Bytes() const noexcept { return m_bytes; }
        [[nodiscard]] explicit operator bool() const noexcept { return m_budget != nullptr; }

    private:
        void MoveFrom(Lease&& other) noexcept {
            m_budget = other.m_budget;
            m_bytes = other.m_bytes;
            other.m_budget = nullptr;
            other.m_bytes = 0u;
        }

        ActiveByteBudget* m_budget{nullptr};
        std::uint64_t m_bytes{0u};
    };

    explicit ActiveByteBudget(
        const std::uint64_t maxActiveBytes = 1u,
        const bool collectTiming = false) noexcept {
        Reset(maxActiveBytes, collectTiming);
    }

    void Reset(
        const std::uint64_t maxActiveBytes = 1u,
        const bool collectTiming = false) noexcept {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_maxActiveBytes = std::max<std::uint64_t>(maxActiveBytes, 1u);
            m_collectTiming = collectTiming;
            m_activeBytes = 0u;
            m_peakActiveBytes = 0u;
            m_peakRequestedBytes = 0u;
            m_acquireCount = 0u;
            m_waitCount = 0u;
            m_totalWaitNanoseconds = 0u;
        }
        m_condition.notify_all();
    }

    [[nodiscard]] Lease Acquire(const std::uint64_t requestedBytes) {
        const auto bytes = std::max<std::uint64_t>(requestedBytes, 1u);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_peakRequestedBytes = std::max(m_peakRequestedBytes, bytes);
        if (bytes > m_maxActiveBytes) {
            throw std::length_error("active byte budget request exceeds its configured limit");
        }
        const auto shouldWait = !CanAcquireLocked(bytes);
        const auto waitStart = std::chrono::steady_clock::now();
        m_condition.wait(lock, [this, bytes]() {
            return CanAcquireLocked(bytes);
        });
        if (shouldWait) {
            ++m_waitCount;
            if (m_collectTiming) {
                m_totalWaitNanoseconds += static_cast<std::uint64_t>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::steady_clock::now() - waitStart).count());
            }
        }
        ++m_acquireCount;
        m_activeBytes += bytes;
        m_peakActiveBytes = std::max(m_peakActiveBytes, m_activeBytes);
        return Lease(this, bytes);
    }

    [[nodiscard]] std::optional<Lease> TryAcquire(const std::uint64_t requestedBytes) {
        const auto bytes = std::max<std::uint64_t>(requestedBytes, 1u);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_peakRequestedBytes = std::max(m_peakRequestedBytes, bytes);
        if (!CanAcquireLocked(bytes)) {
            return std::nullopt;
        }
        ++m_acquireCount;
        m_activeBytes += bytes;
        m_peakActiveBytes = std::max(m_peakActiveBytes, m_activeBytes);
        return Lease(this, bytes);
    }

    [[nodiscard]] std::uint64_t ActiveBytes() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_activeBytes;
    }

    [[nodiscard]] std::uint64_t PeakActiveBytes() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_peakActiveBytes;
    }

    [[nodiscard]] std::uint64_t MaxActiveBytes() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_maxActiveBytes;
    }

    [[nodiscard]] ActiveByteBudgetStats SnapshotStats() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return ActiveByteBudgetStats{
            .maxActiveBytes = m_maxActiveBytes,
            .activeBytes = m_activeBytes,
            .peakActiveBytes = m_peakActiveBytes,
            .peakRequestedBytes = m_peakRequestedBytes,
            .acquireCount = m_acquireCount,
            .waitCount = m_waitCount,
            .totalWaitNanoseconds = m_totalWaitNanoseconds,
        };
    }

private:
    [[nodiscard]] bool CanAcquireLocked(const std::uint64_t bytes) const noexcept {
        return bytes <= m_maxActiveBytes &&
            m_activeBytes <= m_maxActiveBytes - bytes;
    }

    void Release(const std::uint64_t bytes) noexcept {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_activeBytes = bytes >= m_activeBytes ? 0u : m_activeBytes - bytes;
        }
        m_condition.notify_all();
    }

    std::uint64_t m_maxActiveBytes{1u};
    bool m_collectTiming{false};
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::uint64_t m_activeBytes{0u};
    std::uint64_t m_peakActiveBytes{0u};
    std::uint64_t m_peakRequestedBytes{0u};
    std::uint64_t m_acquireCount{0u};
    std::uint64_t m_waitCount{0u};
    std::uint64_t m_totalWaitNanoseconds{0u};
};

} // namespace resource
} // namespace datacodec

#endif
