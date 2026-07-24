#ifndef DATACODEC_STORAGE_BYTEIO_SCRATCHBYTEBUFFER_H
#define DATACODEC_STORAGE_BYTEIO_SCRATCHBYTEBUFFER_H

#include "DataCodec/Storage/ByteIO/ByteBudget.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <span>
#include <utility>
#include <vector>
namespace datacodec {

class ScratchByteBufferPool;

using ScratchByteQuotaLease = resource::ActiveByteBudget::Lease;
using ScratchByteQuotaAcquire = std::function<ScratchByteQuotaLease(std::uint64_t)>;

struct ScratchByteBufferPoolStats {
    std::size_t maxRetainedBlockCount{0u};
    std::size_t maxRetainedBlockBytes{0u};
    std::uint64_t maxRetainedTotalBytes{0u};
    std::uint64_t acquiredBytes{0u};
    std::uint64_t activeBytes{0u};
    std::uint64_t peakActiveBytes{0u};
    std::uint64_t retainedBytes{0u};
    std::uint64_t reusedBlockCount{0u};
    std::uint64_t allocationCount{0u};
};

class ScratchByteBuffer final {
public:
    ScratchByteBuffer() = default;
    ScratchByteBuffer(
        ScratchByteBufferPool* pool,
        std::vector<std::uint8_t> bytes,
        const std::size_t accountedBytes,
        ScratchByteQuotaLease quotaLease = {}) noexcept
        : m_pool(pool),
          m_bytes(std::move(bytes)),
          m_accountedBytes(accountedBytes),
          m_quotaLease(std::move(quotaLease)) {}

    ScratchByteBuffer(const ScratchByteBuffer&) = delete;
    ScratchByteBuffer& operator=(const ScratchByteBuffer&) = delete;

    ScratchByteBuffer(ScratchByteBuffer&& other) noexcept {
        MoveFrom(std::move(other));
    }

    ScratchByteBuffer& operator=(ScratchByteBuffer&& other) noexcept {
        if (this != &other) {
            Release();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    ~ScratchByteBuffer() { Release(); }

    [[nodiscard]] std::vector<std::uint8_t>& Bytes() noexcept { return m_bytes; }
    [[nodiscard]] const std::vector<std::uint8_t>& Bytes() const noexcept { return m_bytes; }
    [[nodiscard]] std::span<std::uint8_t> Span() noexcept {
        return std::span<std::uint8_t>(m_bytes.data(), m_bytes.size());
    }
    [[nodiscard]] std::span<const std::uint8_t> Span() const noexcept {
        return std::span<const std::uint8_t>(m_bytes.data(), m_bytes.size());
    }

    void Release() noexcept;

private:
    void MoveFrom(ScratchByteBuffer&& other) noexcept {
        m_pool = other.m_pool;
        m_bytes = std::move(other.m_bytes);
        m_accountedBytes = other.m_accountedBytes;
        m_quotaLease = std::move(other.m_quotaLease);
        other.m_pool = nullptr;
        other.m_accountedBytes = 0u;
    }

    ScratchByteBufferPool* m_pool{nullptr};
    std::vector<std::uint8_t> m_bytes;
    std::size_t m_accountedBytes{0u};
    ScratchByteQuotaLease m_quotaLease;
};

class ScratchByteBufferPool final {
public:
    explicit ScratchByteBufferPool(
        const std::size_t maxRetainedBlocks = 16u,
        const std::size_t maxRetainedBlockBytes = 64u * 1024u * 1024u,
        const std::uint64_t maxRetainedTotalBytes = 1024ull * 1024ull * 1024ull)
        : m_maxRetainedBlocks(maxRetainedBlocks),
          m_maxRetainedBlockBytes(maxRetainedBlockBytes),
          m_maxRetainedTotalBytes(maxRetainedTotalBytes) {}

    void Configure(
        const std::size_t maxRetainedBlocks,
        const std::size_t maxRetainedBlockBytes,
        const std::uint64_t maxRetainedTotalBytes) noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxRetainedBlocks = maxRetainedBlocks;
        m_maxRetainedBlockBytes = maxRetainedBlockBytes;
        m_maxRetainedTotalBytes = maxRetainedTotalBytes;
        TrimRetainedLocked();
    }

    [[nodiscard]] ScratchByteBuffer Acquire(const std::size_t bytes) {
        return Acquire(bytes, {});
    }

    [[nodiscard]] ScratchByteBuffer Acquire(
        const std::size_t bytes,
        ScratchByteQuotaLease quotaLease) {
        std::vector<std::uint8_t> block;
        bool reused = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto best = m_freeBlocks.end();
            for (auto it = m_freeBlocks.begin(); it != m_freeBlocks.end(); ++it) {
                if (it->capacity() >= bytes &&
                    (best == m_freeBlocks.end() || it->capacity() < best->capacity())) {
                    best = it;
                }
            }
            if (best != m_freeBlocks.end()) {
                m_retainedBytes -= static_cast<std::uint64_t>(best->capacity());
                block = std::move(*best);
                m_freeBlocks.erase(best);
                reused = true;
            }
        }
        block.resize(bytes);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_acquiredBytes += static_cast<std::uint64_t>(bytes);
            m_activeBytes += static_cast<std::uint64_t>(bytes);
            m_peakActiveBytes = std::max(m_peakActiveBytes, m_activeBytes);
            if (reused) {
                ++m_reusedBlockCount;
            } else {
                ++m_allocationCount;
            }
        }
        return ScratchByteBuffer(this, std::move(block), bytes, std::move(quotaLease));
    }

    void Return(
        std::vector<std::uint8_t> block,
        const std::size_t accountedBytes) noexcept {
        block.clear();
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto activeBytes = static_cast<std::uint64_t>(accountedBytes);
        m_activeBytes = activeBytes >= m_activeBytes ? 0u : m_activeBytes - activeBytes;
        const auto capacity = static_cast<std::uint64_t>(block.capacity());
        if (capacity == 0u ||
            block.capacity() > m_maxRetainedBlockBytes ||
            m_freeBlocks.size() >= m_maxRetainedBlocks ||
            capacity > m_maxRetainedTotalBytes - std::min(m_retainedBytes, m_maxRetainedTotalBytes)) {
            return;
        }
        m_retainedBytes += capacity;
        m_freeBlocks.push_back(std::move(block));
    }

    void Clear() noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::vector<std::uint8_t>>().swap(m_freeBlocks);
        m_acquiredBytes = 0u;
        m_activeBytes = 0u;
        m_peakActiveBytes = 0u;
        m_retainedBytes = 0u;
        m_reusedBlockCount = 0u;
        m_allocationCount = 0u;
    }

    [[nodiscard]] ScratchByteBufferPoolStats SnapshotStats() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return ScratchByteBufferPoolStats{
            .maxRetainedBlockCount = m_maxRetainedBlocks,
            .maxRetainedBlockBytes = m_maxRetainedBlockBytes,
            .maxRetainedTotalBytes = m_maxRetainedTotalBytes,
            .acquiredBytes = m_acquiredBytes,
            .activeBytes = m_activeBytes,
            .peakActiveBytes = m_peakActiveBytes,
            .retainedBytes = m_retainedBytes,
            .reusedBlockCount = m_reusedBlockCount,
            .allocationCount = m_allocationCount,
        };
    }

private:
    void TrimRetainedLocked() noexcept {
        while (!m_freeBlocks.empty() &&
               (m_freeBlocks.size() > m_maxRetainedBlocks ||
                m_retainedBytes > m_maxRetainedTotalBytes)) {
            m_retainedBytes -= static_cast<std::uint64_t>(m_freeBlocks.back().capacity());
            m_freeBlocks.pop_back();
        }
        for (auto it = m_freeBlocks.begin(); it != m_freeBlocks.end();) {
            if (it->capacity() > m_maxRetainedBlockBytes) {
                m_retainedBytes -= static_cast<std::uint64_t>(it->capacity());
                it = m_freeBlocks.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::size_t m_maxRetainedBlocks{16u};
    std::size_t m_maxRetainedBlockBytes{64u * 1024u * 1024u};
    std::uint64_t m_maxRetainedTotalBytes{1024ull * 1024ull * 1024ull};
    mutable std::mutex m_mutex;
    std::vector<std::vector<std::uint8_t>> m_freeBlocks;
    std::uint64_t m_acquiredBytes{0u};
    std::uint64_t m_activeBytes{0u};
    std::uint64_t m_peakActiveBytes{0u};
    std::uint64_t m_retainedBytes{0u};
    std::uint64_t m_reusedBlockCount{0u};
    std::uint64_t m_allocationCount{0u};
};

inline void ScratchByteBuffer::Release() noexcept {
    if (m_pool != nullptr) {
        m_pool->Return(std::move(m_bytes), m_accountedBytes);
        m_pool = nullptr;
    }
    m_accountedBytes = 0u;
    m_bytes.clear();
    m_quotaLease.Release();
}

} // namespace datacodec

#endif
